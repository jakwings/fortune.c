#ifndef FORTUNE_UTILS_H
#define FORTUNE_UTILS_H 1

/*#ifndef FORTUNE_MAKE_LIB*/
    #include <stdio.h>
    #include <string.h>
    #include <sys/errno.h>
    #define L_ERROR(...) fprintf(stderr, __VA_ARGS__)
    #define P_ERROR(fmt, ...) L_ERROR(fmt ": %s\n", __VA_ARGS__, strerror(errno))
/*#else*/
/*    #define L_ERROR(...)*/
/*    #define P_ERROR(...)*/
/*#endif*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 256
#define CHUNK_SIZE 512

#define FORTUNE_ALLOC(buf, size) do { \
    if ((size) <= 0) { \
        L_ERROR("Requested memeory size <= 0\n"); \
        exit(EXIT_FAILURE); \
    } \
    buf = realloc(buf, \
                  sizeof(*(buf)) * (((size) / CHUNK_SIZE + 1) * CHUNK_SIZE)); \
    if ((buf) == NULL) { \
        perror("Memory limit exceeded"); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define FORTUNE_FREE(buffer) do { \
    free(buffer); \
    buffer = NULL; \
} while (0)

/*
 * Reads at most (count - 1) bytes into buf, and terminate it will NUL.
 * Stop reading whenever an EOF occurs or a newline is read.
 * Return number of bytes read on success, or -1 on failure.
 */
static ssize_t fdgets(int fd, char* buf, int count)
{
    ssize_t nbyte = 0;
    int i = 0;
    while (i < count - 1 && (nbyte = read(fd, buf + i, 1)) > 0) {
        i++;
        if (buf[i-1] == '\n') break;
    }
    buf[i] = '\0';
    return nbyte == -1 ? -1 : i;
}

#endif /* FORTUNE_UTILS_H */
