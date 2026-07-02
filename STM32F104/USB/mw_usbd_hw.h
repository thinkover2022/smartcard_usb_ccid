/* STM32 port shim: replaces the PN7462 ROM USB middleware. Provides the endpoint
   byte-I/O the ported phExCcid_UsbCcid.c calls; implemented in usbd_ccid.c. */
#ifndef MW_USBD_HW_SHIM_H
#define MW_USBD_HW_SHIM_H
#include <stdint.h>
typedef void * USBD_HANDLE_T;
extern USBD_HANDLE_T UsbHandle;
uint32_t hwUSB_ReadEP (USBD_HANDLE_T hUsb, uint8_t bEP, uint8_t *pBuf);
uint32_t hwUSB_WriteEP(USBD_HANDLE_T hUsb, uint8_t bEP, uint8_t *pBuf, uint32_t dwLen);
#endif
