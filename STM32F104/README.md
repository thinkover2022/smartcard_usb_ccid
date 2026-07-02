# STM32F104 (chip SDK / driver layer) — PLACEHOLDER + skeleton

This folder is the STM32 counterpart of `PN7462AU/`. It holds the MCU-specific
drivers the NFC stack and app sit on. Most of it must be populated from
**STM32CubeF1** (download from ST); the NFC-specific glue is provided here as
skeletons.

## What YOU must add (from STM32CubeF1)
| Subfolder | Put here | Source in STM32CubeF1 |
|-----------|----------|-----------------------|
| `CMSIS/`  | `stm32f1xx.h`, `stm32f103xb.h`, `core_cm3.h`, `system_stm32f1xx.h` | `Drivers/CMSIS/...` |
| `HAL/`    | HAL (or LL) drivers you use: `stm32f1xx_hal.c`, `_rcc`, `_gpio`, `_spi`, `_tim`, `_pcd`(USB), + `stm32f1xx_hal_conf.h` | `Drivers/STM32F1xx_HAL_Driver/` |
| `startup/`| `startup_stm32f103xb.s`, `system_stm32f1xx.c` | `Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/` |
| `USB/`    | USB Device core + a **CCID class** implementation (ST USB Device Library, or TinyUSB). CCID class must be re-created (ST library ships CDC/HID/MSC, not CCID) | ST USB Device Lib / TinyUSB |

The Makefile already:
- compiles every `*.c` found under this folder (so once CMSIS/HAL/startup/USB
  `.c` files are here they are picked up automatically),
- assembles `startup/*.s`,
- adds `CMSIS/`, `HAL/`, `boards/` to the include path.

## What is already provided here (skeletons — implement the TODOs)
| File | Role |
|------|------|
| `phbalReg/phbalReg_Stm32Spi.c` | SPI bus (phbalReg.h) between STM32 and CLRC663 |
| `phDriver/phDriver_Stm32.c`     | GPIO/Timer/IRQ (phDriver.h) for RESET/IRQ/timeouts |
| `boards/Board_Stm32Rc663.h`     | pin map / SPI instance |

## Reuse hint
`NfcrdlibEx1_BasicDiscoveryLoop` (already in this repo) is the canonical
CLRC663 bring-up example — copy its `phApp_Init.c` flow into
`../STM32F104_ex_phExCcid/src/` and swap the board/BAL for these STM32 files.

See `../STM32F104_ex_phExCcid/PORTING.md` for the full plan.
