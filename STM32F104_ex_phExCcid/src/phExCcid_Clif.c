/* ============================================================================
 *  phExCcid_Clif.c  -  STM32 port of the contactless-interface layer.
 *
 *  Adapted from PN7462AU_ex_phExCcid/src/phExCcid_Clif.c. The ONLY changes vs the
 *  original are the reader-HAL binding and the removal of PN7462-only bits:
 *    - HAL: phhalHw_PN7462AU_Init  ->  phhalHw_Rc663_Init (external CLRC663/SPI)
 *    - dropped: phhalRf/phhalPmu (TxLDO monitor), PCR GPREG LPCD persistence,
 *               phLED pattern engine.
 *  phExCcidClif_PalInit / DiscLoopConfig / ClifMain / DiscLoopParamInit are the
 *  original generic NxpNfcRdLib logic (HAL-agnostic) and are unchanged in spirit.
 * ==========================================================================*/
#include "ph_Datatypes.h"
#include "phhalHw.h"
#include "phhalHw_Rc663.h"
#include "phpalFelica.h"
#include "phExCcid.h"
#include "phExCcid_Clif.h"
#include "phacDiscLoop.h"
#include "ph_Status.h"
#include "phExCcid_Poll.h"
#include "phUser.h"
#include "phExCcid_LED.h"
#include "phpalI14443p3b.h"
#include "phpalI14443p3a.h"
#include "phpalI14443p4a.h"
#include "phpalI14443p4.h"
#include "phpalSli15693.h"
#ifdef NXPBUILD__PHAC_DISCLOOP_I18000P3M3_TAGS
#include "phpalI18000p3m3.h"
#include "phalI18000p3m3.h"
#endif
#include "phExCcid_UsbCcid.h"
#include "phKeyStore.h"

#include "phDriver.h"
#include "phbalReg.h"
#include "Board_Stm32Rc663.h"

/* Reader HAL (RC663) + BAL + global Tx/Rx buffers. sHal is shared with main.c. */
phhalHw_Rc663_DataParams_t         sHal;
static phbalReg_Type_t             sBal;
static uint8_t gphExCcid_Rxbuf[PH_EXCCID_CLIF_RXBUFSIZE] = {0};
static uint8_t gphExCcid_Txbuf[PH_EXCCID_CLIF_TXBUFSIZE] = {0};

#if defined(NXPBUILD__PH_KEYSTORE_SW)
#define PH_EXCCID_NO_OF_KEYENTRIES        2U
#define PH_EXCCID_NO_OF_KEYVERSIONPAIRS   2U
#define PH_EXCCID_NO_OF_KUCENTRIES        1U
phKeyStore_Sw_DataParams_t sKeyStore;
static phKeyStore_Sw_KeyEntry_t       sKeyEntries[PH_EXCCID_NO_OF_KEYENTRIES];
static phKeyStore_Sw_KeyVersionPair_t sKeyVersionPairs[PH_EXCCID_NO_OF_KEYVERSIONPAIRS * PH_EXCCID_NO_OF_KEYENTRIES];
static phKeyStore_Sw_KUCEntry_t       sKUCEntries[PH_EXCCID_NO_OF_KUCENTRIES];
#endif

PH_NOINIT static phpalI14443p3a_Sw_DataParams_t  gphpal_Sw_DataParams3A;
PH_NOINIT static phpalI14443p4a_Sw_DataParams_t  gphpal_Sw_DataParams4A;
PH_NOINIT static phpalI14443p4_Sw_DataParams_t   gphpal_Sw_DataParams4;
PH_NOINIT static phpalFelica_Sw_DataParams_t     gphpal_Sw_DataParamsF;
PH_NOINIT static phpalI14443p3b_Sw_DataParams_t  gphpal_Sw_DataParamsB;
PH_NOINIT static phpalSli15693_Sw_DataParams_t   gphpal_Sw_DataParams15693;
#ifdef NXPBUILD__PHAC_DISCLOOP_I18000P3M3_TAGS
PH_NOINIT static phpalI18000p3m3_Sw_DataParams_t gphpal_Sw_DataParams18000;
PH_NOINIT static phalI18000p3m3_Sw_DataParams_t  gphal_Sw_DataParams18000;
#endif

