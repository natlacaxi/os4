#include <stdio.h>
#include <string.h>
#include "talco.h"

static int boot(void)
{
    int ok = 0, ko = 0;
    printf("TALCO-OS3 %s -- demarrage (simulation hote)\n", TALCO_VERSION);
    for (int i = 0; i < talco_module_count; i++) {
        const talco_module_t *m = &talco_modules[i];
        if (m->init() == 0) ok++; else { ko++; talco_log(m->layer, "ECHEC %s", m->name); }
    }
    printf("Boot termine : %d modules OK, %d en echec\n", ok, ko);
    return ko ? 1 : 0;
}

static void shell(void)
{
    char line[128];
    puts("TALCO shell -- commandes : modules, layers, version, exit");
    for (;;) {
        printf("talco> "); fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (!strcmp(line, "exit")) break;
        else if (!strcmp(line, "version")) puts(TALCO_VERSION);
        else if (!strcmp(line, "modules"))
            for (int i = 0; i < talco_module_count; i++) printf("%s/%s\n", talco_modules[i].layer, talco_modules[i].name);
        else if (!strcmp(line, "layers")) {
            const char *last = "";
            for (int i = 0; i < talco_module_count; i++)
                if (strcmp(last, talco_modules[i].layer)) { last = talco_modules[i].layer; puts(last); }
        } else if (*line) puts("commande inconnue");
    }
}

int main(int argc, char **argv)
{
    int rc = boot();
    if (argc > 1 && !strcmp(argv[1], "--shell")) shell();
    return rc;
}
