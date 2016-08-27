#ifndef FORTUNE_ENDIAN_H
#define FORTUNE_ENDIAN_H 1

#if defined(__APPLE__) && defined(__MACH__)
    #include <machine/endian.h>
#elif defined(__FreeBSD__)
    #include <sys/endian.h>
#else
    #include <endian.h>
#endif

#define htobe64(x) htonll(x)
#define htobe32(x) htonl(x)
#define htobe16(x) htons(x)
#define be64toh(x) ntohll(x)
#define be32toh(x) ntohl(x)
#define be16toh(x) ntohs(x)

#endif /* FORTUNE_ENDIAN_H */
