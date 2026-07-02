/* ============================================================================
 *  phbalReg_Stm32Spi.c  -  SPI Bus Abstraction Layer for STM32F1 <-> CLRC663
 *  Implements the four entry points of DAL/inc/phbalReg.h on the STM32 HAL SPI
 *  driver. Model: DAL/src/LPCOpen/phbalReg_LpcOpenSpi.c.
 *
 *  CLRC663 SPI framing: full-duplex, MSB first, CPOL=0/CPHA=0 (SPI mode 0),
 *  8-bit. NSS is driven by the RC663 HAL itself (phhalHw_Rc663_WriteSSEL ->
 *  phDriver_PinWrite(PHDRIVER_PIN_SSEL)), NOT here - so this BAL must NOT touch
 *  the chip-select line, it only performs the raw byte transfers.
 *
 *  Contract expected by the RC663 HAL (see phhalHw_Rc663_Int.c ReadData/WriteData):
 *    - write : Exchange(tx, n, 0,    NULL, &rd) -> transmit n, *rd == n
 *    - read  : Exchange(buf,n, n,    buf,  &rd) -> full-duplex n, *rd == n
 *  i.e. the returned length must always equal the Tx length ("fake full duplex").
 * ==========================================================================*/

#include "phDriver.h"          /* generic DAL: uint types, phStatus_t, PH_DRIVER_* */
#include "phbalReg.h"
#include "Board_Stm32Rc663.h"
#include "stm32f1xx_hal.h"

#define PHBAL_REG_STM32_SPI_ID      0x0EU   /**< ID for this STM32 SPI BAL component */

/* HAL SPI handle for the RC663 link. Exposed (non-static) so the SPI IRQ glue
 * or MSP callbacks in main.c can reference it if needed. */
SPI_HandleTypeDef g_Rc663Spi;

/* Per-transfer timeout in HAL ticks (ms). Adjustable via phbalReg_SetConfig. */
static uint32_t s_dwSpiTimeoutMs = 100U;

/* ---- map the PORT index used by the board header to a GPIO bank ---- */
static GPIO_TypeDef * const s_apGpioPort[] = { GPIOA, GPIOB, GPIOC, GPIOD };

static void phbalReg_Stm32EnableGpioClk(uint8_t bPortNum)
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

static void phbalReg_Stm32SpiPinConfig(void)
{
    GPIO_InitTypeDef sGpio = {0};
    uint8_t bPort;

    /* SCK / MOSI : alternate-function push-pull outputs.
     * On STM32F1 the AF is selected implicitly by the peripheral (no AFn field);
     * outputs just need GPIO_MODE_AF_PP. */
    bPort = (uint8_t)((PHDRIVER_PIN_SCK >> 8) & 0xFF);
    phbalReg_Stm32EnableGpioClk(bPort);
    sGpio.Pin   = (uint16_t)(1U << (PHDRIVER_PIN_SCK & 0xFF));
    sGpio.Mode  = GPIO_MODE_AF_PP;
    sGpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(s_apGpioPort[bPort], &sGpio);

    bPort = (uint8_t)((PHDRIVER_PIN_MOSI >> 8) & 0xFF);
    phbalReg_Stm32EnableGpioClk(bPort);
    sGpio.Pin = (uint16_t)(1U << (PHDRIVER_PIN_MOSI & 0xFF));
    HAL_GPIO_Init(s_apGpioPort[bPort], &sGpio);

    /* MISO : input, floating (driven by the RC663). */
    bPort = (uint8_t)((PHDRIVER_PIN_MISO >> 8) & 0xFF);
    phbalReg_Stm32EnableGpioClk(bPort);
    sGpio.Pin  = (uint16_t)(1U << (PHDRIVER_PIN_MISO & 0xFF));
    sGpio.Mode = GPIO_MODE_INPUT;
    sGpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(s_apGpioPort[bPort], &sGpio);
}

static void phbalReg_Stm32SpiClkEnable(SPI_TypeDef *pInstance)
{
    if (pInstance == SPI1)      { __HAL_RCC_SPI1_CLK_ENABLE(); }
#ifdef SPI2
    else if (pInstance == SPI2) { __HAL_RCC_SPI2_CLK_ENABLE(); }
#endif
}

