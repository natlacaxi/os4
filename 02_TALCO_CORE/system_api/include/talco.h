#ifndef TALCO_H
#define TALCO_H
#define TALCO_VERSION "0.0.1"
typedef int (*talco_init_fn)(void);
typedef struct { const char *layer; const char *name; talco_init_fn init; } talco_module_t;
void talco_log(const char *layer, const char *fmt, ...);
extern const talco_module_t talco_modules[];
extern const int talco_module_count;
#endif
