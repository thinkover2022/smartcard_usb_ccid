/* ============================================================================
 *  phDriver_Stm32.c  -  GPIO / Timer / IRQ abstraction for STM32F1.
 *  Implements DAL/inc/phDriver.h on top of the STM32 HAL. Model:
 *  DAL/src/LPCOpen/phDriver_LPCOpen.c.
 *
 *  Pin encoding (from Board_Stm32Rc663.h): dwPinNumber = (PORT << 8) | PIN,
 *  PORT: A=0,B=1,C=2,D=3 ; PIN: 0..15.
 *
 *  NxpNfcRdLib's RC663 HAL uses phDriver for:
 *    - the front-end RESET/PDOWN pin (output),
 *    - the SSEL pin (output, software chip-select),
 *    - the IRQ pin (EXTI input, or polled via phDriver_PinRead),
 *    - a single-shot timer for RF timeouts (blocking or callback).
 *
 *  The timer uses TIM3 (16-bit, APB1). Blocking mode (NULL callback) busy-waits
 *  on the update flag; callback mode fires from TIM3_IRQHandler.
 * ==========================================================================*/

#include "phDriver.h"
#include "Board_Stm32Rc663.h"
#include "stm32f1xx_hal.h"

/* Provided by the application (main.c). Called from the RC663 IRQ EXTI line to
 * forward the interrupt to the HAL's RF ISR callback. Weak fallback does
 * nothing so this file also links when the app polls the IRQ pin instead. */
__attribute__((weak)) void CLIF_IRQHandler(void) { }

/* --------------------------------------------------------------------------
 *  GPIO
 * ------------------------------------------------------------------------ */
static GPIO_TypeDef * const s_apGpioPort[] = { GPIOA, GPIOB, GPIOC, GPIOD };

static void phDriver_Stm32EnableGpioClk(uint8_t bPortNum)
{
    switch (bPortNum)
    {
    case 0: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
    case 1: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
    case 2: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
    case 3: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
    default: break;
    }
}

phStatus_t phDriver_PinConfig(uint32_t dwPinNumber, phDriver_Pin_Func_t ePinFunc,
                              phDriver_Pin_Config_t *pPinConfig)
{
    GPIO_InitTypeDef sGpio = {0};
    uint8_t  bPortNum = (uint8_t)((dwPinNumber & 0x0000FF00U) >> 8);
    uint8_t  bPinNum  = (uint8_t)(dwPinNumber & 0x000000FFU);

    if ((pPinConfig == NULL) || (bPortNum >= (sizeof(s_apGpioPort)/sizeof(s_apGpioPort[0]))))
    {
        return (PH_DRIVER_ERROR | PH_COMP_DRIVER);
    }

    phDriver_Stm32EnableGpioClk(bPortNum);

    sGpio.Pin   = (uint16_t)(1U << bPinNum);
    sGpio.Speed = GPIO_SPEED_FREQ_HIGH;
    sGpio.Pull  = (pPinConfig->bPullSelect == PH_DRIVER_PULL_DOWN)
                      ? GPIO_PULLDOWN : GPIO_PULLUP;

    switch (ePinFunc)
    {
    case PH_DRIVER_PINFUNC_INPUT:
        sGpio.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(s_apGpioPort[bPortNum], &sGpio);
        break;

    case PH_DRIVER_PINFUNC_OUTPUT:
        sGpio.Mode = GPIO_MODE_OUTPUT_PP;
        HAL_GPIO_Init(s_apGpioPort[bPortNum], &sGpio);
        /* Apply the requested default output level. */
        HAL_GPIO_WritePin(s_apGpioPort[bPortNum], sGpio.Pin,
            pPinConfig->bOutputLogic ? GPIO_PIN_SET : GPIO_PIN_RESET);
        break;

    case PH_DRIVER_PINFUNC_INTERRUPT:
        /* EXTI needs the AFIO clock for the EXTICR line mux. */
        __HAL_RCC_AFIO_CLK_ENABLE();
        switch (pPinConfig->eInterruptConfig)
        {
        case PH_DRIVER_INTERRUPT_RISINGEDGE:  sGpio.Mode = GPIO_MODE_IT_RISING;         break;
        case PH_DRIVER_INTERRUPT_FALLINGEDGE: sGpio.Mode = GPIO_MODE_IT_FALLING;        break;
        case PH_DRIVER_INTERRUPT_EITHEREDGE:  sGpio.Mode = GPIO_MODE_IT_RISING_FALLING; break;
        default:
            /* Level-triggered EXTI is not supported by STM32 EXTI. */
            return (PH_DRIVER_ERROR | PH_COMP_DRIVER);
        }
        HAL_GPIO_Init(s_apGpioPort[bPortNum], &sGpio);

        HAL_NVIC_SetPriority((IRQn_Type)EINT_IRQn, EINT_PRIORITY, 0);
        HAL_NVIC_ClearPendingIRQ((IRQn_Type)EINT_IRQn);
        HAL_NVIC_EnableIRQ((IRQn_Type)EINT_IRQn);
        break;

    default:
        return (PH_DRIVER_ERROR | PH_COMP_DRIVER);
    }

    return PH_DRIVER_SUCCESS;
}

