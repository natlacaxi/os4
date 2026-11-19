#ifndef HAL_H
#define HAL_H
#include "kernel.h"
/* Couche d'abstraction materielle : une implementation par architecture. */
void        hal_init(usize a0, usize a1);   /* a0/a1 : registres passes par le bootloader */
void        hal_putc(char c);
char        hal_getc(void);                 /* bloquant */
void        hal_clear(void);
void        hal_color(int c);               /* 0 normal, 1 cyan, 2 vert */
u32         hal_uptime_ms(void);
u32         hal_ram_kb(void);               /* 0 = inconnue */
void        hal_cpu_info(void);
int         hal_set_layout(const char *n);  /* 0 ok, -1 inconnu */
void        hal_reboot(void);
void        hal_halt(void);
const char *hal_arch(void);
#endif
