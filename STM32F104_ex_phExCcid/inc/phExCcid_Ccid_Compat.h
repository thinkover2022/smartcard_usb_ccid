/* ============================================================================
 *  phExCcid_Ccid_Compat.h  -  portability shim for the CCID/PC-SC engine port.
 *
 *  The CCID protocol engine (phExCcid_UsbCcid_PCSC.c, phExCcid_UsbCcid_Process.c)
 *  is copied verbatim from PN7462AU_ex_phExCcid. On PN7462 it pulled in the ROM
 *  USB middleware (mw_usbd.h) and the PN7462 RTOS wrapper (phRtos.h). Those are
 *  replaced here by portable definitions so the same source compiles for STM32.
 *
 *  What lives here:
 *    - packed-struct macros (PH_PACK_STRUCT_BEGIN/END)
 *    - phStatus_t / PH_ERR_* (from NxpNfcRdLib ph_Status.h)
 *    - phUser_MemCpy / phUser_MemSet / PH_USER_ASSERT
 *    - phRtos_EventHandle_t (opaque event-group handle used by the CCID glue)
 * ==========================================================================*/
#ifndef PHEXCCID_CCID_COMPAT_H
#define PHEXCCID_CCID_COMPAT_H

#include <stdint.h>
#include <string.h>
#include "ph_Status.h"      /* phStatus_t, PH_ERR_SUCCESS, PH_COMP_* (NxpNfcRdLib) */
#include "FreeRTOS.h"
#include "event_groups.h"   /* EventGroupHandle_t backs phRtos_EventHandle_t */

/* ---- PN7462-only status alias not present in NxpNfcRdLib ph_Status.h ---- */
#ifndef PH_ERR_FAILED
#  define PH_ERR_FAILED   PH_ERR_INTERNAL_ERROR
#endif

/* ---- no-init section attribute (PN7462 ph_Datatypes.h macro) ---- */
#ifndef PH_NOINIT
#  define PH_NOINIT
#endif

/* ---- packed struct attribute (GCC) ---- */
#ifndef PH_PACK_STRUCT_BEGIN
#  define PH_PACK_STRUCT_BEGIN
#endif
#ifndef PH_PACK_STRUCT_END
#  define PH_PACK_STRUCT_END   __attribute__((packed))
#endif

/* ---- byte helpers used by the CCID engine ---- */
#ifndef phUser_MemCpy
#  define phUser_MemCpy(dst, src, len)   (void)memcpy((void *)(dst), (const void *)(src), (size_t)(len))
#endif
#ifndef phUser_MemSet
#  define phUser_MemSet(dst, val, len)   (void)memset((void *)(dst), (int)(val), (size_t)(len))
#endif

/* ---- assert: trap in a spin loop (matches PN7462 PH_USER_ASSERT "should never
 * reach here" semantics) ---- */
#ifndef PH_USER_ASSERT
#  define PH_USER_ASSERT(x)   do { if (!(x)) { for (;;) { } } } while (0)
#endif

/* ---- event-group handle used by phExCcid_Event_Info_t (FreeRTOS-backed). ---- */
typedef EventGroupHandle_t phRtos_EventHandle_t;

#endif /* PHEXCCID_CCID_COMPAT_H */