uint8_t phDriver_PinRead(uint32_t dwPinNumber, phDriver_Pin_Func_t ePinFunc)
{
    uint8_t  bPortNum = (uint8_t)((dwPinNumber & 0x0000FF00U) >> 8);
    uint8_t  bPinNum  = (uint8_t)(dwPinNumber & 0x000000FFU);
    uint16_t wPinMask = (uint16_t)(1U << bPinNum);

    if (ePinFunc == PH_DRIVER_PINFUNC_INTERRUPT)
    {
        /* Report whether the EXTI line for this pin is pending. */
        return (__HAL_GPIO_EXTI_GET_IT(wPinMask) != 0U) ? 1U : 0U;
    }

    return (HAL_GPIO_ReadPin(s_apGpioPort[bPortNum], wPinMask) == GPIO_PIN_SET) ? 1U : 0U;
}

void phDriver_PinWrite(uint32_t dwPinNumber, uint8_t bValue)
{
    uint8_t  bPortNum = (uint8_t)((dwPinNumber & 0x0000FF00U) >> 8);
    uint8_t  bPinNum  = (uint8_t)(dwPinNumber & 0x000000FFU);

    HAL_GPIO_WritePin(s_apGpioPort[bPortNum], (uint16_t)(1U << bPinNum),
                      bValue ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void phDriver_PinClearIntStatus(uint32_t dwPinNumber)
{
    uint8_t  bPinNum = (uint8_t)(dwPinNumber & 0x000000FFU);
    __HAL_GPIO_EXTI_CLEAR_IT((uint16_t)(1U << bPinNum));
}

/* --------------------------------------------------------------------------
 *  Single-shot timer (TIM3)
 * ------------------------------------------------------------------------ */
#define PH_DRIVER_STM32_TIMER            TIM3
#define PH_DRIVER_STM32_TIMER_IRQn       TIM3_IRQn
#define PH_DRIVER_STM32_TIMER_CLK_ENABLE()  __HAL_RCC_TIM3_CLK_ENABLE()
#define PH_DRIVER_STM32_TIMER_CLK_DISABLE() __HAL_RCC_TIM3_CLK_DISABLE()
#define STM32_TIMER_MAX_16BIT            0xFFFFU

static pphDriver_TimerCallBck_t s_pTimerCallBack;
static volatile uint8_t         s_bTimerExpired;

/* APB1 timer input clock: 2*PCLK1 unless the APB1 prescaler is 1. */
static uint32_t phDriver_Stm32GetTimerClk(void)
{
    uint32_t dwPclk1 = HAL_RCC_GetPCLK1Freq();

    if ((RCC->CFGR & RCC_CFGR_PPRE1) == RCC_CFGR_PPRE1_DIV1)
    {
        return dwPclk1;
    }
    return dwPclk1 * 2U;
}

phStatus_t phDriver_TimerStart(phDriver_Timer_Unit_t eTimerUnit, uint32_t dwTimePeriod,
                               pphDriver_TimerCallBck_t pTimerCallBack)
{
    uint64_t qwTicks;
    uint32_t dwPrescaler;
    uint32_t dwReload;
    uint32_t dwTimerClk = phDriver_Stm32GetTimerClk();

    /* total timer ticks = (clk / unit) * period */
    qwTicks  = (uint64_t)(dwTimerClk / (uint32_t)eTimerUnit);
    qwTicks *= dwTimePeriod;

    if (qwTicks == 0U)
    {
        qwTicks = 1U;
    }
    /* 16-bit counter x 16-bit prescaler upper bound. */
    if (qwTicks > ((uint64_t)(STM32_TIMER_MAX_16BIT + 1U) * (STM32_TIMER_MAX_16BIT + 1U)))
    {
        return (PH_DRIVER_ERROR | PH_COMP_DRIVER);
    }

    /* Split into prescaler + reload so both fit in 16 bits. */
    dwPrescaler = (uint32_t)((qwTicks + STM32_TIMER_MAX_16BIT) / (STM32_TIMER_MAX_16BIT + 1U));
    if (dwPrescaler == 0U)
    {
        dwPrescaler = 1U;
    }
    dwReload = (uint32_t)(qwTicks / dwPrescaler);
    if (dwReload == 0U)
    {
        dwReload = 1U;
    }

    s_bTimerExpired  = 0U;
    s_pTimerCallBack = pTimerCallBack;   /* NULL => blocking */

    PH_DRIVER_STM32_TIMER_CLK_ENABLE();

    /* Program a one-pulse up-counter that raises an update event at dwReload. */
    PH_DRIVER_STM32_TIMER->CR1  = TIM_CR1_OPM;               /* one-pulse mode */
    PH_DRIVER_STM32_TIMER->PSC  = (uint16_t)(dwPrescaler - 1U);
    PH_DRIVER_STM32_TIMER->ARR  = (uint16_t)(dwReload - 1U);
    PH_DRIVER_STM32_TIMER->CNT  = 0U;
    PH_DRIVER_STM32_TIMER->EGR  = TIM_EGR_UG;                /* latch PSC/ARR */
    PH_DRIVER_STM32_TIMER->SR   = 0U;                        /* clear stale flags */
    PH_DRIVER_STM32_TIMER->DIER = TIM_DIER_UIE;              /* update interrupt */

    HAL_NVIC_SetPriority(PH_DRIVER_STM32_TIMER_IRQn, EINT_PRIORITY + 1U, 0U);
    HAL_NVIC_ClearPendingIRQ(PH_DRIVER_STM32_TIMER_IRQn);
    HAL_NVIC_EnableIRQ(PH_DRIVER_STM32_TIMER_IRQn);

    PH_DRIVER_STM32_TIMER->CR1 |= TIM_CR1_CEN;              /* start */

    if (pTimerCallBack == NULL)
    {
        /* Blocking: wait for the ISR to flag expiry. */
        while (s_bTimerExpired == 0U)
        {
            /* busy-wait */
        }
    }

    return PH_DRIVER_SUCCESS;
}

phStatus_t phDriver_TimerStop(void)
{
    PH_DRIVER_STM32_TIMER->CR1  &= ~TIM_CR1_CEN;
    PH_DRIVER_STM32_TIMER->DIER  = 0U;
    HAL_NVIC_DisableIRQ(PH_DRIVER_STM32_TIMER_IRQn);
    PH_DRIVER_STM32_TIMER_CLK_DISABLE();
    return PH_DRIVER_SUCCESS;
}

/* --------------------------------------------------------------------------
 *  ISRs
 * ------------------------------------------------------------------------ */
void TIM3_IRQHandler(void)
{
    if ((PH_DRIVER_STM32_TIMER->SR & TIM_SR_UIF) != 0U)
    {
        PH_DRIVER_STM32_TIMER->SR &= ~TIM_SR_UIF;    /* clear update flag */
        PH_DRIVER_STM32_TIMER->CR1 &= ~TIM_CR1_CEN;  /* one-shot: ensure stopped */

        if (s_pTimerCallBack != NULL)
        {
            s_pTimerCallBack();
        }
        else
        {
            s_bTimerExpired = 1U;
        }

        HAL_NVIC_DisableIRQ(PH_DRIVER_STM32_TIMER_IRQn);
    }
}

/* RC663 IRQ line (see Board_Stm32Rc663.h: EINT_IRQn = EXTI1_IRQn, IRQ = PB1). */
void EXTI1_IRQHandler(void)
{
    /* Forward to the reader-IC RF ISR (weak CLIF_IRQHandler above / app-provided).
     * CLIF_IRQHandler is responsible for clearing the EXTI pending bit via
     * phDriver_PinClearIntStatus(). */
    CLIF_IRQHandler();
}
