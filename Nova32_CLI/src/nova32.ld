/* Project NOVA-32 Phase 1 Linker Script — Maps Flash, DTCM-RAM (0-wait), and AXI-SRAM */
ENTRY(Reset_Handler)

MEMORY {
  FLASH   (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K  /* Non-volatile Kernel & Vector ROM */
  DTCMRAM (xrw) : ORIGIN = 0x20000000, LENGTH = 128K   /* Zero-Wait-State Core Stack & Data */
  AXISRAM (xrw) : ORIGIN = 0x24000000, LENGTH = 512K   /* Primary Kernel Heap Pool (D1)     */
}

_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);           /* Top of 128KB DTCM = 0x20020000    */
_heap_start = ORIGIN(AXISRAM);                         /* Start of 512KB Heap = 0x24000000  */
_heap_end   = ORIGIN(AXISRAM) + LENGTH(AXISRAM);       /* End of 512KB Heap   = 0x24080000  */

SECTIONS {
  .isr_vector : {
    . = ALIGN(4); KEEP(*(.isr_vector)) . = ALIGN(4);
  } > FLASH

  .text : {
    . = ALIGN(4); *(.text*) *(.rodata*) . = ALIGN(4);
    _etext = .;
  } > FLASH

  _sidata = LOADADDR(.data);
  .data : {
    . = ALIGN(4); _sdata = .; *(.data*) . = ALIGN(4); _edata = .;
  } > DTCMRAM AT> FLASH

  .bss : {
    . = ALIGN(4); _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .;
  } > DTCMRAM
}