/* ============================================================================
 *  APP_NxpBuild.h  -  NxpNfcRdLib compile-time feature switches for the
 *  STM32F104 + CLRC663 build. Force-included in every TU by the Makefile.
 *
 *  Difference vs the PN7462AU app config: the on-chip HAL switches
 *  (PHHAL_CT/PCR/PMU/RF/TIMER/LED/SYNCCT/USB, PHHAL_HW_PN7462AU) are removed and
 *  replaced by the external reader-IC HAL switch NXPBUILD__PHHAL_HW_RC663. The
 *  upper protocol/AL/discovery stack is identical.
 * ==========================================================================*/
#ifndef APP_NXP_BUILD_H
#define APP_NXP_BUILD_H

/* ---- Reader-IC HAL (external CLRC663 over SPI) ---- */
#define NXPBUILD__PHHAL_HW_RC663                 /**< CLRC663 front-end HAL */

/* ---- Protocol Abstraction Layer (contactless) ---- */
#define NXPBUILD__PHPAL_I14443P3A_SW             /**< ISO14443-3A */
#define NXPBUILD__PHPAL_I14443P3B_SW             /**< ISO14443-3B */
#define NXPBUILD__PHPAL_I14443P4_SW              /**< ISO14443-4  (T=CL) */
#define NXPBUILD__PHPAL_I14443P4A_SW             /**< ISO14443-4A (RATS/PPS) */
#define NXPBUILD__PHPAL_FELICA_SW                /**< FeliCa */
#define NXPBUILD__PHPAL_I18000P3M3_SW            /**< ISO18000-3m3 */
#define NXPBUILD__PHPAL_SLI15693_SW              /**< ISO15693 (SLI) */
#define NXPBUILD__PHPAL_MIFARE_SW                /**< MIFARE transport */

/* ---- Key store / crypto ---- */
#define NXPBUILD__PH_KEYSTORE_SW
#define NXPBUILD__PH_CRYPTOSYM_SW
#define NXPBUILD__PH_CRYPTORNG_SW

/* ---- Application layer (card commands) ---- */
#ifdef NXPBUILD__PHPAL_FELICA_SW
#   define NXPBUILD__PHAL_FELICA_SW
#endif
#ifdef NXPBUILD__PHPAL_MIFARE_SW
#   ifdef NXPBUILD__PH_KEYSTORE_SW
#       define NXPBUILD__PHAL_MFC_SW             /**< MIFARE Classic */
#   endif
#   define NXPBUILD__PHAL_MFDF_SW                /**< MIFARE DESFire */
#   define NXPBUILD__PHAL_MFUL_SW                /**< MIFARE Ultralight */
#endif
#ifdef NXPBUILD__PHPAL_SLI15693_SW
#   define NXPBUILD__PHAL_ICODE_SW
#endif
#ifdef NXPBUILD__PHPAL_I18000P3M3_SW
#   define NXPBUILD__PHAL_I18000P3M3_SW
#endif

/* ---- Discovery loop ---- */
#define NXPBUILD__PHAC_DISCLOOP_SW
#ifdef NXPBUILD__PHAC_DISCLOOP_SW
#   ifdef NXPBUILD__PHPAL_I14443P3A_SW
#       define NXPBUILD__PHAC_DISCLOOP_TYPEA_I3P3_TAGS
#       if defined(NXPBUILD__PHPAL_I14443P4A_SW) && defined(NXPBUILD__PHPAL_I14443P4_SW)
#           define NXPBUILD__PHAC_DISCLOOP_TYPEA_I3P4_TAGS
#       endif
#   endif
#   ifdef NXPBUILD__PHPAL_FELICA_SW
#       define NXPBUILD__PHAC_DISCLOOP_FELICA_TAGS
#       define NXPBUILD__PHAC_DISCLOOP_TYPEF_TAGS
#   endif
#   ifdef NXPBUILD__PHPAL_I14443P3B_SW
#       define NXPBUILD__PHAC_DISCLOOP_TYPEB_I3P3B_TAGS
#       ifdef NXPBUILD__PHPAL_I14443P4_SW
#           define NXPBUILD__PHAC_DISCLOOP_TYPEB_I3P4B_TAGS
#       endif
#   endif
#   ifdef NXPBUILD__PHPAL_SLI15693_SW
#       define NXPBUILD__PHAC_DISCLOOP_TYPEV_TAGS
#   endif
#   ifdef NXPBUILD__PHPAL_I18000P3M3_SW
#       define NXPBUILD__PHAC_DISCLOOP_I18000P3M3_TAGS
#   endif
#endif

/* ---- OS abstraction ---- */
#ifdef PH_OSAL_FREERTOS
#   define PHFL_HALAPI_WITH_RTOS
#endif
#ifdef PH_OSAL_NULLOS
#   define PHFL_HALAPI_NO_RTOS
#endif

/* TODO(port): CT (ISO7816) and USB-CCID are app-level here, not NxpNfcRdLib
 * switches. See PORTING.md steps 4-5. */

#endif /* APP_NXP_BUILD_H */
