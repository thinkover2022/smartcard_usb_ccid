#ifndef USBD_CCID_H
#define USBD_CCID_H
#include "usbd_core.h"

#define CCID_BULK_OUT_EP   0x01U
#define CCID_BULK_IN_EP    0x81U
#define CCID_INT_IN_EP     0x82U
#define CCID_EP_MAXPKT     64U

extern USBD_ClassTypeDef  USBD_CCID;
extern USBD_HandleTypeDef hUsbDevice;

#endif
