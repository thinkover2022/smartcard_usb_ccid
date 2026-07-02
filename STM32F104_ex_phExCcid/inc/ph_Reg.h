/* STM32 port shim: PN7462 register-access macros are unused after HAL swap. */
#ifndef PH_REG_SHIM_H
#define PH_REG_SHIM_H
#define PH_REG_SET(reg, val)      do { (void)(val); } while (0)
#define PH_REG_GET(reg)           (0U)
#define PH_REG_SET_BIT(reg, m)    do {} while (0)
#define PH_REG_CLEAR_BIT(reg, m)  do {} while (0)
#define PH_REG_TEST_BIT(reg, m)   (0U)
#endif
