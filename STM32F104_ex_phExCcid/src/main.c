/* ============================================================================
 *  main.c  -  STM32F103 + CLRC663 USB-CCID reader (application entry).
 *
 *  Drives the ported phExCcid contactless framework:
 *    - phExCcid_Clif_HalInit()      : bring up the CLRC663 (SPI BAL + RC663 HAL)
 *    - phExCcidClif_DiscLoopConfig  : configure the NxpNfcRdLib discovery loop
 *    - phExCcidClif_PalInit         : init ISO14443-3/4, FeliCa, 15693, 18000 PALs
 *    - phExCcid_ClifMain(POLL)      : run discovery; on a card -> phExCcid_Poll_Main
 *                                     which classifies it, does L3/L4 activation,
 *                                     fills the CCID slot state, and services the
 *                                     PC/SC XfrBlock exchange via the CL event group.
 *
 *  The CCID/PC-SC engine (phExCcid_UsbCcid_*, ported verbatim) sits on top; the
 *  only piece still to wire is the STM32 USB-FS bulk endpoint I/O (Step 3b) that
 *  feeds phExCcid_UsbCcid_Process() and backs phExCcid_UsbCcid_Usb_Send().
 * ==========================================================================*/

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "ph_Status.h"     /* set PH_STATUS_H before phDriver.h (skip its fallback) */
#include "phDriver.h"
#include "phhalHw.h"
#include "phhalHw_Rc663.h"
#include "phacDiscLoop.h"
#include "phOsal.h"
#include "Board_Stm32Rc663.h"

#include "phExCcid.h"
#include "phExCcid_Clif.h"
#include "phExCcid_Poll.h"
#include "phExCcid_UsbCcid.h"

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_ccid.h"

/* On-board LED (Blue Pill: PC13, active-low). */
#define LED_PORT   GPIOC
#define LED_PIN    GPIO_PIN_13
#define LED_OFF()  HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET)

/* The reader HAL lives in phExCcid_Clif.c; pHal is used by the IRQ forwarder. */
extern phhalHw_Rc663_DataParams_t sHal;
static phhalHw_Rc663_DataParams_t * const pHal = &sHal;

/* Discovery-loop parameters + ATS holder (L4). */
static phacDiscLoop_Sw_DataParams_t s_DiscLoop;
static uint8_t                      s_bAts[64];

#define READER_TASK_STACK   700U           /* words */
static phOsal_ThreadObj_t s_ReaderTask;

/* --------------------------------------------------------------------------
 *  Clock / board init
 * ------------------------------------------------------------------------ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef sOsc = {0};
    RCC_ClkInitTypeDef sClk = {0};

    /* 8 MHz HSE -> PLL x9 -> 72 MHz SYSCLK. */
    sOsc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    sOsc.HSEState       = RCC_HSE_ON;
    sOsc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    sOsc.PLL.PLLState   = RCC_PLL_ON;
    sOsc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    sOsc.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&sOsc) != HAL_OK) { while (1) { } }

    sClk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                     RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    sClk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    sClk.AHBCLKDivider  = RCC_SYSCLK_DIV1;   /* HCLK  = 72 MHz */
    sClk.APB1CLKDivider = RCC_HCLK_DIV2;     /* PCLK1 = 36 MHz  */
    sClk.APB2CLKDivider = RCC_HCLK_DIV1;     /* PCLK2 = 72 MHz (SPI1) */
    if (HAL_RCC_ClockConfig(&sClk, FLASH_LATENCY_2) != HAL_OK) { while (1) { } }

    /* USB peripheral needs 48 MHz: 72 MHz / 1.5. */
    {
        RCC_PeriphCLKInitTypeDef sPeriph = {0};
        sPeriph.PeriphClockSelection = RCC_PERIPHCLK_USB;
        sPeriph.UsbClockSelection    = RCC_USBCLKSOURCE_PLL_DIV1_5;
        if (HAL_RCCEx_PeriphCLKConfig(&sPeriph) != HAL_OK) { while (1) { } }
    }
}

/* --------------------------------------------------------------------------
 *  USB device (CCID class) bring-up
 * ------------------------------------------------------------------------ */
static void Usb_Device_Init(void)
{
    USBD_Init(&hUsbDevice, &CCID_Desc, 0);
    USBD_RegisterClass(&hUsbDevice, &USBD_CCID);
    USBD_Start(&hUsbDevice);
}

static void Led_Init(void)
{
    GPIO_InitTypeDef sGpio = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    sGpio.Pin   = LED_PIN;
    sGpio.Mode  = GPIO_MODE_OUTPUT_PP;
    sGpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &sGpio);
    LED_OFF();
}

/* --------------------------------------------------------------------------
 *  RC663 IRQ line -> HAL RF ISR (from EXTI1_IRQHandler in phDriver_Stm32.c)
 * ------------------------------------------------------------------------ */
void CLIF_IRQHandler(void)
{
    if (phDriver_PinRead(PHDRIVER_PIN_IRQ, PH_DRIVER_PINFUNC_INTERRUPT))
    {
        if (pHal->pRFISRCallback != NULL)
        {
            pHal->pRFISRCallback(pHal);
        }
    }
    phDriver_PinClearIntStatus(PHDRIVER_PIN_IRQ);
}

/* --------------------------------------------------------------------------
 *  Reader task: discovery loop + contactless/CCID servicing (ported framework).
 * ------------------------------------------------------------------------ */
static void Reader_Task(void *pParam)
{
    (void)pParam;

    /* Bring up the CLRC663 and the discovery loop. */
    phExCcid_Clif_HalInit();

    if (phExCcidClif_DiscLoopConfig(&s_DiscLoop) != PH_ERR_SUCCESS) { for (;;) { } }
    phExCcidClif_DiscLoopParamInit(&s_DiscLoop, s_bAts);
    if (phExCcidClif_PalInit(&s_DiscLoop) != PH_ERR_SUCCESS) { for (;;) { } }

    /* CL event group: XfrBlock/Auth/Read/Write commands posted by the CCID engine
     * are consumed inside phExCcid_Poll_Main. */
    gphExCcid_sUsb_EventInfo.xCL_Events = xEventGroupCreate();

    for (;;)
    {
        (void)phExCcid_ClifMain(&s_DiscLoop, PHAC_DISCLOOP_ENTRY_POINT_POLL);
        (void)phOsal_ThreadDelay(100);
    }
}

/* --------------------------------------------------------------------------
 *  main
 * ------------------------------------------------------------------------ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    Led_Init();

    (void)phOsal_Init();

    /* USB-FS CCID device: enumerates as a PC/SC smart-card reader. */
    Usb_Device_Init();

    s_ReaderTask.pTaskName      = (uint8_t *)"Reader";
    s_ReaderTask.pStackBuffer   = NULL;                 /* dynamic stack */
    s_ReaderTask.priority       = 2U;
    s_ReaderTask.stackSizeInNum = READER_TASK_STACK;
    (void)phOsal_ThreadCreate(&s_ReaderTask.ThreadHandle, &s_ReaderTask,
                              &Reader_Task, NULL);

    phOsal_StartScheduler();

    for (;;) { }
    return 0;
}

/* --------------------------------------------------------------------------
 *  Keep HAL_GetTick() coherent once FreeRTOS owns SysTick (both 1 kHz).
 * ------------------------------------------------------------------------ */
uint32_t HAL_GetTick(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        return (uint32_t)xTaskGetTickCount();
    }
    return uwTick;
}
