/* STM32 port shim: PN7462 logging -> no-op (no debug UART wired). */
#ifndef PH_LOG_SHIM_H
#define PH_LOG_SHIM_H
#define PHFL_LOG_ENABLE 0
#define LOG_TXT(...)      do {} while (0)
#define LOG_AU8(...)      do {} while (0)
#define LOG_U32(...)      do {} while (0)
#define LOG_X32(...)      do {} while (0)
#define ph_Log(...)       do {} while (0)
#endif
