#include <stdio.h>
#include "rotate.h"
#include "utils.h"

#define _(cookie) fortune ## cookie

#define S1(l, r, c) \
    ((l) <= (c) && (c) <= (r))

#define S2(l, r, p) \
    (S1(l, r, (p)[0]) && \
     S1(0x80, 0xBF, (p)[1]))

#define S3(l, r, p) \
    (S1(l, r, (p)[0]) && \
     S1(0x80, 0xBF, (p)[1]) && \
     S1(0x80, 0xBF, (p)[2]))

#define S4(l, r, p) \
    (S1(l, r, (p)[0]) && \
     S1(0x80, 0xBF, (p)[1]) && \
     S1(0x80, 0xBF, (p)[2]) && \
     S1(0x80, 0xBF, (p)[3]))

static int skip(unsigned char* p, int m, int n);
static unsigned char rotate(size_t s, size_t e, unsigned char c);

bool _(RotateData)(int argc, char** argv)
{
    char buf[4096];
    size_t nbyte = 0;
    while ((nbyte = fread(buf, 1, sizeof(buf), stdin))) {
        if (ferror(stdin)) {
            P_ERROR("%s", "Failed to read from stdin");
            return false;
        }
        if (fwrite(_(Rotate)(buf, nbyte), 1, nbyte, stdout) != nbyte) {
            P_ERROR("%s", "Failed to write to stdout");
            return false;
        }
    }
    return true;
}

static int skip(unsigned char* p, int m, int n)
{
    /* U+80-A0 */
    if (m + 1 < n && 0xC2 == p[0] && S1(0x80, 0xA0, p[1])) return 1;
    if (m + 2 < n &&
             /* U+180B-180F */
            ((0xE1 == p[0] && 0xA0 == p[1] && S1(0x8B, 0x8F, p[2])) ||
             /* U+2000-200F */
             (0xE2 == p[0] && 0x80 == p[1] && S1(0x80, 0x8F, p[2])) ||
             /* U+2028-202F */
             (0xE2 == p[0] && 0x80 == p[1] && S1(0xA8, 0xAF, p[2])) ||
             /* U+205F-206F */
             (0xE2 == p[0] && 0x81 == p[1] && S1(0x9F, 0xAF, p[2])) ||
             /* U+3000 */
             (0xE3 == p[0] && 0x80 == p[1] && 0x80 == p[2]) ||
             /* U+E000-EFFF */
             (0xEE == p[0] && S1(0x80, 0xBF, p[1]) && S1(0x80, 0xBF, p[2])) ||
             /* U+F000-F8FF */
             (0xEF == p[0] && S1(0x80, 0xA3, p[1]) && S1(0x80, 0xBF, p[2])) ||
             /* U+FDD0-FDEF */
             (0xEF == p[0] && 0xB7 == p[1] && S1(0x90, 0xAF, p[2])) ||
             /* U+FE00-FE0F */
             (0xEF == p[0] && 0xB8 == p[1] && S1(0x80, 0x8F, p[2])) ||
             /* U+FF00 */
             (0xEF == p[0] && 0xBC == p[1] && 0x80 == p[2]) ||
             /* U+FFF0-FFFF */
             (0xEF == p[0] && 0xBF == p[1] && S1(0xB0, 0xBF, p[2])))) {
        return 2;
    }
    if (m + 3 < n &&
             /* U+0E0000-0FFFFF */
            ((0xF3 == p[0] && S1(0xA0, 0xBF, p[1]) &&
              S1(0x80, 0xBF, p[2]) && S1(0x80, 0xBF, p[3])) ||
             /* U+100000-10FFFF */
             (0xF4 == p[0] && S1(0x80, 0x8F, p[1]) &&
              S1(0x80, 0xBF, p[2]) && S1(0x80, 0xBF, p[3])))) {
        return 3;
    }
    return 0;
}

static unsigned char rotate(size_t s, size_t e, unsigned char c)
{
    size_t n = e - s + 1;
    size_t r = n % 26;
    size_t i = c - s + 1;
    size_t k = s + 26 * (i / 26 - (i % 26 == 0));
    /* |A-Z|A-Z|?-?|A-Z|... */
    if (c - s + 1 <= n - r) return k + (c - k + 13) % 26;
    /* |A-Z|A-Z|...|A-?| */
    if (r % 2 == 0) return k + (c - k + r / 2) % r;
    if (c != e) return k + (c - k + (r - 1) / 2) % (r - 1);
    return c;
}

