/* ============================================================================
 *  phRtos.h  -  STM32 port shim mapping the PN7462 RTOS wrapper onto FreeRTOS.
 *  Only the subset used by the ported phExCcid sources is provided.
 * ==========================================================================*/
#ifndef PHRTOS_SHIM_H
#define PHRTOS_SHIM_H

#include "phExCcid_Ccid_Compat.h"   /* phRtos_EventHandle_t (EventGroupHandle_t) */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

typedef QueueHandle_t phRtos_QueueHandle_t;
typedef TaskHandle_t  phRtos_TaskHandle_t;

typedef enum {
    phRtos_Fail    = 0,
    phRtos_Success = 1        /* == pdPASS */
} phRtos_Status_t;

#ifndef PHRTOS_MAX_DELAY
#  define PHRTOS_MAX_DELAY   portMAX_DELAY
#endif

/* ---- event groups ---- */
#define phRtos_EventGroupCreate()                       xEventGroupCreate()
#define phRtos_EventGroupSetBits(h, bits)               \
            (((h) != NULL) ? (xEventGroupSetBits((h), (bits)), phRtos_Success) : phRtos_Fail)
#define phRtos_EventGroupClearBits(h, bits)             xEventGroupClearBits((h), (bits))
#define phRtos_EventGroupWaitBits(h, bits, clr, all, to) \
            xEventGroupWaitBits((h), (bits), (clr), (all), (to))

/* ---- tasks ---- */
#define phRtos_TaskDelay(ticks)                         vTaskDelay((TickType_t)(ticks))

/* ---- queues (used by the not-yet-ported task framework; provided for headers) ---- */
#define phRtos_QueueCreate(len, isz)                    xQueueCreate((len), (isz))
#define phRtos_QueueSend(q, item, to)                   \
            ((xQueueSend((q), (item), (to)) == pdPASS) ? phRtos_Success : phRtos_Fail)
#define phRtos_QueueReceive(q, item, to)                xQueueReceive((q), (item), (to))

#endif /* PHRTOS_SHIM_H */
