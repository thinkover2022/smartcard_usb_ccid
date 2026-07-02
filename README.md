# smartcard_usb_ccid

STM32F1 (Cortex-M3) firmware that turns an **external NXP CLRC663** NFC
front-end into a **USB-CCID contactless smart-card reader**. The board
enumerates over USB Full-Speed as a standard PC/SC reader, polls the RF field
with the NXP NfcRdLib discovery loop, and bridges card APDUs to the host through
the CCID / PC-SC protocol.

> Status: builds and links cleanly with the ARM GNU toolchain. Firmware
> footprint is ~73 KB Flash / ~8.6 KB RAM on an STM32F103xB (see
> [Memory footprint](#memory-footprint)).

---

## What it does

1. Brings up the CLRC663 over SPI (software NSS) and its IRQ line.
2. Runs the NfcRdLib **discovery loop** to detect a card in the field.
3. Classifies and activates the card (ISO14443-3/-4 L3/L4, FeliCa, 15693, …).
4. Exposes the card to the host as a **PC/SC smart-card slot** via USB-CCID, so
   any PC/SC application (e.g. `pcsc_scan`, OpenSC, WebUSB/PCSC middleware) can
   run APDU exchanges against the tag.

### Supported card technologies

Enabled through compile-time switches in
[`APP_NxpBuild.h`](STM32F104_ex_phExCcid/inc/APP_NxpBuild.h):

| Standard | Protocol layer | Application layer |
|---|---|---|
| ISO14443 Type A (3A / 4A, T=CL) | `phpalI14443p3a/p4/p4a` | MIFARE Classic, Ultralight, DESFire, ISO14443-4 |
| ISO14443 Type B (3B / 4B) | `phpalI14443p3b/p4` | ISO14443-4 exchange |
| FeliCa (Type F) | `phpalFelica` | `phalFelica` |
| ISO15693 / NFC Type V | `phpalSli15693` | ICODE (`phalICode`) |
| ISO18000-3 Mode 3 | `phpalI18000p3m3` | `phalI18000p3m3` |

---

## Hardware

- **MCU:** STM32F103xB (Cortex-M3, 128 KB Flash, 20 KB SRAM) — e.g. a "Blue Pill" board.
- **NFC front-end:** NXP CLRC663 (RC663) connected over SPI1.
- **Clocks:** 8 MHz HSE → PLL ×9 → 72 MHz SYSCLK; USB clock 48 MHz (72 / 1.5);
  SPI1 on APB2 @ 72 MHz.

### CLRC663 ↔ STM32F103 wiring

Defined in [`Board_Stm32Rc663.h`](STM32F104/boards/Board_Stm32Rc663.h):

| CLRC663 | STM32 pin | Function |
|---|---|---|
| RESET / PDOWN | **PB0** | reset / power-down (power-down = high) |
| IRQ | **PB1** | RF interrupt → EXTI1, rising-edge (IRQ active-high) |
| NSS / SSEL | **PA4** | SPI chip-select (software-driven) |
| SCK | **PA5** | SPI1 clock |
| MISO | **PA6** | SPI1 MISO |
| MOSI | **PA7** | SPI1 MOSI |

- SPI baud rate: **5 MHz** (`RC663_SPI_BAUD_HZ`, within the CLRC663 limit).
- Status LED: **PC13** (on-board Blue Pill LED, active-low).

### USB identity

Descriptors in [`usbd_desc.c`](STM32F104/USB/usbd_desc.c):

- VID `0x1FC9` (NXP) / PID `0x0117`
- Manufacturer: `NXP` · Product: `STM32 CLRC663 CCID Reader`
- Device class: USB-CCID (enumerates as a PC/SC reader)

---

## Software architecture

```
        ┌───────────────────────────────────────────────┐
  App   │  phExCcid framework  (STM32F104_ex_phExCcid)   │
        │  discovery • poll/classify • CCID / PC-SC engine│
        └───────────────┬───────────────────┬────────────┘
                        │                   │
        ┌───────────────▼──────┐   ┌────────▼───────────────┐
  Stack │  NXP NfcRdLib        │   │  USB Device + CCID class│
        │  DiscLoop, PAL, AL,  │   │  (STM32F104/USB)        │
        │  KeyStore, Crypto    │   └────────┬───────────────┘
        └───────┬──────────────┘            │
        ┌───────▼───────┐  ┌────────────────▼───────────────┐
  HAL   │  RC663 HAL    │  │  STM32Cube F1 HAL / CMSIS       │
        │  (phhalHw)    │  │  phDriver / phbalReg (SPI, GPIO)│
        └───────┬───────┘  └────────────────┬───────────────┘
        ┌───────▼──────────────────────────▼────────────────┐
  OS    │  FreeRTOS (Cortex-M3 port) + phOsal abstraction     │
        └────────────────────────────────────────────────────┘
```

- **Application** (`phExCcid_*`): ported from NXP's `PN7462AU_ex_phExCcid`
  example and retargeted to the CLRC663 + STM32. Handles the discovery loop,
  card polling/classification, per-technology L4 exchange, and the CCID/PC-SC
  command engine.
- **NXP NfcRdLib**: discovery loop, protocol abstraction layers (PAL),
  application layers (AL), key store, symmetric crypto and RNG.
- **RC663 HAL** (`phhalHw/Rc663`): CLRC663 front-end driver.
- **DAL / phDriver / phbalReg**: the CLRC663 HAL talks to the MCU through the
  DAL interface (`phDriver.h`, `phbalReg.h`); the STM32 implementation lives in
  `STM32F104/phDriver` and `STM32F104/phbalReg`.
- **FreeRTOS + phOsal**: single reader task on the FreeRTOS Cortex-M3 port
  (`heap_3` allocator, i.e. newlib malloc/free); `phOsal` provides the RTOS
  abstraction the stack uses.
- **USB**: STM32Cube USB Device core plus a CCID class implementation that feeds
  the ported CCID engine (`CCID_DataOut → phExCcid_UsbCcid_Usb_Bulk_Out`, etc.).

### Runtime flow (`main.c`)

```
main() → HAL_Init → SystemClock_Config(72 MHz) → LED → phOsal_Init
       → Usb_Device_Init (CCID)  → create "Reader" task → start scheduler

Reader_Task:
   phExCcid_Clif_HalInit()            # CLRC663 SPI BAL + RC663 HAL
   phExCcidClif_DiscLoopConfig/PalInit
   loop: phExCcid_ClifMain(POLL) → phExCcid_Poll_Main   # detect→activate→CCID
```

The CLRC663 IRQ (PB1/EXTI1) is forwarded to the HAL RF ISR through
`CLIF_IRQHandler()`.

---

## Repository layout

| Path | Contents |
|---|---|
| `STM32F104_ex_phExCcid/` | The application: `src/`, `inc/`, and the standalone `build/` (Makefile, linker script, `syscalls.c`, `compat/`). |
| `STM32F104/` | STM32 chip SDK: `CMSIS/`, `HAL/`, `USB/` (incl. the CCID class), `startup/`, `phDriver/`, `phbalReg/`, `boards/`. |
| `NxpNfcRdLib/` | NXP Reader Library (`comps/`, `intfs/`, `types/`) — trimmed to the RC663 target. |
| `FreeRTOS/` | FreeRTOS kernel + Cortex-M3 port. |
| `phOsal/` | OS abstraction layer (FreeRTOS backend). |
| `DAL/` | Driver Abstraction Layer interface headers (`inc/`, `cfg/BoardSelection.h`). |

The tree is trimmed to the CLRC663 / Cortex-M3 target: other reader-IC HALs
(PN7462AU, PN5180), non-CM3 RTOS ports, and non-STM32 DAL/OSAL platform sources
have been removed.

---

## Building

Requires the **ARM GNU toolchain** (`arm-none-eabi-gcc`, with newlib-nano) and
GNU Make on `PATH`.

```sh
cd STM32F104_ex_phExCcid/build
make            # build -> STM32F104_ex_phExCcid.elf/.bin/.hex (+ size report)
make V=1        # same, but print full compiler/linker command lines
make srcs       # list the collected source files and counts
make clean      # remove obj/ and the .elf/.bin/.hex/.map outputs
```

Cross-compiler prefix is overridable: `make CROSS=arm-none-eabi-`.

**Portable across shells.** The Makefile runs from a POSIX shell
(WSL / MSYS2 / Git-Bash) *or* a Windows `cmd.exe` / PowerShell prompt — on
Windows it routes housekeeping recipes through `cmd.exe`, so no Unix coreutils
need to be on `PATH`.

Key build settings:

- `-mcpu=cortex-m3 -mthumb`, `-Os -g3`, `-std=gnu99`, `--specs=nano.specs`
- `-ffunction-sections -fdata-sections -Wl,--gc-sections` (dead-code strip)
- Feature switches force-included from `APP_NxpBuild.h`; a small `compat/` shim
  is force-included for `<stdbool.h>` handling.
- Linker script: [`STM32F103xB.ld`](STM32F104_ex_phExCcid/build/STM32F103xB.ld)
  (128 KB Flash @ `0x08000000`, 20 KB RAM @ `0x20000000`).

### Flashing

Flash `STM32F104_ex_phExCcid.hex` (or `.bin` at `0x08000000`) with your usual
tool, e.g.:

```sh
# ST-LINK
st-flash --format ihex write STM32F104_ex_phExCcid.hex
# or OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
        -c "program STM32F104_ex_phExCcid.elf verify reset exit"
```

---

## Memory footprint

Latest build (`-Os`, newlib-nano):

| Region | Used | Size | % |
|---|---|---|---|
| FLASH | 73 248 B | 128 KB | 55.9 % |
| RAM | 8 592 B | 20 KB | 42.0 % |

`text 72904 · data 340 · bss 8260` (≈ 81.5 KB total).

---

## Configuration

- **Enabled protocols / card types:** edit the `NXPBUILD__*` switches in
  [`APP_NxpBuild.h`](STM32F104_ex_phExCcid/inc/APP_NxpBuild.h).
- **Pin map, SPI speed, IRQ/reset polarity:** edit
  [`Board_Stm32Rc663.h`](STM32F104/boards/Board_Stm32Rc663.h). The board is
  selected by `-DPHDRIVER_STM32RC663_BOARD` (in the Makefile `DEFS`), which
  `DAL/cfg/BoardSelection.h` maps to that header.
- **Clock tree:** `SystemClock_Config()` in
  [`main.c`](STM32F104_ex_phExCcid/src/main.c).

---

## Credits & licensing

This project bundles third-party components, each under its own license:

- **NXP NfcRdLib** and the `phExCcid` example framework — © NXP Semiconductors.
- **STM32Cube F1** HAL / CMSIS / USB Device library — © STMicroelectronics.
- **FreeRTOS** kernel — © Amazon.com, Inc. / Richard Barry (MIT).

Refer to the license headers in the respective source folders for terms.
