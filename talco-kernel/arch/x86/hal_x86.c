/* HAL x86 : VGA texte, serie COM1, clavier (scrutation), IDT + PIC + timer PIT 100 Hz. */
#include "kernel.h"
#include "hal.h"

static inline void outb(u16 p, u8 v) { __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(p)); }
static inline u8   inb(u16 p)        { u8 r; __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(p)); return r; }

/* ---- serie ---- */
static void serial_init(void)
{
    outb(0x3F9, 0x00); outb(0x3FB, 0x80); outb(0x3F8, 0x03); outb(0x3F9, 0x00);
    outb(0x3FB, 0x03); outb(0x3FA, 0xC7); outb(0x3FC, 0x0B);
}
static void serial_putc(char c)
{
    for (int i = 0; i < 100000 && !(inb(0x3FD) & 0x20); i++) { }
    outb(0x3F8, (u8)c);
}

/* ---- VGA ---- */
static volatile u16 *const vga = (volatile u16 *)0xB8000;
static int row, col;
static u8 attr = 0x0F;
static void cursor_update(void)
{
    u16 pos = (u16)(row * 80 + col);
    outb(0x3D4, 14); outb(0x3D5, (u8)(pos >> 8));
    outb(0x3D4, 15); outb(0x3D5, (u8)(pos & 0xFF));
}
static void scroll(void)
{
    for (int i = 0; i < 24 * 80; i++) vga[i] = vga[i + 80];
    for (int i = 24 * 80; i < 25 * 80; i++) vga[i] = (u16)(attr << 8) | ' ';
    row = 24;
}
void hal_clear(void)
{
    for (int i = 0; i < 25 * 80; i++) vga[i] = (u16)(attr << 8) | ' ';
    row = col = 0; cursor_update();
}
void hal_color(int c) { attr = c == 1 ? 0x0B : c == 2 ? 0x0A : 0x0F; }
void hal_putc(char c)
{
    if (c == '\n') serial_putc('\r');
    serial_putc(c);
    if (c == '\n') { col = 0; row++; }
    else if (c == '\b') { if (col > 0) { col--; vga[row * 80 + col] = (u16)(attr << 8) | ' '; } }
    else { vga[row * 80 + col] = (u16)(attr << 8) | (u8)c; if (++col == 80) { col = 0; row++; } }
    if (row >= 25) scroll();
    cursor_update();
}

/* ---- interruptions ---- */
struct idt_entry { u16 lo; u16 sel; u8 zero; u8 flags; u16 hi; } __attribute__((packed));
struct idt_ptr   { u16 limit; u32 base; } __attribute__((packed));
static struct idt_entry idt[256];
static volatile u32 ticks;
extern void irq0_stub(void);
extern void exc_stub(void);
void pit_tick(void) { ticks++; }

static void idt_set(int n, void (*h)(void), u32 cs)
{
    u32 a = (u32)h;
    idt[n].lo = (u16)(a & 0xFFFF); idt[n].sel = (u16)cs; idt[n].zero = 0;
    idt[n].flags = 0x8E; idt[n].hi = (u16)(a >> 16);
}

static u32 mem_kb;
void hal_init(usize magic, usize info)
{
    serial_init();
    if (magic == 0x2BADB002) { u32 *mbi = (u32 *)info; if (mbi[0] & 1) mem_kb = mbi[1] + mbi[2]; }

    /* PIC : IRQ 0-15 -> vecteurs 32-47, seule l'IRQ0 (timer) est active */
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0xFE); outb(0xA1, 0xFF);

    /* PIT : 100 Hz */
    outb(0x43, 0x36); outb(0x40, 11931 & 0xFF); outb(0x40, 11931 >> 8);

    u32 cs; __asm__ volatile("movl %%cs, %0" : "=r"(cs));
    for (int i = 0; i < 32; i++) idt_set(i, exc_stub, cs);
    idt_set(32, irq0_stub, cs);
    struct idt_ptr p = { sizeof idt - 1, (u32)idt };
    __asm__ volatile("lidt %0" : : "m"(p));
    __asm__ volatile("sti");
}
u32 hal_uptime_ms(void) { return ticks * 10; }
u32 hal_ram_kb(void) { return mem_kb; }
const char *hal_arch(void) { return "x86 (i386)"; }

void hal_cpu_info(void)
{
    u32 b, c, d;
    __asm__ volatile("cpuid" : "=b"(b), "=c"(c), "=d"(d) : "a"(0));
    u32 r[3] = { b, d, c };
    for (int i = 0; i < 12; i++) hal_putc((char)(r[i / 4] >> (8 * (i % 4))));
}

/* ---- clavier ---- */
static const char map_lo[] = {0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
static const char map_up[] = {0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '};
static int shift, caps, azerty;
static char azerty_fix(char c)
{
    switch (c) {
    case 'a': return 'q'; case 'q': return 'a'; case 'z': return 'w'; case 'w': return 'z';
    case 'A': return 'Q'; case 'Q': return 'A'; case 'Z': return 'W'; case 'W': return 'Z';
    case ';': return 'm'; case ':': return 'M'; case 'm': return ','; case 'M': return '?';
    }
    return c;
}
int hal_set_layout(const char *n)
{
    if (kstreq(n, "azerty")) { azerty = 1; return 0; }
    if (kstreq(n, "qwerty")) { azerty = 0; return 0; }
    return -1;
}
char hal_getc(void)
{
    for (;;) {
        while (!(inb(0x64) & 1)) __asm__ volatile("hlt");
        u8 sc = inb(0x60);
        if (sc == 0xE0) continue;
        if (sc == 0x2A || sc == 0x36) { shift = 1; continue; }
        if (sc == 0xAA || sc == 0xB6) { shift = 0; continue; }
        if (sc == 0x3A) { caps = !caps; continue; }
        if ((sc & 0x80) || sc >= sizeof map_lo) continue;
        char c = (shift ? map_up : map_lo)[sc];
        if (caps && c >= 'a' && c <= 'z') c = (char)(c - 32);
        if (azerty) c = azerty_fix(c);
        if (c) return c;
    }
}

void hal_reboot(void)
{
    while (inb(0x64) & 2) { }
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile("hlt");
}
void hal_halt(void) { for (;;) __asm__ volatile("cli; hlt"); }
