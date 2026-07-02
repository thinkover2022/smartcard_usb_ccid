/* ============================================================================
 *  phExCcid_LED.h  -  STM32 port shim (status LEDs).
 *  Same API surface the CCID engine expects; the STM32 implementation
 *  (phExCcid_LED.c) drives one or more GPIOs (or is a no-op when a colour has
 *  no dedicated LED on the target board).
 * ==========================================================================*/
#ifndef PHEXCCID_LED_H
#define PHEXCCID_LED_H

#include "phExCcid_Ccid_Compat.h"

#define LED_ON      1
#define LED_OFF     0
#define BLUE_LED    9
#define GREEN_LED   10
#define YELLOW_LED  11
#define RED_LED     12

void phExCcid_LED_Status(uint8_t bLedColor, uint8_t bOnOff);
void phExCcid_All_LED_Off(void);
void phExCcid_All_LED_On(void);

void phExCcid_LED_TxnPass(uint8_t bSlotType);
void phExCcid_LED_TxnFail(uint8_t bSlotType);

#endif /* PHEXCCID_LED_H */
