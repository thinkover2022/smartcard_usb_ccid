/* compat/ph_bool_fix.h - force-included first in every TU (see PN7462 build for
 * the full rationale). Establishes 'bool' as a 4-byte type once and sets both
 * guards (BOOL_DEFINED, __BOOL_DEFINED) so NxpNfcRdLib ph_TypeDefs.h and any SDK
 * ph_Datatypes.h don't fight over it under newlib. Paired with compat/stdbool.h. */
#ifndef PH_BOOL_FIX_H
#define PH_BOOL_FIX_H
#include <stdint.h>
#ifndef __cplusplus
typedef uint32_t bool;
#ifndef true
#  define true  (1)
#endif
#ifndef false
#  define false (0)
#endif
#ifndef TRUE
#  define TRUE  (1)
#endif
#ifndef FALSE
#  define FALSE (0)
#endif
#ifndef __BOOL_DEFINED
#  define __BOOL_DEFINED 1
#endif
#ifndef BOOL_DEFINED
#  define BOOL_DEFINED
#endif
#ifndef __bool_true_false_are_defined
#  define __bool_true_false_are_defined 1
#endif
#endif /* __cplusplus */
#endif /* PH_BOOL_FIX_H */
