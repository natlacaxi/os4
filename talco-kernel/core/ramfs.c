#include "kernel.h"
#define FS_MAX 128
typedef struct { char name[32]; u8 used, dir; int parent; u8 *data; usize size; } fnode;
static fnode nodes[FS_MAX];
static int cwd;

int fs_init(void)
{
    kmemset(nodes, 0, sizeof nodes);
    nodes[0].used = 1; nodes[0].dir = 1; nodes[0].parent = 0;
    cwd = 0;
    return 0;
}
static int child(int parent, const char *name)
{
    for (int i = 1; i < FS_MAX; i++)
        if (nodes[i].used && nodes[i].parent == parent && kstreq(nodes[i].name, name)) return i;
    return -1;
}
static int lookup(const char *path)
{
    int cur = path[0] == '/' ? 0 : cwd;
    char comp[32];
    while (*path) {
        while (*path == '/') path++;
        if (!*path) break;
        usize n = 0;
        while (path[n] && path[n] != '/') n++;
        if (n >= sizeof comp) return FS_EINVAL;
        for (usize i = 0; i < n; i++) comp[i] = path[i];
        comp[n] = 0; path += n;
        if (kstreq(comp, ".")) continue;
        if (kstreq(comp, "..")) { cur = nodes[cur].parent; continue; }
        if (!nodes[cur].dir) return FS_ENOTDIR;
        int c = child(cur, comp);
        if (c < 0) return FS_ENOENT;
        cur = c;
    }
    return cur;
}
/* separe path en (dossier parent, nom) */
static int split(const char *path, char leaf[32])
{
    char dir[128];
    usize len = kstrlen(path);
    if (!len || len >= sizeof dir) return FS_EINVAL;
    kstrlcpy(dir, path, sizeof dir);
    while (len > 1 && dir[len - 1] == '/') dir[--len] = 0;
    usize slash = len;
    while (slash > 0 && dir[slash - 1] != '/') slash--;
    const char *name = dir + slash;
    if (!*name || kstrlen(name) >= 32 || kstreq(name, ".") || kstreq(name, "..")) return FS_EINVAL;
    kstrlcpy(leaf, name, 32);
    if (slash == 0) return cwd;
    if (slash == 1) return 0;
    dir[slash - 1] = 0;
    int p = lookup(dir);
    if (p < 0) return p;
    return nodes[p].dir ? p : FS_ENOTDIR;
}
static int create(const char *path, int dir)
{
    char leaf[32];
    int p = split(path, leaf);
    if (p < 0) return p;
    if (child(p, leaf) >= 0) return FS_EEXIST;
    for (int i = 1; i < FS_MAX; i++)
        if (!nodes[i].used) {
            kmemset(&nodes[i], 0, sizeof nodes[i]);
            nodes[i].used = 1; nodes[i].dir = (u8)dir; nodes[i].parent = p;
            kstrlcpy(nodes[i].name, leaf, 32);
            return i;
        }
    return FS_EFULL;
}
int fs_mkdir(const char *path) { int r = create(path, 1); return r < 0 ? r : FS_OK; }
int fs_write(const char *path, const u8 *data, usize len, int append)
{
    int i = lookup(path);
    if (i == FS_ENOENT) i = create(path, 0);
    if (i < 0) return i;
    if (nodes[i].dir) return FS_EISDIR;
    usize old = append ? nodes[i].size : 0;
    u8 *buf = kmalloc(old + len + 1);
    if (!buf) return FS_EFULL;
    if (old) kmemcpy(buf, nodes[i].data, old);
    if (len) kmemcpy(buf + old, data, len);
    buf[old + len] = 0;
    if (nodes[i].data) kfree(nodes[i].data);
    nodes[i].data = buf; nodes[i].size = old + len;
    return FS_OK;
}
int fs_read(const char *path, const u8 **data, usize *len)
{
    int i = lookup(path);
    if (i < 0) return i;
    if (nodes[i].dir) return FS_EISDIR;
    *data = nodes[i].data; *len = nodes[i].size;
    return FS_OK;
}
int fs_ls(const char *path, fs_ls_cb cb)
{
    int d = lookup(path);
    if (d < 0) return d;
    if (!nodes[d].dir) { cb(nodes[d].name, 0, nodes[d].size); return FS_OK; }
    for (int i = 1; i < FS_MAX; i++)
        if (nodes[i].used && nodes[i].parent == d) cb(nodes[i].name, nodes[i].dir, nodes[i].size);
    return FS_OK;
}
int fs_rm(const char *path)
{
    int i = lookup(path);
    if (i < 0) return i;
    if (i == 0) return FS_EINVAL;
    if (nodes[i].dir)
        for (int j = 1; j < FS_MAX; j++)
            if (nodes[j].used && nodes[j].parent == i) return FS_ENOTEMPTY;
    if (nodes[i].data) kfree(nodes[i].data);
    if (cwd == i) cwd = nodes[i].parent;
    nodes[i].used = 0;
    return FS_OK;
}
int fs_cd(const char *path)
{
    int i = lookup(path);
    if (i < 0) return i;
    if (!nodes[i].dir) return FS_ENOTDIR;
    cwd = i;
    return FS_OK;
}
void fs_pwd(char *buf, usize n)
{
    int chain[16], depth = 0;
    for (int c = cwd; c != 0 && depth < 16; c = nodes[c].parent) chain[depth++] = c;
    usize pos = 0;
    if (!depth) { kstrlcpy(buf, "/", n); return; }
    while (depth--) {
        if (pos + 1 < n) buf[pos++] = '/';
        for (const char *s = nodes[chain[depth]].name; *s && pos + 1 < n; s++) buf[pos++] = *s;
    }
    buf[pos] = 0;
}
int fs_count(void)
{
    int c = 0;
    for (int i = 0; i < FS_MAX; i++) c += nodes[i].used;
    return c;
}
