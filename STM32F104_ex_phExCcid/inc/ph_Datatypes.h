/* ============================================================================
 *  ph_Datatypes.h  -  STM32 port shim.
 *  The PN7462AU SDK's ph_Datatypes.h was dropped with that SDK. The copied CCID
 *  engine (phExCcid_UsbCcid_PCSC.c) includes it only for base integer types,
 *  the packed-struct macros and the byte helpers - all provided by the compat
 *  header. This shim just forwards to it.
 * ==========================================================================*/
#ifndef PH_DATATYPES_H
#define PH_DATATYPES_H

#include "phExCcid_Ccid_Compat.h"

#endif /* PH_DATATYPES_H */
