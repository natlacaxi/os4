#ifndef KERNEL_H
#define KERNEL_H
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef unsigned long      usize;   /* 32 bits sur x86, 64 bits sur aarch64 */
#define NULL ((void *)0)
#define TALCO_VERSION "0.2.0"

/* chaines et memoire */
usize kstrlen(const char *s);
int   kstreq(const char *a, const char *b);
void  kstrlcpy(char *dst, const char *src, usize n);
void *kmemcpy(void *d, const void *s, usize n);
void *kmemset(void *d, int c, usize n);

/* console (via la HAL) */
void kputc(char c);
void kputs(const char *s);
void kputu(u32 n);
void kputhex(u32 n);

/* tas memoire */
void *kmalloc(usize n);
void  kfree(void *p);
usize heap_used(void);
usize heap_total(void);

/* systeme de fichiers en RAM */
typedef void (*fs_ls_cb)(const char *name, int is_dir, usize size);
int  fs_init(void);
int  fs_mkdir(const char *path);
int  fs_write(const char *path, const u8 *data, usize len, int append);
int  fs_read(const char *path, const u8 **data, usize *len);
int  fs_ls(const char *path, fs_ls_cb cb);
int  fs_rm(const char *path);
int  fs_cd(const char *path);
void fs_pwd(char *buf, usize n);
int  fs_count(void);
#define FS_OK 0
#define FS_ENOENT (-1)
#define FS_EEXIST (-2)
#define FS_EFULL  (-3)
#define FS_EISDIR (-4)
#define FS_ENOTDIR (-5)
#define FS_ENOTEMPTY (-6)
#define FS_EINVAL (-7)

/* shell */
void shell_exec(char *line);
void shell_run(void);
void kmain(usize a0, usize a1);
#endif
