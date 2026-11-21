/* HAL Linux : le coeur TALCO (shell, fichiers, tas) tourne comme processus init (PID 1) sur un vrai noyau Linux. */
#define _GNU_SOURCE
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include "kernel.h"
#include "hal.h"

static struct timespec t0;
static char arch_buf[64] = "Linux";
static char cpu_buf[96] = "inconnu";

static void out(const char *s, usize n) { (void)!write(1, s, n); }

void hal_init(usize a, usize b)
{
    (void)a; (void)b;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    mount("proc", "/proc", "proc", 0, 0);
    mount("sysfs", "/sys", "sysfs", 0, 0);
    mount("tmpfs", "/tmp", "tmpfs", 0, 0);
    mount("devtmpfs", "/dev", "devtmpfs", 0, 0);
    signal(SIGCHLD, SIG_IGN);
    if (access("/talco-ui", X_OK) == 0 && fork() == 0) {      /* interface graphique si un ecran existe */
        execl("/talco-ui", "talco-ui", (char *)0);
        _exit(1);
    }
    struct utsname u;
    if (uname(&u) == 0) {
        kstrlcpy(arch_buf, "Linux ", sizeof arch_buf);
        usize l = kstrlen(arch_buf);
        kstrlcpy(arch_buf + l, u.machine, sizeof arch_buf - l);
        kstrlcpy(cpu_buf, u.machine, sizeof cpu_buf);
        l = kstrlen(cpu_buf);
        kstrlcpy(cpu_buf + l, ", noyau ", sizeof cpu_buf - l);
        l = kstrlen(cpu_buf);
        kstrlcpy(cpu_buf + l, u.release, sizeof cpu_buf - l);
    }
    struct termios t;
    if (tcgetattr(0, &t) == 0) {            /* saisie brute : le shell fait son propre echo */
        t.c_lflag &= ~(tcflag_t)(ICANON | ECHO | ISIG);
        t.c_cc[VMIN] = 1; t.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &t);
    }
}
void hal_putc(char c) { out(&c, 1); }
char hal_getc(void)
{
    char c;
    for (;;) {
        long r = read(0, &c, 1);
        if (r == 1) return c == '\r' ? '\n' : c;
        sleep(1);
    }
}
void hal_clear(void) { kputs("\033[2J\033[H"); }
void hal_color(int c) { kputs(c == 1 ? "\033[96m" : c == 2 ? "\033[92m" : "\033[0m"); }
u32 hal_uptime_ms(void)
{
    struct timespec n; clock_gettime(CLOCK_MONOTONIC, &n);
    return (u32)((n.tv_sec - t0.tv_sec) * 1000 + (n.tv_nsec - t0.tv_nsec) / 1000000);
}
u32 hal_ram_kb(void)
{
    struct sysinfo si;
    if (sysinfo(&si)) return 0;
    return (u32)(((unsigned long long)si.totalram * si.mem_unit) / 1024);
}
void hal_cpu_info(void) { kputs(cpu_buf); }
int hal_set_layout(const char *n) { (void)n; return -1; }
const char *hal_arch(void) { return arch_buf; }
void hal_reboot(void) { sync(); reboot(RB_AUTOBOOT); _exit(0); }
void hal_halt(void)   { sync(); reboot(RB_POWER_OFF); _exit(0); }

int main(void) { kmain(0, 0); return 0; }