char* _(Rotate)(char* s, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        char c = s[i];
        if (S1('A', 'Z', c)) {
            s[i] = rotate('A', 'Z', c);
        } else if (S1('a', 'z', c)) {
            s[i] = rotate('a', 'z', c);
        } else if (!S1(0x00, 0x7F, c)) {
            unsigned char* p = (void*)(s + i);

            size_t k = 0;
            if ((k = skip(p, i, n))) {
                i += k;
                continue;
            }
            /* Complement for S_SKIP */
            if (i + 1 < n && 0xC2 == p[0] && S1(0xA1, 0xBF, p[1])) {
                p[1] = rotate(0xA1, 0xBF, p[1]);  /* odd */
                i += 1; continue;
            }
            if (i + 2 < n) {
                if (0xE1 == p[0] && 0xA0 == p[1] && S1(0x80, 0x8A, p[2])) {
                    p[2] = rotate(0x80, 0x8A, p[2]);  /* odd */
                    i += 2; continue;
                }
                if (0xE1 == p[0] && 0xA0 == p[1] && S1(0x90, 0xBF, p[2])) {
                    p[2] = rotate(0x90, 0xBF, p[2]);  /* even */
                    i += 2; continue;
                }
                if (0xE2 == p[0] && 0x80 == p[1] && S1(0x90, 0xA7, p[2])) {
                    p[2] = rotate(0x90, 0xA7, p[2]);  /* even */
                    i += 2; continue;
                }
                if (0xE2 == p[0] && 0x80 == p[1] && S1(0xB0, 0xBF, p[2])) {
                    p[2] = rotate(0xB0, 0xBF, p[2]);  /* even */
                    i += 2; continue;
                }
                if (0xE2 == p[0] && 0x81 == p[1] && S1(0x80, 0x9E, p[2])) {
                    p[2] = rotate(0x80, 0x9E, p[2]);  /* odd */
                    i += 2; continue;
                }
                if (0xE2 == p[0] && 0x81 == p[1] && S1(0xB0, 0xBF, p[2])) {
                    p[2] = rotate(0xB0, 0xBF, p[2]);  /* even */
                    i += 2; continue;
                }
                if (0xE3 == p[0] && 0x80 == p[1] && S1(0x81, 0xBF, p[2])) {
                    p[2] = rotate(0x81, 0xBF, p[2]);  /* odd */
                    i += 2; continue;
                }
                if (0xEF == p[0] && 0xB7 == p[1] && S1(0x80, 0x8F, p[2])) {
                    p[2] = rotate(0x80, 0x8F, p[2]);  /* even */
                    i += 2; continue;
                }
                if (0xEF == p[0] && 0xB7 == p[1] && S1(0xB0, 0xBF, p[2])) {
                    p[2] = rotate(0xB0, 0xBF, p[2]);  /* even */
                    i += 2; continue;
                }
                if (0xEF == p[0] && 0xB8 == p[1] && S1(0x90, 0xBF, p[2])) {
                    p[2] = rotate(0x90, 0xBF, p[2]);  /* even */
                    i += 2; continue;
                }
                if (0xEF == p[0] && 0xBC == p[1] && S1(0x81, 0xBF, p[2])) {
                    p[2] = rotate(0x81, 0xBF, p[2]);  /* odd */
                    i += 2; continue;
                }
                if (0xEF == p[0] && 0xBF == p[1] && S1(0x80, 0xAF, p[2])) {
                    p[2] = rotate(0x80, 0xAF, p[2]);  /* even */
                    i += 2; continue;
                }
            }

            /* Do not care for invalid byte sequences in input nor output. */
            if (i + 1 < n && S2(0xC0, 0xDF, p)) {
                p[1] = rotate(0x80, 0xBF, p[1]);  /* even */
                i += 1; continue;
            }
            if (i + 2 < n && S3(0xE0, 0xEF, p)) {
                p[2] = rotate(0x80, 0xBF, p[2]);  /* even */
                i += 2; continue;
            }
            if (i + 3 < n && S4(0xF0, 0xF7, p)) {
                p[3] = rotate(0x80, 0xBF, p[3]);  /* even */
                i += 3; continue;
            }
        }
    }
    return s;
}

#ifndef FORTUNE_MAKE_LIB
#include <stdlib.h>

int main(int argc, char** argv)
{
    if (_(RotateData)(argc, argv)) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
#endif /* FORTUNE_MAKE_LIB */
