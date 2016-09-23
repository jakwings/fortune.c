#ifndef FORTUNE_DATA_H
#define FORTUNE_DATA_H 1

#include <stdint.h>

/* version of binary data format */
#define FORTUNE_DATA_VERSION 1

/* maximum length of cookie text */
#define FORTUNE_DATA_MAX_LENGTH 0x7FFFFFFF

#define FORTUNE_DATA_MASK     0x03
#define FORTUNE_DATA_COMMENT  0x01
#define FORTUNE_DATA_ROTATED  0x02

/*--------------------------------------------*\
 | INDEX FILE FORMAT
 | =
 | |HEADER  --> STATISTICS
 | |INDEX:0 --> SOURCE:FIRST
 | |INDEX:1 --> SOURCE:NEXT
 | |.....:.
 | |INDEX:N --> SOURCE:LAST
 | =
 *--------------------------------------------*/

typedef struct {
    /* big-endian */
    uint16_t version;
    uint8_t flags;
    char delim;
    uint32_t total;
} FortuneHeader;

/* big-endian */
typedef uint64_t FortuneIndex;

/*--------------------------------------------*\
 | SOURCE FILE FORMAT
 |
 | * Fortune cookies are delimited by
 |   a delimiter followed by a newline.  E.g.
 |
 |   |This is just a fortune cookie.
 |   |%
 |   |This is just another fortune cookie.
 |
 | * If flags have the FORTUNE_DATA_COMMENT bit
 |   set to 1, any line that starts with two
 |   delimiter is a comment.  E.g.
 |
 |   |This is just a fortune cookie.
 |   |%%This cookie is commented, or not.
 |   |%
 |   |This is just another fortune cookie.
 |
 | * Both empty cookies and ones that contain
 |   only comments will not be indexed.  E.g.
 |
 |   |This is the first cookie.
 |   |%
 |   |%
 |   |%% The second cookie is lost.
 |   |%
 |   |This is the fourth cookie.
 |
 *--------------------------------------------*/

#endif /* FORTUNE_DATA_H */
