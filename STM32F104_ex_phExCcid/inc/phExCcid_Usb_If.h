/* ============================================================================
 *  phExCcid_Usb_If.h  -  STM32 port shim.
 *  Replaces the PN7462 on-chip USB interface. The STM32 USB-FS device driver
 *  (Step 3b) provides the actual endpoint I/O; here we only expose the constants
 *  and prototypes the ported sources reference.
 * ==========================================================================*/
#ifndef PHEXCCID_USB_IF_H
#define PHEXCCID_USB_IF_H

#include "phExCcid_Ccid_Compat.h"

/* USB full-speed bulk max packet size (host <-> reader). */
#ifndef PN7462AU_USB_MAX_PACKET_SIZE
#  define PN7462AU_USB_MAX_PACKET_SIZE   64U
#endif

phStatus_t phExCcid_Usb_If_TotalInit(void);
phStatus_t phExCcid_Usb_If_Init(void);

#endif /* PHEXCCID_USB_IF_H */
