#include <stdint.h>

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
extern void kernel_main(void);
extern void SysTick_Handler(void);
extern void USART3_IRQHandler(void);

void Default_Handler(void) { while (1) { __asm__ volatile("nop"); } }

void Reset_Handler(void) {
    /* 1. Copy .data section from Flash LMA (_sidata) to DTCM-RAM VMA (_sdata.._edata) */
    uint32_t *src = &_sidata, *dst = &_sdata;
    while (dst < &_edata) { *dst++ = *src++; }

    /* 2. Zero-initialize the .bss section in DTCM-RAM (_sbss.._ebss) */
    dst = &_sbss;
    while (dst < &_ebss) { *dst++ = 0U; }

    /* 3. Enable Cortex-M7 Hardware FPU (CP10 and CP11 Full Access in SCB->CPACR) */
    *(volatile uint32_t *)0xE000ED88UL |= (0xFUL << 20);
    __asm__ volatile("dsb\n isb");

    /* 4. Enter NovaOS v0.1 Kernel Main */
    kernel_main();
    while (1);
}

/* Cortex-M7 Exception & Interrupt Vector Table pinned at 0x08000000 */
__attribute__((section(".isr_vector"), used))
void (* const g_pfnVectors[16 + 40])(void) = {
    (void (*)(void))(&_estack),      /* 0x00: Initial Main Stack Pointer (MSP) */
    Reset_Handler,                   /* 0x04: Reset Handler                    */
    Default_Handler, Default_Handler, Default_Handler, Default_Handler, Default_Handler, 0,
    0, 0, 0, Default_Handler, Default_Handler, 0, Default_Handler,
    SysTick_Handler,                 /* 0x3C: Exception 15 — OS System Tick    */
    [16 ... 54] = Default_Handler,   /* External IRQs 0..38 Default            */
    [16 + 39]   = USART3_IRQHandler  /* External IRQ 39 — USART3 Console RX    */
};