#include "kernel.h"
#include "hal.h"
static void put(const char *path, const char *txt) { fs_write(path, (const u8 *)txt, kstrlen(txt), 0); }

void kmain(usize a0, usize a1)
{
    hal_init(a0, a1);
    hal_clear();
    hal_color(1);
    kputs("  _____  _    _     ____ ___    ___  ____   _____\n");
    kputs(" |_   _|/ \\  | |   / ___/ _ \\  / _ \\/ ___| |___ /\n");
    kputs("   | | / _ \\ | |  | |  | | | || | | \\___ \\   |_ \\\n");
    kputs("   | |/ ___ \\| |__| |__| |_| || |_| |___) | ___) |\n");
    kputs("   |_/_/   \\_\\____|\\____\\___/  \\___/|____/ |____/\n");
    hal_color(0);
    kputs("\nTALCO-OS3 " TALCO_VERSION " sur "); kputs(hal_arch()); kputs("\n\n");

    fs_init();
    fs_mkdir("/etc"); fs_mkdir("/home"); fs_mkdir("/tmp");
    put("/etc/release", "TALCO-OS3 " TALCO_VERSION);
    put("/etc/arch", hal_arch());
    put("/home/bienvenue.txt", "Bienvenue dans TALCO-OS3. Tapez help.");
    kputs("[ok] tas : "); kputu((u32)(heap_total() / 1024)); kputs(" Ko\n");
    kputs("[ok] systeme de fichiers : "); kputu((u32)fs_count()); kputs(" noeuds\n");
    kputs("[ok] pret. Tapez help.\n\n");
    shell_run();
}
