#include "kernel.h"
#include "hal.h"

static const char *layers[] = {
    "00_FOUNDATION", "01_KERNEL", "02_TALCO_CORE", "03_DEVICE_ENGINE", "04_DRIVERS",
    "05_TALCO_EXPERIENCE", "06_APPLICATION_FRAMEWORK", "07_APPLICATIONS", "08_TALCO_STORE",
    "09_SECURITY", "10_INTELLIGENCE", "11_CONNECTIVITY", "12_ENERGY", "13_UPDATE",
    "14_CLOUD", "15_DEVELOPER_PLATFORM", "16_DEVICE_SUPPORT", "17_BUILD_SYSTEM",
    "18_QUALITY_ASSURANCE", "19_DESIGN_SYSTEM", "20_INFRASTRUCTURE", "21_DOCUMENTATION"
};

static void err(int r)
{
    switch (r) {
    case FS_ENOENT: kputs("introuvable\n"); break;
    case FS_EEXIST: kputs("existe deja\n"); break;
    case FS_EFULL: kputs("plus de place\n"); break;
    case FS_EISDIR: kputs("est un dossier\n"); break;
    case FS_ENOTDIR: kputs("n'est pas un dossier\n"); break;
    case FS_ENOTEMPTY: kputs("dossier non vide\n"); break;
    default: kputs("argument invalide\n");
    }
}
static void ls_cb(const char *name, int is_dir, usize size)
{
    kputs(name);
    if (is_dir) kputc('/'); else { kputs("  ("); kputu((u32)size); kputs(" o)"); }
    kputc('\n');
}
static void help(void)
{
    kputs("Systeme : help version clear about arch cpu mem uptime layers kbd reboot halt\n");
    kputs("Fichiers: ls [d] cd d pwd mkdir d touch f cat f write f txt append f txt rm f df\n");
    kputs("Divers  : echo txt\n");
}

void shell_exec(char *line)
{
    char *arg = line;
    while (*arg && *arg != ' ') arg++;
    if (*arg) { *arg++ = 0; while (*arg == ' ') arg++; }
    char *arg2 = arg;                          /* 2e argument (reste de la ligne) */
    while (*arg2 && *arg2 != ' ') arg2++;
    if (*arg2) { *arg2++ = 0; while (*arg2 == ' ') arg2++; }

    if (!*line) return;
    if (kstreq(line, "help")) help();
    else if (kstreq(line, "version")) { kputs("TALCO-OS3 " TALCO_VERSION "\n"); }
    else if (kstreq(line, "clear")) hal_clear();
    else if (kstreq(line, "about")) kputs("TALCO-OS3 : noyau bare-metal (x86 et ARM64), 22 couches.\n");
    else if (kstreq(line, "arch")) { kputs(hal_arch()); kputc('\n'); }
    else if (kstreq(line, "cpu")) { hal_cpu_info(); kputc('\n'); }
    else if (kstreq(line, "mem")) {
        u32 ram = hal_ram_kb();
        kputs("RAM : "); if (ram) { kputu(ram / 1024); kputs(" Mo"); } else kputs("inconnue");
        kputs("\nTas : "); kputu((u32)(heap_used() / 1024)); kputs(" / ");
        kputu((u32)(heap_total() / 1024)); kputs(" Ko\n");
    }
    else if (kstreq(line, "uptime")) {
        u32 ms = hal_uptime_ms();
        kputu(ms / 1000); kputc('.'); kputu((ms % 1000) / 100); kputs(" s\n");
    }
    else if (kstreq(line, "layers")) {
        for (u32 i = 0; i < sizeof layers / sizeof *layers; i++) { kputs(layers[i]); kputc('\n'); }
    }
    else if (kstreq(line, "echo")) { kputs(arg); if (*arg2) { kputc(' '); kputs(arg2); } kputc('\n'); }
    else if (kstreq(line, "kbd")) {
        if (hal_set_layout(arg) == 0) kputs("clavier change\n"); else kputs("usage : kbd azerty|qwerty\n");
    }
    else if (kstreq(line, "reboot")) hal_reboot();
    else if (kstreq(line, "halt")) { kputs("Arret.\n"); hal_halt(); }
    else if (kstreq(line, "ls")) { int r = fs_ls(*arg ? arg : ".", ls_cb); if (r) err(r); }
    else if (kstreq(line, "cd")) { int r = fs_cd(*arg ? arg : "/"); if (r) err(r); }
    else if (kstreq(line, "pwd")) { char b[128]; fs_pwd(b, sizeof b); kputs(b); kputc('\n'); }
    else if (kstreq(line, "mkdir")) { int r = fs_mkdir(arg); if (r) err(r); }
    else if (kstreq(line, "touch")) { int r = fs_write(arg, (const u8 *)"", 0, 1); if (r) err(r); }
    else if (kstreq(line, "rm")) { int r = fs_rm(arg); if (r) err(r); }
    else if (kstreq(line, "cat")) {
        const u8 *d; usize n; int r = fs_read(arg, &d, &n);
        if (r) err(r); else { for (usize i = 0; i < n; i++) kputc((char)d[i]); if (n && d[n - 1] != '\n') kputc('\n'); }
    }
    else if (kstreq(line, "write") || kstreq(line, "append")) {
        int r = fs_write(arg, (const u8 *)arg2, kstrlen(arg2), line[0] == 'a');
        if (r) err(r);
    }
    else if (kstreq(line, "df")) {
        kputs("fichiers : "); kputu((u32)fs_count()); kputs(" / 128\n");
        kputs("tas      : "); kputu((u32)(heap_used() / 1024)); kputs(" Ko utilises\n");
    }
    else { kputs("commande inconnue : "); kputs(line); kputc('\n'); }
}

void shell_run(void)
{
    char line[160];
    for (;;) {
        char cwdb[128]; fs_pwd(cwdb, sizeof cwdb);
        hal_color(2); kputs("talco"); hal_color(0); kputc(':'); kputs(cwdb); kputs("> ");
        usize n = 0;
        for (;;) {
            char c = hal_getc();
            if (c == '\n' || c == '\r') { kputc('\n'); break; }
            if (c == '\b' || c == 127) { if (n) { n--; kputs("\b \b"); } continue; }
            if (c >= ' ' && n < sizeof line - 1) { line[n++] = c; kputc(c); }
        }
        line[n] = 0;
        shell_exec(line);
    }
}
