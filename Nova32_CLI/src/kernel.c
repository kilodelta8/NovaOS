#include <stdint.h>

extern void hal_uart_init(uint32_t clk, uint32_t baud);
extern void uart_putc(char c);
extern void uart_puts(const char *s);
extern int  uart_getc_nonblock(char *out_ch);
extern void uart_puthex32(uint32_t val);
extern uint32_t _heap_start, _heap_end;

#define SYST_CSR           (*(volatile uint32_t *)0xE000E010UL)
#define SYST_RVR           (*(volatile uint32_t *)0xE000E014UL)
#define SYST_CVR           (*(volatile uint32_t *)0xE000E018UL)
#define GPIOB_ODR          (*(volatile uint32_t *)0x58020414UL)
#define SYS_CLOCK_HZ       64000000UL

static volatile uint32_t g_systick_ms = 0U;
static uint8_t          *g_heap_ptr   = (uint8_t *)&_heap_start;

void SysTick_Handler(void) {
    g_systick_ms++;
    if ((g_systick_ms % 500U) == 0U) { GPIOB_ODR ^= (1UL << 0); } /* 1Hz Heartbeat on PB0 */
}

void *kalloc(uint32_t bytes) {
    bytes = (bytes + 7U) & ~7U; /* Enforce 8-byte AAPCS alignment in AXI-SRAM */
    if ((g_heap_ptr + bytes) > (uint8_t *)&_heap_end) return (void *)0;
    void *block = g_heap_ptr;
    g_heap_ptr += bytes;
    return block;
}

/* Minimal Bare-Metal Standard Library Helpers (No libc dependency) */
int kstrcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

uint32_t kstrtoul_hex(const char *s) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    uint32_t val = 0U;
    while (*s) {
        char c = *s++;
        uint32_t d = (c >= '0' && c <= '9') ? (c - '0') :
                     (c >= 'a' && c <= 'f') ? (c - 'a' + 10) :
                     (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : 0xFFU;
        if (d == 0xFFU) break;
        val = (val << 4) | d;
    }
    return val;
}

void uart_putdec(uint32_t n) {
    char buf[12]; int i = 0;
    if (n == 0U) { uart_putc('0'); return; }
    while (n > 0U) { buf[i++] = (char)('0' + (n % 10U)); n /= 10U; }
    while (i > 0)  { uart_putc(buf[--i]); }
}

static void cli_execute(char *line) {
    char *argv[8]; int argc = 0;
    while (*line && argc < 8) {
        while (*line == ' ') *line++ = '\0';
        if (!*line) break;
        argv[argc++] = line;
        while (*line && *line != ' ') line++;
    }
    if (argc == 0) return;

    if (kstrcmp(argv[0], "help") == 0) {
        uart_puts("Commands: help | sysinfo | uptime | clear | peek <addr> | poke <addr> <val> | hexdump <addr> | alloc <bytes> | reboot\n");
    } else if (kstrcmp(argv[0], "sysinfo") == 0) {
        uart_puts("CPU: STM32H743ZIT6 Cortex-M7 | CPUID: "); uart_puthex32(*(volatile uint32_t *)0xE000ED00UL);
        uart_puts("\nAXI-SRAM Heap Used: "); uart_putdec((uint32_t)(g_heap_ptr - (uint8_t *)&_heap_start));
        uart_puts(" / 524288 bytes\n");
    } else if (kstrcmp(argv[0], "uptime") == 0) {
        uart_puts("Uptime: "); uart_putdec(g_systick_ms / 1000U); uart_putc('.'); uart_putdec(g_systick_ms % 1000U); uart_puts(" sec\n");
    } else if (kstrcmp(argv[0], "clear") == 0) {
        uart_puts("\033[2J\033[H");
    } else if (kstrcmp(argv[0], "peek") == 0 && argc >= 2) {
        uint32_t addr = kstrtoul_hex(argv[1]);
        uart_puthex32(addr); uart_puts(" = "); uart_puthex32(*(volatile uint32_t *)addr); uart_putc('\n');
    } else if (kstrcmp(argv[0], "poke") == 0 && argc >= 3) {
        *(volatile uint32_t *)kstrtoul_hex(argv[1]) = kstrtoul_hex(argv[2]);
        uart_puts("OK\n");
    } else if (kstrcmp(argv[0], "hexdump") == 0 && argc >= 2) {
        uint32_t base = kstrtoul_hex(argv[1]);
        for (uint32_t i = 0; i < 16U; i += 4U) {
            uart_puthex32(base + i); uart_puts(": "); uart_puthex32(*(volatile uint32_t *)(base + i)); uart_putc('\n');
        }
    } else if (kstrcmp(argv[0], "alloc") == 0 && argc >= 2) {
        void *p = kalloc(kstrtoul_hex(argv[1]));
        uart_puts("Allocated Block @ "); uart_puthex32((uint32_t)p); uart_putc('\n');
    } else if (kstrcmp(argv[0], "reboot") == 0) {
        *(volatile uint32_t *)0xE000ED0CUL = 0x05FA0004UL; /* SCB->AIRCR SYSRESETREQ */
    } else { uart_puts("Unknown command. Type 'help'.\n"); }
}

void kernel_main(void) {
    SYST_RVR = (SYS_CLOCK_HZ / 1000UL) - 1UL; /* 1ms OS Tick */
    SYST_CVR = 0UL; SYST_CSR = 0x07UL;        /* CLKSOURCE=CPU, TICKINT=1, ENABLE=1 */
    hal_uart_init(SYS_CLOCK_HZ, 115200UL);
    uart_puts("\033[2J\033[H=== PROJECT NOVA-32 | NovaOS v0.1 (Bare-Metal CLI Kernel) ===\nnova32> ");

    char cmd_buf[80]; uint32_t pos = 0U; char c;
    while (1) {
        if (!uart_getc_nonblock(&c)) { __asm__ volatile("wfi"); continue; } /* Sleep until IRQ */
        if (c == '\r' || c == '\n') {
            uart_puts("\n"); cmd_buf[pos] = '\0'; cli_execute(cmd_buf); pos = 0U; uart_puts("nova32> ");
        } else if ((c == 0x08 || c == 0x7F) && pos > 0U) {
            pos--; uart_puts("\b \b");
        } else if (c >= ' ' && pos < 78U) { cmd_buf[pos++] = c; uart_putc(c); }
    }
}