/* ------------------------------------------------------------------------- */
phStatus_t phbalReg_Init(void *pDataParams, uint16_t wSizeOfDataParams)
{
    if ((pDataParams == NULL) || (sizeof(phbalReg_Type_t) != wSizeOfDataParams))
    {
        return (PH_DRIVER_ERROR | PH_COMP_DRIVER);
    }

    ((phbalReg_Type_t *)pDataParams)->wId      = PH_COMP_DRIVER | PHBAL_REG_STM32_SPI_ID;
    ((phbalReg_Type_t *)pDataParams)->bBalType = PHBAL_REG_TYPE_SPI;

    /* Route SCK/MOSI/MISO to the SPI peripheral. */
    phbalReg_Stm32SpiPinConfig();
    phbalReg_Stm32SpiClkEnable(RC663_SPI_INSTANCE);

    /* Configure the SPI master: mode 0, 8-bit, MSB-first, software NSS.
     * Baud prescaler is chosen so f_SCK <= RC663_SPI_BAUD_HZ. SPI1 sits on
     * APB2 (PCLK2 = SystemCoreClock on F103 @72 MHz); /16 -> 4.5 MHz. */
    g_Rc663Spi.Instance               = RC663_SPI_INSTANCE;
    g_Rc663Spi.Init.Mode              = SPI_MODE_MASTER;
    g_Rc663Spi.Init.Direction         = SPI_DIRECTION_2LINES;
    g_Rc663Spi.Init.DataSize          = SPI_DATASIZE_8BIT;
    g_Rc663Spi.Init.CLKPolarity       = SPI_POLARITY_LOW;   /* CPOL = 0 */
    g_Rc663Spi.Init.CLKPhase          = SPI_PHASE_1EDGE;    /* CPHA = 0 */
    g_Rc663Spi.Init.NSS               = SPI_NSS_SOFT;       /* NSS driven by GPIO */
    g_Rc663Spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    g_Rc663Spi.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    g_Rc663Spi.Init.TIMode            = SPI_TIMODE_DISABLE;
    g_Rc663Spi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    g_Rc663Spi.Init.CRCPolynomial     = 7;

    if (HAL_SPI_Init(&g_Rc663Spi) != HAL_OK)
    {
        return (PH_DRIVER_FAILURE | PH_COMP_DRIVER);
    }

    return PH_DRIVER_SUCCESS;
}

phStatus_t phbalReg_Exchange(void *pDataParams, uint16_t wOption,
                             uint8_t *pTxBuffer, uint16_t wTxLength,
                             uint16_t wRxBufSize, uint8_t *pRxBuffer,
                             uint16_t *pRxLength)
{
    HAL_StatusTypeDef eHal;
    uint16_t          wXferLen;

    (void)pDataParams;
    (void)wOption;

    if (wRxBufSize == 0U)
    {
        /* Transmit-only (register write). Report a "fake full duplex" length. */
        eHal     = HAL_SPI_Transmit(&g_Rc663Spi, pTxBuffer, wTxLength, s_dwSpiTimeoutMs);
        wXferLen = wTxLength;
    }
    else if (wTxLength == 0U)
    {
        /* Receive-only. */
        eHal     = HAL_SPI_Receive(&g_Rc663Spi, pRxBuffer, wRxBufSize, s_dwSpiTimeoutMs);
        wXferLen = wRxBufSize;
    }
    else
    {
        /* Full-duplex (register read: Tx and Rx share the same buffer/length). */
        eHal     = HAL_SPI_TransmitReceive(&g_Rc663Spi, pTxBuffer, pRxBuffer,
                                           wTxLength, s_dwSpiTimeoutMs);
        wXferLen = wTxLength;
    }

    if (eHal == HAL_TIMEOUT)
    {
        return (PH_DRIVER_TIMEOUT | PH_COMP_DRIVER);
    }
    if (eHal != HAL_OK)
    {
        return (PH_DRIVER_FAILURE | PH_COMP_DRIVER);
    }

    if (pRxLength != NULL)
    {
        *pRxLength = wXferLen;
    }

    return PH_DRIVER_SUCCESS;
}

phStatus_t phbalReg_SetConfig(void *pDataParams, uint16_t wConfig, uint16_t wValue)
{
    (void)pDataParams;

    switch (wConfig)
    {
    case PHBAL_REG_CONFIG_WRITE_TIMEOUT_MS:
    case PHBAL_REG_CONFIG_READ_TIMEOUT_MS:
        s_dwSpiTimeoutMs = (uint32_t)wValue;
        break;
    default:
        return (PH_DRIVER_ERROR | PH_COMP_DRIVER);
    }
    return PH_DRIVER_SUCCESS;
}

phStatus_t phbalReg_GetConfig(void *pDataParams, uint16_t wConfig, uint16_t *pValue)
{
    (void)pDataParams;

    if (pValue == NULL)
    {
        return (PH_DRIVER_ERROR | PH_COMP_DRIVER);
    }

    switch (wConfig)
    {
    case PHBAL_REG_CONFIG_WRITE_TIMEOUT_MS:
    case PHBAL_REG_CONFIG_READ_TIMEOUT_MS:
        *pValue = (uint16_t)s_dwSpiTimeoutMs;
        break;
    default:
        return (PH_DRIVER_ERROR | PH_COMP_DRIVER);
    }
    return PH_DRIVER_SUCCESS;
}
