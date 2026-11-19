/* HAL ARM64 : UART PL011 (QEMU virt), compteur generique ARM, PSCI. Console serie uniquement. */
#include "kernel.h"
#include "hal.h"

#define UART_BASE 0x09000000UL
#define UARTDR (*(volatile u32 *)(UART_BASE + 0x00))
#define UARTFR (*(volatile u32 *)(UART_BASE + 0x18))

void hal_init(usize a0, usize a1) { (void)a0; (void)a1; }
void hal_putc(char c)
{
    if (c == '\n') hal_putc('\r');
    while (UARTFR & (1u << 5)) { }          /* TXFF */
    UARTDR = (u32)c;
}
char hal_getc(void)
{
    while (UARTFR & (1u << 4)) { }          /* RXFE */
    char c = (char)(UARTDR & 0xFF);
    return c == '\r' ? '\n' : c;
}
void hal_clear(void) { kputs("\033[2J\033[H"); }
void hal_color(int c) { kputs(c == 1 ? "\033[96m" : c == 2 ? "\033[92m" : "\033[0m"); }
int hal_set_layout(const char *n) { (void)n; return -1; }

u32 hal_uptime_ms(void)
{
    u64 cnt, frq;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(cnt));
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frq));
    if (!frq) return 0;
    return (u32)((cnt / frq) * 1000 + ((cnt % frq) * 1000) / frq);
}
u32 hal_ram_kb(void) { return 0; }          /* a lire depuis le device tree (a faire) */
const char *hal_arch(void) { return "ARM64 (aarch64)"; }
void hal_cpu_info(void)
{
    u64 midr;
    __asm__ volatile("mrs %0, midr_el1" : "=r"(midr));
    kputs("ARMv8-A, MIDR "); kputhex((u32)midr);
}

static void psci(u64 fn)
{
    register u64 x0 __asm__("x0") = fn;
    __asm__ volatile("hvc #0" : "+r"(x0) : : "x1", "x2", "x3", "memory");
}
void hal_reboot(void) { psci(0x84000009UL); for (;;) __asm__ volatile("wfe"); }
void hal_halt(void)   { psci(0x84000008UL); for (;;) __asm__ volatile("wfe"); }
