#include <string.h>
#include "hal.h"
char out[8192]; static int n;
void out_reset(void) { n = 0; out[0] = 0; }
void hal_init(usize a, usize b) { (void)a; (void)b; }
void hal_putc(char c) { if (n < 8190) { out[n++] = c; out[n] = 0; } }
char hal_getc(void) { return 0; }
void hal_clear(void) {}
void hal_color(int c) { (void)c; }
u32 hal_uptime_ms(void) { return 12345; }
u32 hal_ram_kb(void) { return 262144; }
void hal_cpu_info(void) { kputs("host"); }
int hal_set_layout(const char *x) { return kstreq(x, "azerty") ? 0 : -1; }
void hal_reboot(void) {}
void hal_halt(void) {}
const char *hal_arch(void) { return "host"; }
