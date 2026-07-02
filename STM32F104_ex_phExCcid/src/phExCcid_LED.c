/* ============================================================================
 *  phExCcid_LED.c  -  STM32 status-LED implementation for the CCID engine.
 *
 *  The reference board (F103 "Blue Pill") has a single on-board LED on PC13
 *  (active-low). There are no dedicated blue/green/yellow/red LEDs, so this
 *  maps "transaction pass / any-LED-on" to PC13 lit and "fail / off" to PC13
 *  dark. On a board with real multi-colour LEDs, extend phExCcid_LED_Status().
 * ==========================================================================*/
#include "phExCcid_LED.h"
#include "phLED.h"
#include "stm32f1xx_hal.h"

/* LED blink patterns referenced by the ported sources. Content is unused by the
 * minimal single-LED implementation, but the symbols must exist. */
const uint8_t gkphLED_BootUp[PH_LED_BLINK_LEN]    = {0};
const uint8_t gkphLED_TVDD_Fail[PH_LED_BLINK_LEN] = {0};
const uint8_t gkphLED_Ct_Pass[PH_LED_BLINK_LEN]   = {0};

void phLED_Init(void)                       { }
void phLED_SetPattern(const uint8_t *p)     { (void)p; }
void phLED_TimerCallback(void)              { }

#define LED_GPIO_PORT   GPIOC
#define LED_GPIO_PIN    GPIO_PIN_13
#define LED_WRITE(on)   HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, \
                            (on) ? GPIO_PIN_RESET : GPIO_PIN_SET)   /* active-low */

void phExCcid_LED_Status(uint8_t bLedColor, uint8_t bOnOff)
{
    (void)bLedColor;   /* single physical LED on this board */
    LED_WRITE(bOnOff == LED_ON);
}

void phExCcid_All_LED_Off(void)
{
    LED_WRITE(0);
}

void phExCcid_All_LED_On(void)
{
    LED_WRITE(1);
}

void phExCcid_LED_TxnPass(uint8_t bSlotType)
{
    (void)bSlotType;
    LED_WRITE(1);
}

void phExCcid_LED_TxnFail(uint8_t bSlotType)
{
    (void)bSlotType;
    LED_WRITE(0);
}
