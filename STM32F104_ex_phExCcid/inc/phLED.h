/* STM32 port shim: PN7462 phLED pattern engine -> minimal (see phExCcid_LED.c). */
#ifndef PHLED_SHIM_H
#define PHLED_SHIM_H
#include "phExCcid_Ccid_Compat.h"
#define PH_LED_BLINK_LEN 8
extern const uint8_t gkphLED_BootUp[PH_LED_BLINK_LEN];
extern const uint8_t gkphLED_TVDD_Fail[PH_LED_BLINK_LEN];
extern const uint8_t gkphLED_Ct_Pass[PH_LED_BLINK_LEN];
void phLED_Init(void);
void phLED_SetPattern(const uint8_t *pPattern);
void phLED_TimerCallback(void);
#endif
