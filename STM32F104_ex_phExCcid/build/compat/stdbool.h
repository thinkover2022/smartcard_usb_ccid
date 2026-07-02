/* compat/stdbool.h - <stdbool.h> shim (first on -I). Keeps 'bool' as the
 * ph_bool_fix.h type (uint32_t) instead of newlib's _Bool, and only supplies
 * the C99 true/false macros, so NxpNfcRdLib's '#include <stdbool.h>' cannot
 * turn bool into a 1-byte _Bool. See compat/ph_bool_fix.h. */
#ifndef PH_COMPAT_STDBOOL_H
#define PH_COMPAT_STDBOOL_H
#ifndef __cplusplus
#  ifndef true
#    define true  1
#  endif
#  ifndef false
#    define false 0
#  endif
#  ifndef __bool_true_false_are_defined
#    define __bool_true_false_are_defined 1
#  endif
   /* intentionally NO '#define bool _Bool' */
#endif
#endif /* PH_COMPAT_STDBOOL_H */