PH_NOINIT phhalTimer_Timers_t *gpphExCcid_Clif_PollTimer;
extern phhalTimer_Timers_t *pLedTimer;

/* ------------------------------------------------------------------------- */
phStatus_t phExCcid_ClifMain(phacDiscLoop_Sw_DataParams_t *psDiscLoopParams,
                             phacDiscLoop_Sw_EntryPoints_t eDiscLoopEntry)
{
    phStatus_t wDiscLoopStatus;

    wDiscLoopStatus = phacDiscLoop_Run(psDiscLoopParams, (uint8_t)eDiscLoopEntry);
    wDiscLoopStatus &= PH_ERR_MASK;

    switch (wDiscLoopStatus)
    {
    case PHAC_DISCLOOP_DEVICE_ACTIVATED:
        phhalTimer_Stop(pLedTimer);
        phExCcid_All_LED_Off();
        phExCcid_Poll_Main(psDiscLoopParams);       /* -> classify + activate + CCID slot */
        break;
    default:
        break;
    }

    (void)phhalHw_FieldOff(psDiscLoopParams->pHalDataParams);
    return (phStatus_t)wDiscLoopStatus;
}

void phExCcid_Clif_HalInit(void)
{
    phStatus_t            status;
    phDriver_Pin_Config_t pinCfg;

    phUser_MemSet((void *)&gphExCcid_Rxbuf[0], 0x00, PH_EXCCID_CLIF_RXBUFSIZE);
    phUser_MemSet((void *)&gphExCcid_Txbuf[0], 0x00, PH_EXCCID_CLIF_TXBUFSIZE);
    phUser_MemSet((void *)&sHal, 0x00, sizeof(sHal));

    /* SPI BAL to the external CLRC663. */
    PH_USER_ASSERT(phbalReg_Init(&sBal, sizeof(phbalReg_Type_t)) == PH_DRIVER_SUCCESS);

    /* RC663 RESET pin, then a reset pulse. */
    pinCfg.bPullSelect  = PHDRIVER_PIN_RESET_PULL_CFG;
    pinCfg.bOutputLogic = RESET_POWERUP_LEVEL;
    (void)phDriver_PinConfig(PHDRIVER_PIN_RESET, PH_DRIVER_PINFUNC_OUTPUT, &pinCfg);
    phDriver_PinWrite(PHDRIVER_PIN_RESET, RESET_POWERDOWN_LEVEL);
    (void)phDriver_TimerStart(PH_DRIVER_TIMER_MILLI_SECS, 2U, NULL);
    phDriver_PinWrite(PHDRIVER_PIN_RESET, RESET_POWERUP_LEVEL);
    (void)phDriver_TimerStart(PH_DRIVER_TIMER_MILLI_SECS, 5U, NULL);

    status = phhalHw_Rc663_Init(&sHal, sizeof(sHal), &sBal, NULL,
                                gphExCcid_Txbuf, sizeof(gphExCcid_Txbuf),
                                gphExCcid_Rxbuf, sizeof(gphExCcid_Rxbuf));
    PH_USER_ASSERT(status == PH_ERR_SUCCESS);

    sHal.bBalConnectionType = PHHAL_HW_BAL_CONNECTION_SPI;

    /* RC663 IRQ pin as EXTI (forwarded to the HAL RF ISR by main's CLIF_IRQHandler). */
    pinCfg.bPullSelect      = PHDRIVER_PIN_IRQ_PULL_CFG;
    pinCfg.bOutputLogic     = PH_DRIVER_SET_LOW;
    pinCfg.eInterruptConfig = PIN_IRQ_TRIGGER_TYPE;
    (void)phDriver_PinConfig(PHDRIVER_PIN_IRQ, PH_DRIVER_PINFUNC_INTERRUPT, &pinCfg);

#if defined(NXPBUILD__PH_KEYSTORE_SW)
    (void)phKeyStore_Sw_Init(&sKeyStore, sizeof(sKeyStore),
                             sKeyEntries, PH_EXCCID_NO_OF_KEYENTRIES,
                             sKeyVersionPairs, PH_EXCCID_NO_OF_KEYVERSIONPAIRS,
                             sKUCEntries, PH_EXCCID_NO_OF_KUCENTRIES);
#endif
}

