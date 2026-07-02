/* ============================================================================
 *  usbd_conf.h  -  configuration for the ST USB Device Library (STM32F103 FS).
 * ==========================================================================*/
#ifndef USBD_CONF_H
#define USBD_CONF_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "stm32f1xx_hal.h"

#define USBD_MAX_NUM_INTERFACES        1U
#define USBD_MAX_NUM_CONFIGURATION     1U
#define USBD_MAX_STR_DESC_SIZ          64U
#define USBD_SELF_POWERED              1U
#define USBD_DEBUG_LEVEL               0U
#define USBD_SUPPORT_USER_STRING_DESC  0U
#define USBD_MAX_SUPPORTED_CLASS       1U
#define USBD_LPM_ENABLED               0U
#define USBD_CLASS_BOS_ENABLED         0U
#define USBD_CLASS_USER_STRING_DESC    0U
#define USBD_USER_REGISTER_CALLBACK    0U

/* Memory management: a small static pool (single class, no dynamic churn). */
void *USBD_static_malloc(uint32_t size);
void  USBD_static_free(void *p);
#define USBD_malloc         (void *)USBD_static_malloc
#define USBD_free           USBD_static_free
#define USBD_memset         memset
#define USBD_memcpy         memcpy
#define USBD_Delay          HAL_Delay

/* Logging disabled. */
#define USBD_UsrLog(...)
#define USBD_ErrLog(...)
#define USBD_DbgLog(...)

#endif /* USBD_CONF_H */
