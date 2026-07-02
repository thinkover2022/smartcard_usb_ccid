/* ============================================================================
 *  usbd_ccid.c  -  USB CCID (smart-card, class 0x0B) device class for STM32-FS.
 *
 *  Bridges the ST USB Device Library to the ported CCID transport core
 *  (phExCcid_UsbCcid.c): bulk-OUT packets are handed to
 *  phExCcid_UsbCcid_Usb_Bulk_Out(); bulk-IN / interrupt-IN completions drive
 *  phExCcid_UsbCcid_Usb_Bulk_In() / _Usb_IntIn(). The endpoint byte I/O
 *  (hwUSB_ReadEP / hwUSB_WriteEP) is implemented here on USBD_LL_*.
 * ==========================================================================*/
#include "usbd_ccid.h"
#include "usbd_ctlreq.h"
#include "usbd_conf.h"
#include "mw_usbd_hw.h"
#include "phExCcid_UsbCcid.h"

/* Config descriptor: Config(9) + Interface(9) + CCID func(54) + 3xEP(7) = 93. */
#define CCID_CONFIG_DESC_SIZE   93U

/* the single device handle (referenced by the transport core via UsbHandle). */
USBD_HandleTypeDef hUsbDevice;
USBD_HANDLE_T      UsbHandle = (USBD_HANDLE_T)&hUsbDevice;

/* last bulk-OUT packet, staged for hwUSB_ReadEP(). */
static uint8_t  s_CcidRxBuf[CCID_EP_MAXPKT];
static uint32_t s_CcidRxLen;

static uint8_t CCID_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t CCID_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t CCID_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t CCID_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t CCID_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t *CCID_GetFSCfgDesc(uint16_t *length);
static uint8_t *CCID_GetDeviceQualifierDesc(uint16_t *length);

USBD_ClassTypeDef USBD_CCID = {
    CCID_Init,
    CCID_DeInit,
    CCID_Setup,
    NULL,                 /* EP0_TxSent */
    NULL,                 /* EP0_RxReady */
    CCID_DataIn,
    CCID_DataOut,
    NULL,                 /* SOF */
    NULL,                 /* IsoINIncomplete */
    NULL,                 /* IsoOUTIncomplete */
    CCID_GetFSCfgDesc,    /* HS (reuse FS) */
    CCID_GetFSCfgDesc,    /* FS */
    CCID_GetFSCfgDesc,    /* Other speed (reuse FS) */
    CCID_GetDeviceQualifierDesc,
};

__ALIGN_BEGIN static uint8_t s_CfgDesc[CCID_CONFIG_DESC_SIZE] __ALIGN_END = {
    /* ---- Configuration Descriptor (9) ---- */
    0x09, USB_DESC_TYPE_CONFIGURATION,
    LOBYTE(CCID_CONFIG_DESC_SIZE), HIBYTE(CCID_CONFIG_DESC_SIZE),
    0x01,                 /* bNumInterfaces */
    0x01,                 /* bConfigurationValue */
    0x00,                 /* iConfiguration */
    0x80,                 /* bmAttributes: bus-powered */
    0xFA,                 /* bMaxPower: 500 mA */

    /* ---- Interface Descriptor (9): Smart Card class 0x0B ---- */
    0x09, USB_DESC_TYPE_INTERFACE,
    0x00,                 /* bInterfaceNumber */
    0x00,                 /* bAlternateSetting */
    0x03,                 /* bNumEndpoints */
    0x0B,                 /* bInterfaceClass: CCID */
    0x00, 0x00,           /* SubClass, Protocol */
    0x00,                 /* iInterface */

    /* ---- CCID Class Descriptor (54, type 0x21) ---- */
    0x36, 0x21,
    0x10, 0x01,           /* bcdCCID 1.10 */
    0x00,                 /* bMaxSlotIndex */
    0x07,                 /* bVoltageSupport (5/3/1.8) */
    0x03, 0x00, 0x00, 0x00,   /* dwProtocols: T=0,T=1 */
    0x65, 0x0E, 0x00, 0x00,   /* dwDefaultClock 3.686 MHz */
    0xF0, 0x37, 0x00, 0x00,   /* dwMaximumClock */
    0x00,                 /* bNumClockSupported */
    0xB5, 0x26, 0x00, 0x00,   /* dwDataRate */
    0x80, 0xF0, 0x0C, 0x00,   /* dwMaxDataRate */
    0x00,                 /* bNumDataRatesSupported */
    0xFE, 0x00, 0x00, 0x00,   /* dwMaxIFSD */
    0x00, 0x00, 0x00, 0x00,   /* dwSynchProtocols */
    0x00, 0x00, 0x00, 0x00,   /* dwMechanical */
    0xBE, 0x04, 0x02, 0x00,   /* dwFeatures (auto params, TPDU) */
    0x0F, 0x01, 0x00, 0x00,   /* dwMaxCCIDMessageLength (271) */
    0x00,                 /* bClassGetResponse */
    0x00,                 /* bClassEnvelope */
    0x00, 0x00,           /* wLcdLayout */
    0x00,                 /* bPINSupport */
    0x01,                 /* bMaxCCIDBusySlots */

    /* ---- Bulk IN EP 0x81 (7) ---- */
    0x07, USB_DESC_TYPE_ENDPOINT, CCID_BULK_IN_EP, 0x02,
    LOBYTE(CCID_EP_MAXPKT), HIBYTE(CCID_EP_MAXPKT), 0x00,

    /* ---- Bulk OUT EP 0x01 (7) ---- */
    0x07, USB_DESC_TYPE_ENDPOINT, CCID_BULK_OUT_EP, 0x02,
    LOBYTE(CCID_EP_MAXPKT), HIBYTE(CCID_EP_MAXPKT), 0x00,

    /* ---- Interrupt IN EP 0x82 (7) ---- */
    0x07, USB_DESC_TYPE_ENDPOINT, CCID_INT_IN_EP, 0x03,
    LOBYTE(CCID_EP_MAXPKT), HIBYTE(CCID_EP_MAXPKT), 0x04,
};

