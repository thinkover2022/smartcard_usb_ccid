# ============================================================================
#  Standalone GNU Make build for STM32F104_ex_phExCcid
#  (STM32F1 / Cortex-M3  +  external CLRC663 over SPI, USB-CCID reader)
#
#  Toolchain : ARM GNU Toolchain (arm-none-eabi-gcc) + newlib-nano
#  Run from  : <repo>/STM32F104_ex_phExCcid/build
#              Works from a POSIX shell (WSL/MSYS2/Git-Bash) or a Windows
#              cmd.exe / PowerShell prompt — on Windows recipes route through
#              cmd.exe so no Unix coreutils on PATH are required.
#
#  STATUS: BUILDS. STM32CubeF1 sources (CMSIS/HAL/startup/USB) are present
#  under ../../STM32F104 and `make` links to STM32F104_ex_phExCcid.elf/.bin/.hex.
#  `make srcs` lists the collected sources
#  (NxpNfcRdLib upper stack + RC663 HAL + FreeRTOS CM3 + phOsal).
#
#  This mirrors the PN7462AU_ex_phExCcid Makefile but retargeted:
#    - cortex-m3 (was cortex-m0)
#    - external CLRC663 HAL kept; PN7462AU/Pn5180 HALs and the whole PN7462AU
#      SDK + prebuilt libs (libphhalRf/libusbd) dropped
#    - ST startup + standard ST linker script (NOT the PN7462 ZIBSS scheme)
# ============================================================================

CROSS   ?= arm-none-eabi-
CC       = $(CROSS)gcc
OBJCOPY  = $(CROSS)objcopy
SIZE     = $(CROSS)size

# ---- host shell selection -------------------------------------------------
# On Windows, force every recipe through cmd.exe. A make launched from cmd or
# PowerShell can otherwise pick up a stray sh.exe (e.g. PortableGit) as SHELL
# while the POSIX coreutils (rm/mkdir/printf) are NOT on PATH; make's direct-
# exec fast path then fails to find `rm`/`echo`. Routing through cmd.exe uses
# its built-ins (del/rmdir/mkdir/echo) and sidesteps that entirely. The compile
# tools are real .exe on PATH, so they run fine under cmd too. On real POSIX
# hosts (Linux/WSL) we keep sh + coreutils.
ifeq ($(OS),Windows_NT)
  WIN := 1
  SHELL := cmd.exe
  .SHELLFLAGS := /c
else
  WIN :=
endif

TARGET  := STM32F104_ex_phExCcid

# ---- paths (this Makefile lives in <repo>/STM32F104_ex_phExCcid/build) ----
ROOT    := ../..
APP     := $(ROOT)/STM32F104_ex_phExCcid
# chip SDK (CMSIS/HAL/startup/USB/phDriver/phbalReg)
SDK     := $(ROOT)/STM32F104
OBJDIR  := obj

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# ---- source roots ----
SRC_ROOTS := \
    $(APP)/src \
    $(SDK) \
    $(ROOT)/NxpNfcRdLib \
    $(ROOT)/FreeRTOS \
    $(ROOT)/phOsal \
    $(ROOT)/DAL

ALL_C := $(foreach r,$(SRC_ROOTS),$(call rwildcard,$r,*.c))
# ST startup is assembly (startup_stm32f103xb.s) — provided by STM32Cube.
ALL_S := $(foreach r,$(SDK),$(call rwildcard,$r,*.s))

# ---- exclusions (path fragments) ----
# Keep: NxpNfcRdLib upper stack + phhalHw/src/Rc663.
# Drop: other reader-IC HALs, PN7462 bits, non-CM3 RTOS ports, non-STM32 DAL/OSAL.
# Only fragments that match a real path in this trimmed tree are listed. The
# other-platform/other-chip source dirs (phhalHw/{PN7462AU,Pn5180}, phOsal/src/
# {Linux,NullOs}, FreeRTOS non-CM3 ports, DAL/src) were physically removed from
# the tree, so their exclusions are gone with them. What remains here are files
# that still exist alongside the ones we build but must not be compiled.
EXCL := \
    /NxpNfcRdLib/types/ \
    /FreeRTOS/heap_2.c /FreeRTOS/heap_4.c \
    _template.c

is_excluded = $(strip $(foreach e,$(EXCL),$(findstring $e,$1)))
SRCS := $(strip $(foreach f,$(ALL_C),$(if $(call is_excluded,$f),,$f)))
SRCS += syscalls.c
ASRC := $(ALL_S)

# ---- include dirs ----
# Curated first (so any ambiguous header resolves to the intended one), then the
# broad auto-discovered set as fallback.
HDR_ROOTS := $(APP)/inc $(SDK) $(ROOT)/NxpNfcRdLib $(ROOT)/FreeRTOS $(ROOT)/phOsal $(ROOT)/DAL
CURATED := compat $(APP)/inc \
    $(ROOT)/NxpNfcRdLib/intfs \
    $(SDK)/boards $(SDK)/CMSIS $(SDK)/HAL \
    $(SDK)/USB $(SDK)/USB/Core/Inc \
    $(ROOT)/DAL/inc $(ROOT)/DAL/cfg \
    $(ROOT)/FreeRTOS/include $(ROOT)/FreeRTOS/portable/GCC/ARM_CM3 \
    $(ROOT)/phOsal/inc
