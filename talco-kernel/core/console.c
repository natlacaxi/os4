#include "kernel.h"
#include "hal.h"
void kputc(char c) { hal_putc(c); }
void kputs(const char *s) { while (*s) hal_putc(*s++); }
void kputu(u32 n)
{
    char b[11]; int i = 10; b[i] = 0;
    do { b[--i] = (char)('0' + n % 10); n /= 10; } while (n);
    kputs(&b[i]);
}
void kputhex(u32 n)
{
    kputs("0x");
    for (int i = 28; i >= 0; i -= 4) hal_putc("0123456789ABCDEF"[(n >> i) & 0xF]);
}