__ALIGN_BEGIN static uint8_t s_QualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
    USB_LEN_DEV_QUALIFIER_DESC, USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00,
};

extern phExCcid_Usb_Bus_Status_t gphExCcid_sUsb_Bus_Status;

static uint8_t CCID_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USBD_LL_OpenEP(pdev, CCID_BULK_IN_EP,  USBD_EP_TYPE_BULK, CCID_EP_MAXPKT);
    USBD_LL_OpenEP(pdev, CCID_BULK_OUT_EP, USBD_EP_TYPE_BULK, CCID_EP_MAXPKT);
    USBD_LL_OpenEP(pdev, CCID_INT_IN_EP,   USBD_EP_TYPE_INTR, CCID_EP_MAXPKT);
    pdev->ep_in[CCID_BULK_IN_EP & 0xFU].is_used = 1U;
    pdev->ep_out[CCID_BULK_OUT_EP & 0xFU].is_used = 1U;
    pdev->ep_in[CCID_INT_IN_EP & 0xFU].is_used = 1U;

    gphExCcid_sUsb_Bus_Status.bAddressed = 1U;

    /* Arm the first bulk-OUT reception. */
    USBD_LL_PrepareReceive(pdev, CCID_BULK_OUT_EP, s_CcidRxBuf, CCID_EP_MAXPKT);
    return (uint8_t)USBD_OK;
}

static uint8_t CCID_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USBD_LL_CloseEP(pdev, CCID_BULK_IN_EP);
    USBD_LL_CloseEP(pdev, CCID_BULK_OUT_EP);
    USBD_LL_CloseEP(pdev, CCID_INT_IN_EP);
    gphExCcid_sUsb_Bus_Status.bAddressed = 0U;
    return (uint8_t)USBD_OK;
}

static uint8_t CCID_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    /* CCID class requests (ABORT/GET_CLOCK/GET_DATARATES): we advertise none
     * supported, so acknowledge no-data requests and stall data requests. */
    if ((req->bmRequest & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_CLASS)
    {
        if (req->wLength != 0U)
        {
            USBD_CtlError(pdev, req);
            return (uint8_t)USBD_FAIL;
        }
        return (uint8_t)USBD_OK;
    }
    return (uint8_t)USBD_OK;
}

static uint8_t CCID_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    (void)pdev;
    if (epnum == (CCID_BULK_IN_EP & 0x7FU))
    {
        phExCcid_UsbCcid_Usb_Bulk_In();       /* continue multi-packet response */
    }
    else if (epnum == (CCID_INT_IN_EP & 0x7FU))
    {
        phExCcid_UsbCcid_Usb_IntIn();
    }
    return (uint8_t)USBD_OK;
}

static uint8_t CCID_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    s_CcidRxLen = USBD_LL_GetRxDataSize(pdev, epnum);
    phExCcid_UsbCcid_Usb_Bulk_Out();          /* -> hwUSB_ReadEP + dispatch */
    /* re-arm for the next OUT packet */
    USBD_LL_PrepareReceive(pdev, CCID_BULK_OUT_EP, s_CcidRxBuf, CCID_EP_MAXPKT);
    return (uint8_t)USBD_OK;
}

static uint8_t *CCID_GetFSCfgDesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(s_CfgDesc);
    return s_CfgDesc;
}
static uint8_t *CCID_GetDeviceQualifierDesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(s_QualifierDesc);
    return s_QualifierDesc;
}

/* --------------------------------------------------------------------------
 *  Endpoint byte I/O used by the ported phExCcid_UsbCcid.c (was PN7462 ROM).
 * ------------------------------------------------------------------------ */
uint32_t hwUSB_ReadEP(USBD_HANDLE_T hUsb, uint8_t bEP, uint8_t *pBuf)
{
    (void)hUsb; (void)bEP;
    if (s_CcidRxLen != 0U)
    {
        (void)memcpy(pBuf, s_CcidRxBuf, s_CcidRxLen);
    }
    return s_CcidRxLen;
}

uint32_t hwUSB_WriteEP(USBD_HANDLE_T hUsb, uint8_t bEP, uint8_t *pBuf, uint32_t dwLen)
{
    (void)hUsb;
    (void)USBD_LL_Transmit(&hUsbDevice, bEP, pBuf, dwLen);
    return dwLen;
}
