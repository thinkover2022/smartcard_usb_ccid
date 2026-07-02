/* ============================================================================
 *  phhalTimer.h  -  STM32 port shim.
 *  On PN7462 this was the on-chip timer HAL. The CCID engine only uses it to
 *  start/stop the contactless poll timer and the LED timer. Here it is a thin
 *  software-timer abstraction backed by the CCID glue (phExCcid_Ccid_Glue.c),
 *  which can drive it from a FreeRTOS software timer or the phDriver TIM.
 * ==========================================================================*/
#ifndef PHHALTIMER_SHIM_H
#define PHHALTIMER_SHIM_H

#include "phExCcid_Ccid_Compat.h"

/* Timer run modes (values kept compatible with the PN7462 enum usage). */
typedef enum {
    E_TIMER_SINGLE_SHOT  = 0,
    E_TIMER_FREE_RUNNING = 1
} phhalTimer_TimerMode_t;

/* Opaque software-timer descriptor. */
typedef struct {
    uint32_t  dwPeriodMs;         /**< configured period (ms) */
    void    (*pCallBack)(void);   /**< expiry callback */
    uint8_t   bRunning;
} phhalTimer_Timers_t;

void phhalTimer_Start(phhalTimer_Timers_t *pTimer, uint32_t bMode);
void phhalTimer_Stop (phhalTimer_Timers_t *pTimer);
void phhalTimer_Configure(phhalTimer_Timers_t *pTimer, uint32_t dwPeriodMs, void (*pCallBack)(void));

#endif /* PHHALTIMER_SHIM_H */
