#include <stdio.h>
#include <string.h>
#include "kernel.h"
extern char out[]; void out_reset(void);
static int fails, total;
#define CHECK(c) do { total++; if (!(c)) { fails++; printf("ECHEC ligne %d : %s\n", __LINE__, #c); } } while (0)
static void sh(const char *cmd) { char b[200]; strcpy(b, cmd); out_reset(); shell_exec(b); }

int main(void)
{
    /* chaines */
    CHECK(kstrlen("abc") == 3); CHECK(kstreq("a", "a")); CHECK(!kstreq("a", "b"));
    /* tas */
    void *a = kmalloc(100), *b = kmalloc(200), *c = kmalloc(300);
    CHECK(a && b && c && a != b);
    usize used = heap_used(); CHECK(used >= 600);
    kfree(b); kfree(a); kfree(c);
    CHECK(heap_used() == 0);
    void *big = kmalloc(1500000); CHECK(big != NULL); kfree(big);
    CHECK(kmalloc(3000000) == NULL);
    /* fichiers */
    fs_init();
    sh("mkdir /home"); CHECK(out[0] == 0);
    sh("mkdir /home"); CHECK(strstr(out, "existe"));
    sh("cd /home"); sh("pwd"); CHECK(!strcmp(out, "/home\n"));
    sh("write a.txt bonjour le monde"); sh("cat a.txt"); CHECK(!strcmp(out, "bonjour le monde\n"));
    sh("append a.txt !"); sh("cat /home/a.txt"); CHECK(!strcmp(out, "bonjour le monde!\n"));
    sh("mkdir docs"); sh("cd docs"); sh("pwd"); CHECK(!strcmp(out, "/home/docs\n"));
    sh("cd .."); sh("pwd"); CHECK(!strcmp(out, "/home\n"));
    sh("ls"); CHECK(strstr(out, "a.txt") && strstr(out, "docs/"));
    sh("rm docs/x"); CHECK(strstr(out, "introuvable"));
    sh("cd /"); sh("rm home"); CHECK(strstr(out, "non vide"));
    sh("rm /home/a.txt"); sh("rm /home/docs"); sh("rm /home"); CHECK(out[0] == 0);
    sh("ls /home"); CHECK(strstr(out, "introuvable"));
    sh("cat /"); CHECK(strstr(out, "dossier"));
    /* divers */
    sh("echo salut tout le monde"); CHECK(!strcmp(out, "salut tout le monde\n"));
    sh("uptime"); CHECK(!strcmp(out, "12.3 s\n"));
    sh("mem"); CHECK(strstr(out, "256 Mo"));
    sh("kbd azerty"); CHECK(strstr(out, "change")); sh("kbd xx"); CHECK(strstr(out, "usage"));
    sh("nimportequoi"); CHECK(strstr(out, "inconnue"));
    /* beaucoup de fichiers */
    sh("mkdir /t"); int ok = 0;
    for (int i = 0; i < 200; i++) { char cmd[64]; sprintf(cmd, "touch /t/f%d", i); sh(cmd); if (!out[0]) ok++; }
    CHECK(ok > 100 && ok < 128);
    printf("%d/%d tests OK\n", total - fails, total);
    return fails != 0;
}
