/* ============================================================================
 *  usbd_desc.c  -  device / string descriptors for the CCID reader
 *  (VID 0x1FC9 / PID 0x0117, matching PN7462AU_ex_phExCcid).
 * ==========================================================================*/
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"

#define USBD_VID            0x1FC9U
#define USBD_PID            0x0117U
#define USBD_LANGID         0x0409U   /* English (US) */

static uint8_t *CCID_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *CCID_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *CCID_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *CCID_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *CCID_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *CCID_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *CCID_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);

USBD_DescriptorsTypeDef CCID_Desc = {
    CCID_DeviceDescriptor,
    CCID_LangIDStrDescriptor,
    CCID_ManufacturerStrDescriptor,
    CCID_ProductStrDescriptor,
    CCID_SerialStrDescriptor,
    CCID_ConfigStrDescriptor,
    CCID_InterfaceStrDescriptor,
};

__ALIGN_BEGIN static uint8_t s_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12,                       /* bLength */
    USB_DESC_TYPE_DEVICE,       /* bDescriptorType */
    0x00, 0x02,                 /* bcdUSB 2.00 */
    0x00,                       /* bDeviceClass (defined at interface) */
    0x00,                       /* bDeviceSubClass */
    0x00,                       /* bDeviceProtocol */
    USB_MAX_EP0_SIZE,           /* bMaxPacketSize0 (64) */
    LOBYTE(USBD_VID), HIBYTE(USBD_VID),
    LOBYTE(USBD_PID), HIBYTE(USBD_PID),
    0x01, 0x01,                 /* bcdDevice 1.01 */
    USBD_IDX_MFC_STR,           /* iManufacturer */
    USBD_IDX_PRODUCT_STR,       /* iProduct */
    USBD_IDX_SERIAL_STR,        /* iSerialNumber */
    USBD_MAX_NUM_CONFIGURATION  /* bNumConfigurations */
};

__ALIGN_BEGIN static uint8_t s_LangIDDesc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC, USB_DESC_TYPE_STRING, LOBYTE(USBD_LANGID), HIBYTE(USBD_LANGID)
};

__ALIGN_BEGIN static uint8_t s_StrDesc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

static uint8_t *CCID_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed; *length = (uint16_t)sizeof(s_DeviceDesc); return s_DeviceDesc;
}
static uint8_t *CCID_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed; *length = (uint16_t)sizeof(s_LangIDDesc); return s_LangIDDesc;
}
static uint8_t *CCID_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed; USBD_GetString((uint8_t *)"NXP", s_StrDesc, length); return s_StrDesc;
}
static uint8_t *CCID_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed; USBD_GetString((uint8_t *)"STM32 CLRC663 CCID Reader", s_StrDesc, length); return s_StrDesc;
}
static uint8_t *CCID_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed; USBD_GetString((uint8_t *)"0001", s_StrDesc, length); return s_StrDesc;
}
static uint8_t *CCID_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed; USBD_GetString((uint8_t *)"CCID Config", s_StrDesc, length); return s_StrDesc;
}
static uint8_t *CCID_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    (void)speed; USBD_GetString((uint8_t *)"CCID Interface", s_StrDesc, length); return s_StrDesc;
}
