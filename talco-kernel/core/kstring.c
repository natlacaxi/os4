#include "kernel.h"
usize kstrlen(const char *s) { usize n = 0; while (s[n]) n++; return n; }
int kstreq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
void kstrlcpy(char *d, const char *s, usize n)
{
    if (!n) return;
    usize i = 0;
    for (; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}
void *kmemcpy(void *d, const void *s, usize n)
{
    u8 *dd = d; const u8 *ss = s;
    while (n--) *dd++ = *ss++;
    return d;
}
void *kmemset(void *d, int c, usize n)
{
    u8 *dd = d;
    while (n--) *dd++ = (u8)c;
    return d;
}
#ifdef KERNEL
/* le compilateur peut generer ces appels meme en mode freestanding */
void *memcpy(void *d, const void *s, usize n) { return kmemcpy(d, s, n); }
void *memset(void *d, int c, usize n) { return kmemset(d, c, n); }
void *memmove(void *d, const void *s, usize n)
{
    u8 *dd = d; const u8 *ss = s;
    if (dd < ss) while (n--) *dd++ = *ss++;
    else { dd += n; ss += n; while (n--) *--dd = *--ss; }
    return d;
}
int memcmp(const void *a, const void *b, usize n)
{
    const u8 *x = a, *y = b;
    for (; n; n--, x++, y++) if (*x != *y) return *x - *y;
    return 0;
}
#endif
