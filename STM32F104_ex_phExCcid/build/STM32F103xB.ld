/* ============================================================================
 *  STM32F103xB.ld  -  standard ST-style GNU linker script for STM32F103xB
 *  (Cortex-M3, 128 KB Flash @0x08000000, 20 KB SRAM @0x20000000).
 *
 *  Uses the conventional ST symbols consumed by ST's startup_stm32f103xb.s
 *  Reset_Handler (_sidata/_sdata/_edata/_sbss/_ebss/_estack). Unlike the PN7462
 *  build, .data/.bss init and the call to main() are done by ST startup, so we
 *  do NOT use -nostartfiles here.
 *
 *  Adjust FLASH/RAM LENGTH to the exact part (C8=64K/20K, RB=128K/20K, ...).
 * ==========================================================================*/

ENTRY(Reset_Handler)

_estack = 0x20005000;      /* top of 20 KB RAM (0x20000000 + 0x5000) */
_Min_Heap_Size  = 0x800;   /* required heap  (malloc / FreeRTOS heap_3) */
_Min_Stack_Size = 0x600;   /* required MSP stack */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
    .isr_vector : { . = ALIGN(4); KEEP(*(.isr_vector)) . = ALIGN(4); } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text) *(.text*)
        *(.glue_7) *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init)) KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata : { . = ALIGN(4); *(.rodata) *(.rodata*) . = ALIGN(4); } > FLASH
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } > FLASH

    .preinit_array : { PROVIDE_HIDDEN(__preinit_array_start = .); KEEP(*(.preinit_array*)) PROVIDE_HIDDEN(__preinit_array_end = .); } > FLASH
    .init_array    : { PROVIDE_HIDDEN(__init_array_start = .); KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array*)) PROVIDE_HIDDEN(__init_array_end = .); } > FLASH
    .fini_array    : { PROVIDE_HIDDEN(__fini_array_start = .); KEEP(*(SORT(.fini_array.*))) KEEP(*(.fini_array*)) PROVIDE_HIDDEN(__fini_array_end = .); } > FLASH

    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data) *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss) *(.bss*) *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /DISCARD/ : { libc.a(*) libm.a(*) libgcc.a(*) }
    .ARM.attributes 0 : { *(.ARM.attributes) }
}
