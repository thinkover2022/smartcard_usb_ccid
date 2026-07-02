/* ============================================================================
 *  phExCcid_Ccid_Glue.c  -  residual glue for the STM32 CCID port.
 *
 *  The CCID transport core (phExCcid_UsbCcid.c) and the PC/SC engine
 *  (phExCcid_UsbCcid_PCSC.c / _Process.c) are now ported verbatim and own all
 *  the CCID globals + framing + event posting. The contactless side
 *  (phExCcid_Clif.c / _Poll.c) owns the poll timer, gMifareULC, etc.
 *
 *  What remains here is only the software timer shim (the PN7462 on-chip timer
 *  HAL has no RC663 equivalent) and the LED timer object whose owner
 *  (phExCcid.c) is not ported.
 * ==========================================================================*/
#include "phhalTimer.h"

static phhalTimer_Timers_t s_LedTimer;
phhalTimer_Timers_t *pLedTimer = &s_LedTimer;

void phhalTimer_Configure(phhalTimer_Timers_t *pTimer, uint32_t dwPeriodMs, void (*pCallBack)(void))
{
    if (pTimer != NULL)
    {
        pTimer->dwPeriodMs = dwPeriodMs;
        pTimer->pCallBack  = pCallBack;
        pTimer->bRunning   = 0U;
    }
}

void phhalTimer_Start(phhalTimer_Timers_t *pTimer, uint32_t bMode)
{
    (void)bMode;
    if (pTimer != NULL) { pTimer->bRunning = 1U; }
}

void phhalTimer_Stop(phhalTimer_Timers_t *pTimer)
{
    if (pTimer != NULL) { pTimer->bRunning = 0U; }
}
