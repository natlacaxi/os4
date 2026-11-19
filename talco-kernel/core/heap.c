#include "kernel.h"
/* Allocateur first-fit avec fusion des blocs libres. 2 Mo statiques. */
#define HEAP_SIZE (2u * 1024 * 1024)
typedef struct hdr { usize size; struct hdr *next; usize free; usize pad; } __attribute__((aligned(16))) hdr_t;
static u8 area[HEAP_SIZE] __attribute__((aligned(16)));
static hdr_t *head;

static void heap_init(void)
{
    head = (hdr_t *)area;
    head->size = HEAP_SIZE - sizeof(hdr_t);
    head->next = NULL; head->free = 1;
}
void *kmalloc(usize n)
{
    if (!head) heap_init();
    if (!n) return NULL;
    n = (n + 15) & ~(usize)15;
    for (hdr_t *h = head; h; h = h->next) {
        if (!h->free || h->size < n) continue;
        if (h->size >= n + sizeof(hdr_t) + 16) {         /* decoupe */
            hdr_t *r = (hdr_t *)((u8 *)(h + 1) + n);
            r->size = h->size - n - sizeof(hdr_t);
            r->next = h->next; r->free = 1;
            h->size = n; h->next = r;
        }
        h->free = 0;
        return h + 1;
    }
    return NULL;
}
void kfree(void *p)
{
    if (!p) return;
    ((hdr_t *)p - 1)->free = 1;
    for (hdr_t *h = head; h; h = h->next)                 /* fusion */
        while (h->free && h->next && h->next->free) {
            h->size += sizeof(hdr_t) + h->next->size;
            h->next = h->next->next;
        }
}
usize heap_used(void)
{
    if (!head) heap_init();
    usize u = 0;
    for (hdr_t *h = head; h; h = h->next) if (!h->free) u += h->size;
    return u;
}
usize heap_total(void) { return HEAP_SIZE; }
