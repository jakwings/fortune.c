#ifndef FORTUNE_STRING_H
#define FORTUNE_STRING_H 1

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static char* addSuffix(char* src, char* sfx)
{
    if (!(src && sfx)) return NULL;
    size_t len_src = strlen(src);
    size_t len_sfx = strlen(sfx);
    size_t len_dst = len_src + len_sfx;
    char* dst = malloc(sizeof(char) * (len_dst + 1));
    if (!dst) return NULL;
    memcpy(dst, src, len_src + 1);
    memcpy(dst + len_src, sfx, len_sfx + 1);
    return dst;
}

static char* joinPath(char* base, char* name)
{
    if (!(base && name)) return NULL;
    if (name[0] == '/') return name;
    size_t len_base = strlen(base);
    size_t len_name = strlen(name);
    size_t len_path = len_base + len_name + 1;
    char* path = malloc(sizeof(char) * (len_path + 1));
    if (!path) return NULL;
    memcpy(path, base, len_base);
    path[len_base] = '/';
    memcpy(path + len_base + 1, name, len_name);
    path[len_path] = '\0';
    return path;
}

static bool endsWith(char* buf, char* sfx)
{
    if (!(buf && sfx)) return NULL;
    size_t len_buf = strlen(buf);
    size_t len_sfx = strlen(sfx);
    return len_buf >= len_sfx && !strcmp(&buf[len_buf-len_sfx], sfx);
}

#endif /* FORTUNE_STRING_H */
