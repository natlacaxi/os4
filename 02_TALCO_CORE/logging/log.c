#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "talco.h"
void talco_log(const char *layer, const char *fmt, ...)
{
    va_list ap;
    printf("[%6.3f] %-26s ", (double)clock() / CLOCKS_PER_SEC, layer);
    va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
    putchar('\n');
}