BROAD    := $(sort $(dir $(foreach r,$(HDR_ROOTS),$(call rwildcard,$r,*.h))))
INC_DIRS := $(CURATED) $(BROAD)

# ---- flags ----
ARCH := -mcpu=cortex-m3 -mthumb
# NXPBUILD__* switches come from the force-included APP_NxpBuild.h.
# STM32F103xB selects the CMSIS device header; USE_HAL_DRIVER for STM32 HAL.
DEFS := -DPH_OSAL_FREERTOS -DPHFL_APP_NXBUILD_CONFIG=1 -DSTM32F103xB -DUSE_HAL_DRIVER \
        -DPHDRIVER_STM32RC663_BOARD -DCORE_M3 -DNDEBUG=1
OPT  := -Os -g3
WARN := -Wno-unknown-pragmas -Wno-strict-aliasing
FORCEINC := -include compat/ph_bool_fix.h -include APP_NxpBuild.h

CFLAGS := $(ARCH) $(DEFS) $(OPT) $(WARN) $(FORCEINC) \
          -ffunction-sections -fdata-sections -fno-builtin -fmessage-length=0 \
          -std=gnu99 -MMD -MP $(addprefix -I,$(INC_DIRS))
ASFLAGS := $(ARCH) -x assembler-with-cpp -MMD -MP

LDSCRIPT := STM32F103xB.ld
# No prebuilt libs on STM32 (RC663 HAL + STM32 HAL are all source).
LDFLAGS  := $(ARCH) -T $(LDSCRIPT) --specs=nano.specs \
            -Wl,--gc-sections -Wl,-Map=$(TARGET).map -Wl,--print-memory-usage \
            -Wl,--no-warn-rwx-segments

# ---- verbosity ----
# Default prints one short line per file (CC/AS/LD ...); `make V=1` shows the
# full compiler/linker command lines instead.
V ?= 0
ifeq ($(V),0)
  Q := @
  E := @echo
else
  Q :=
  E := @true
endif

# ---- objects (flatten path -> unique name) ----
flat  = $(subst /,_,$(patsubst $(ROOT)/%,%,$1))
OBJS := $(foreach s,$(SRCS),$(OBJDIR)/$(call flat,$s).o) \
        $(foreach s,$(ASRC),$(OBJDIR)/$(call flat,$s).o)

# ============================================================================
.PHONY: all size srcs clean
all: $(TARGET).elf $(TARGET).bin $(TARGET).hex size

define CC_RULE
$(OBJDIR)/$(call flat,$1).o: $1 | $(OBJDIR)
	$$(E) "  CC   $1"
	$$(Q)$$(CC) $$(CFLAGS) -c $$< -o $$@
endef
$(foreach s,$(SRCS),$(eval $(call CC_RULE,$s)))

define AS_RULE
$(OBJDIR)/$(call flat,$1).o: $1 | $(OBJDIR)
	$$(E) "  AS   $1"
	$$(Q)$$(CC) $$(ASFLAGS) -c $$< -o $$@
endef
$(foreach s,$(ASRC),$(eval $(call AS_RULE,$s)))

$(OBJDIR):
ifeq ($(WIN),1)
	$(Q)if not exist "$(OBJDIR)" mkdir "$(OBJDIR)"
else
	$(Q)mkdir -p $(OBJDIR)
endif

$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(E) "  LD   $@"
	$(Q)$(CC) $(LDFLAGS) $(OBJS) -o $@

%.bin: %.elf
	$(E) "  BIN  $@"
	$(Q)$(OBJCOPY) -O binary $< $@
%.hex: %.elf
	$(E) "  HEX  $@"
	$(Q)$(OBJCOPY) -O ihex $< $@

size: $(TARGET).elf
	$(SIZE) $<

srcs:
	@echo C sources : $(words $(SRCS))
	@echo ASM       : $(words $(ASRC))
	@echo inc dirs  : $(words $(INC_DIRS))
ifeq ($(WIN),1)
	$(Q)for %%f in ($(SRCS)) do @echo   %%f
else
	@printf '  %s\n' $(SRCS)
endif

clean:
ifeq ($(WIN),1)
	$(Q)if exist "$(OBJDIR)" rmdir /s /q "$(OBJDIR)"
	-$(Q)del /q /f "$(TARGET).elf" "$(TARGET).bin" "$(TARGET).hex" "$(TARGET).map" 2>nul
else
	$(Q)rm -rf $(OBJDIR) $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map
endif

-include $(wildcard $(OBJDIR)/*.d)
