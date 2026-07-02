/* STM32 port shim: PN7462 PCR (power/clock/reset). Only the boot-reason enum is
   referenced by headers; provide it so struct/proto declarations resolve. */
#ifndef PHHALPCR_SHIM_H
#define PHHALPCR_SHIM_H
typedef enum {
    E_BOOT_NONE = 0, E_STARTUP_POR, E_SOFT_RESET, E_ACTIVE_HPD,
    E_TEMP_SENSOR0, E_TEMP_SENSOR1, E_HIF_RESET, E_WATCH_DOG,
    E_NO_PVDD, E_SWP_DET, E_RFLDT_BOOT, E_WUC_CNT, E_CT_PRESENCE
} phhalPcr_BootReason_t;
#endif