phStatus_t phExCcidClif_PalInit(phacDiscLoop_Sw_DataParams_t *psDiscLoopParams)
{
    if (psDiscLoopParams->bPasPollTechCfg & (1 << PHAC_DISCLOOP_TECH_TYPE_A))
    {
        phUser_MemSet((void *)&gphpal_Sw_DataParams3A, 0x00, sizeof(phpalI14443p3a_Sw_DataParams_t));
        phUser_MemSet((void *)&gphpal_Sw_DataParams4A, 0x00, sizeof(phpalI14443p4a_Sw_DataParams_t));
        if (phpalI14443p3a_Sw_Init(&gphpal_Sw_DataParams3A, sizeof(gphpal_Sw_DataParams3A), &sHal) != PH_ERR_SUCCESS)
            return PH_ERR_INTERNAL_ERROR;
        if (phpalI14443p4a_Sw_Init(&gphpal_Sw_DataParams4A, sizeof(gphpal_Sw_DataParams4A), &sHal) != PH_ERR_SUCCESS)
            return PH_ERR_INTERNAL_ERROR;
        psDiscLoopParams->pPal1443p3aDataParams = (void *)&gphpal_Sw_DataParams3A;
        psDiscLoopParams->pPal1443p4aDataParams = (void *)&gphpal_Sw_DataParams4A;
    }

    if (psDiscLoopParams->bPasPollTechCfg & (1 << PHAC_DISCLOOP_TECH_TYPE_B))
    {
        (void)phUser_MemSet((void *)&gphpal_Sw_DataParamsB, 0x00, sizeof(phpalI14443p3b_Sw_DataParams_t));
        if (phpalI14443p3b_Sw_Init(&gphpal_Sw_DataParamsB, sizeof(gphpal_Sw_DataParamsB), &sHal) != PH_ERR_SUCCESS)
            return PH_ERR_INTERNAL_ERROR;
        psDiscLoopParams->pPal1443p3bDataParams = (void *)&gphpal_Sw_DataParamsB;
    }

    if (psDiscLoopParams->bPasPollTechCfg & ((1 << PHAC_DISCLOOP_TECH_TYPE_A) | (1 << PHAC_DISCLOOP_TECH_TYPE_B)))
    {
        phUser_MemSet((void *)&gphpal_Sw_DataParams4, 0x00, sizeof(phpalI14443p4_Sw_DataParams_t));
        if (phpalI14443p4_Sw_Init(&gphpal_Sw_DataParams4, sizeof(gphpal_Sw_DataParams4), &sHal) != PH_ERR_SUCCESS)
            return PH_ERR_INTERNAL_ERROR;
        psDiscLoopParams->pPal14443p4DataParams = (void *)&gphpal_Sw_DataParams4;
    }

    if (psDiscLoopParams->bPasPollTechCfg & ((1 << PHAC_DISCLOOP_TECH_TYPE_F212) | (1 << PHAC_DISCLOOP_TECH_TYPE_F424)))
    {
        phUser_MemSet((void *)&gphpal_Sw_DataParamsF, 0x00, sizeof(phpalFelica_Sw_DataParams_t));
        if (phpalFelica_Sw_Init(&gphpal_Sw_DataParamsF, sizeof(gphpal_Sw_DataParamsF), &sHal) != PH_ERR_SUCCESS)
            return PH_ERR_INTERNAL_ERROR;
        psDiscLoopParams->pPalFelicaDataParams = (void *)&gphpal_Sw_DataParamsF;
    }

    if (psDiscLoopParams->bPasPollTechCfg & (1 << PHAC_DISCLOOP_TECH_TYPE_V))
    {
        (void)phUser_MemSet((void *)&gphpal_Sw_DataParams15693, 0x00, sizeof(phpalSli15693_Sw_DataParams_t));
        if (phpalSli15693_Sw_Init(&gphpal_Sw_DataParams15693, sizeof(gphpal_Sw_DataParams15693), &sHal) != PH_ERR_SUCCESS)
            return PH_ERR_INTERNAL_ERROR;
        psDiscLoopParams->pPalSli15693DataParams = (void *)&gphpal_Sw_DataParams15693;
    }
#ifdef NXPBUILD__PHAC_DISCLOOP_I18000P3M3_TAGS
    if (psDiscLoopParams->bPasPollTechCfg & (1 << PHAC_DISCLOOP_TECH_TYPE_18000P3M3))
    {
        (void)phUser_MemSet((void *)&gphpal_Sw_DataParams18000, 0x00, sizeof(phpalI18000p3m3_Sw_DataParams_t));
        if (phpalI18000p3m3_Sw_Init(&gphpal_Sw_DataParams18000, sizeof(gphpal_Sw_DataParams18000), &sHal) != PH_ERR_SUCCESS)
            return PH_ERR_INTERNAL_ERROR;
        psDiscLoopParams->pPal18000p3m3DataParams = (void *)&gphpal_Sw_DataParams18000;
        if (phalI18000p3m3_Sw_Init(&gphal_Sw_DataParams18000, sizeof(gphal_Sw_DataParams18000), &gphpal_Sw_DataParams18000) != PH_ERR_SUCCESS)
            return PH_ERR_INTERNAL_ERROR;
        psDiscLoopParams->pAl18000p3m3DataParams = (void *)&gphal_Sw_DataParams18000;
    }
#endif

    return PH_ERR_SUCCESS;
}

