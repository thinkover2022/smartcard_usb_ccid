/* STM32 port shim: PN7462 flash-boot event ids/message (used in Clif msg struct). */
#ifndef PHFLASHBOOT_EVENT_SHIM_H
#define PHFLASHBOOT_EVENT_SHIM_H
#include "phExCcid_Ccid_Compat.h"
typedef enum {
    E_PH_BOOT = 0, E_PH_CLIF, E_PH_CT, E_PH_SYS
} phFlashBoot_Event_Ids;
typedef struct {
    uint32_t dwMsg[4];
    phFlashBoot_Event_Ids eSrcId;
    void *pvDes;
} phFlashBoot_Event_SysMsg_t;
#endif
