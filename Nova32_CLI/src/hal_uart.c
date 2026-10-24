#include <stdint.h>

/* Memory-Mapped I/O (MMIO) Physical Register Definitions for STM32H743 */
#define REG32(addr)        (*(volatile uint32_t *)(addr))
#define RCC_AHB4ENR        REG32(0x580244E0UL)
#define RCC_APB1LENR       REG32(0x580244E8UL)
#define GPIOB_MODER        REG32(0x58020400UL)
#define GPIOB_ODR          REG32(0x58020414UL)
#define GPIOD_MODER        REG32(0x58020C00UL)
#define GPIOD_AFRH         REG32(0x58020C24UL)
#define USART3_CR1         REG32(0x40004800UL)
#define USART3_BRR         REG32(0x4000480CUL)
#define USART3_ISR         REG32(0x4000481CUL)
#define USART3_ICR         REG32(0x40004820UL)
#define USART3_RDR         REG32(0x40004824UL)
#define USART3_TDR         REG32(0x40004828UL)
#define NVIC_ISER1         REG32(0xE000E104UL)

#define RX_RING_SIZE       256U
static volatile char     g_rx_ring[RX_RING_SIZE];
static volatile uint32_t g_rx_head = 0U, g_rx_tail = 0U;

void hal_uart_init(uint32_t apb1_clk_hz, uint32_t baud) {
    /* 1. Enable GPIOB (Heartbeat LED), GPIOD (UART Pins), and USART3 Peripheral Clocks */
    RCC_AHB4ENR  |= (1UL << 1) | (1UL << 3);
    RCC_APB1LENR |= (1UL << 18);
    (void)RCC_APB1LENR; /* Read-back pipeline flush */

    /* 2. Configure PB0 as Output (Status LED) and PD8/PD9 as Alternate Function AF7 */
    GPIOB_MODER = (GPIOB_MODER & ~(3UL << 0)) | (1UL << 0);
    GPIOD_MODER = (GPIOD_MODER & ~(0xFUL << 16)) | (0xAUL << 16);
    GPIOD_AFRH  = (GPIOD_AFRH  & ~(0xFFUL << 0)) | (0x77UL << 0);

    /* 3. Configure USART3: Disable UE, set BRR, enable RXNEIE (Bit 5), TE (Bit 3), RE (Bit 2), UE (Bit 0) */
    USART3_CR1 = 0U;
    USART3_BRR = (apb1_clk_hz + (baud / 2U)) / baud;
    USART3_CR1 = (1UL << 5) | (1UL << 3) | (1UL << 2) | (1UL << 0);

    /* 4. Unmask USART3 Global Interrupt (IRQ 39 -> ISER1 Bit 7) in Cortex-M7 NVIC */
    NVIC_ISER1 = (1UL << (39U - 32U));
}

void USART3_IRQHandler(void) {
    if (USART3_ISR & (1UL << 5)) { /* RXNE: Receive Data Register Not Empty */
        char c = (char)(USART3_RDR & 0xFFU);
        uint32_t next = (g_rx_head + 1U) & (RX_RING_SIZE - 1U);
        if (next != g_rx_tail) { g_rx_ring[g_rx_head] = c; g_rx_head = next; }
    }
    USART3_ICR = 0x0FUL; /* Clear any framing/overrun error flags */
}

void uart_putc(char c) {
    while (!(USART3_ISR & (1UL << 7))); /* Wait for TXE (Transmit Data Register Empty) */
    USART3_TDR = (uint8_t)c;
}

void uart_puts(const char *s) {
    while (*s) { if (*s == '\n') uart_putc('\r'); uart_putc(*s++); }
}

int uart_getc_nonblock(char *out_ch) {
    if (g_rx_head == g_rx_tail) return 0;
    *out_ch = g_rx_ring[g_rx_tail];
    g_rx_tail = (g_rx_tail + 1U) & (RX_RING_SIZE - 1U);
    return 1;
}

void uart_puthex32(uint32_t val) {
    static const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4) uart_putc(hex[(val >> i) & 0xFU]);
}