void phExCcidClif_DeInit(void *phhalHwClifRdLib)
{
    (void)phhalHw_FieldOff(phhalHwClifRdLib);
}

uint16_t phExCcidClif_DiscLoopConfig(phacDiscLoop_Sw_DataParams_t *psDiscLoopParams)
{
    uint16_t wStatus;

    wStatus = phacDiscLoop_Sw_Init(psDiscLoopParams, sizeof(phacDiscLoop_Sw_DataParams_t), &sHal);
    if (wStatus != PH_ERR_SUCCESS) return PH_ERR_INTERNAL_ERROR;

    psDiscLoopParams->pHalDataParams = &sHal;

    (void)phacDiscLoop_SetConfig(psDiscLoopParams, PHAC_DISCLOOP_CONFIG_ENABLE_LPCD, false);
    (void)phacDiscLoop_SetConfig(psDiscLoopParams, PHAC_DISCLOOP_CONFIG_PAS_POLL_TECH_CFG, 0x00);
    (void)phacDiscLoop_SetConfig(psDiscLoopParams, PHAC_DISCLOOP_CONFIG_PAS_LIS_TECH_CFG, 0x00);
    (void)phacDiscLoop_SetConfig(psDiscLoopParams, PHAC_DISCLOOP_CONFIG_ACT_POLL_TECH_CFG, 0x00);
    (void)phacDiscLoop_SetConfig(psDiscLoopParams, PHAC_DISCLOOP_CONFIG_ACT_LIS_TECH_CFG, 0x00);

    wStatus = phacDiscLoop_SetConfig(psDiscLoopParams, PHAC_DISCLOOP_CONFIG_PAS_POLL_TECH_CFG,
                  PHAC_DISCLOOP_POS_BIT_MASK_A | PHAC_DISCLOOP_POS_BIT_MASK_B |
                  PHAC_DISCLOOP_POS_BIT_MASK_F212 | PHAC_DISCLOOP_POS_BIT_MASK_F424 |
                  PHAC_DISCLOOP_POS_BIT_MASK_V | PHAC_DISCLOOP_POS_BIT_MASK_18000P3M3);
    if (wStatus != PH_ERR_SUCCESS) return PH_ERR_INTERNAL_ERROR;

    wStatus = phacDiscLoop_SetConfig(psDiscLoopParams, PHAC_DISCLOOP_CONFIG_BAIL_OUT,
                  PHAC_DISCLOOP_POS_BIT_MASK_A | PHAC_DISCLOOP_POS_BIT_MASK_B |
                  PHAC_DISCLOOP_POS_BIT_MASK_F212 | PHAC_DISCLOOP_POS_BIT_MASK_F424 |
                  PHAC_DISCLOOP_POS_BIT_MASK_V);
    return wStatus;
}

void phExCcidClif_DiscLoopParamInit(phacDiscLoop_Sw_DataParams_t *psDiscLoopParams, uint8_t *pbAts)
{
    psDiscLoopParams->sTypeATargetInfo.sTypeA_I3P4.pAts = pbAts;
}
