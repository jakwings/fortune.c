#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "endian.h"
#include "string.h"
#include "unstr.h"
#include "utils.h"

#define _(cookie) fortune ## cookie

#define FORTUNE_OPT_SUMMARY 0x04

static bool seek(int fd, off_t offset, bool seekable);
static bool getOptions(int* pargc, char*** pargv, FortuneHeader*);
static void help(FILE*);

bool _(DumpData)(int argc, char** argv)
{
    FortuneHeader header = {0};
    if (!getOptions(&argc, &argv, &header)) return false;

    /* Save it before the flags get sanitized. */
    bool to_summary = header.flags & FORTUNE_OPT_SUMMARY;

    bool done = false;
    if (argc > 0) {
        char* input = argv[0];
        char* output = argc >= 2 ? argv[1] : "-";
        bool from_stdin = !strcmp(input, "-");
        char* index = argc >= 3 ? argv[2] :
            (from_stdin ? NULL : addSuffix(input, ".dat"));
        if (index) {
            done = _(DumpDataFromPath)(input, index, output, &header);
        } else {
            L_ERROR("Failed to read data file: %s\n", index);
        }
        if (!done) {
            L_ERROR("Failed to dump data file to: %s\n", output);
        } else if (to_summary) {
            fprintf(stderr, "Fortune cookies eaten: %u\n", header.total);
        }
        if (argc < 3) free(index);
    }
    return done;
}

bool _(DumpDataFromFd)(int in, int idx, int out, FortuneHeader* header)
{
    uint8_t no_comment = header->flags & FORTUNE_DATA_COMMENT;
    uint8_t no_rotate = header->flags & FORTUNE_DATA_ROTATED;
    char delim2[2] = {header->delim, '\n'};

    if (read(idx, header, sizeof(*header)) != sizeof(*header)) return false;
    header->version = be16toh(header->version);
    header->total = be32toh(header->total);
    if (header->version != FORTUNE_DATA_VERSION) return false;

    uint32_t total = header->total;
    uint8_t flags = header->flags;
    char delim = header->delim;
    no_comment = no_comment ? (flags & FORTUNE_DATA_COMMENT) : 0;
    no_rotate = no_rotate ? (flags & FORTUNE_DATA_ROTATED) : 0;
    bool seekable_in = lseek(in, 0, SEEK_CUR) != -1;

    uint32_t i = 0;
    FortuneIndex offset = -1;
    while (i++ < total &&
            read(idx, &offset, sizeof(offset)) == sizeof(offset)) {
        offset = be64toh(offset);
        if (!seek(in, offset, seekable_in)) return false;

        char buf[BUF_SIZE+1];
        size_t length = 0;
        bool in_comment = false;
        ssize_t nbyte = 0;
        while ((nbyte = fdgets(in, buf, BUF_SIZE)) > 0) {
            length += nbyte;
            if (nbyte == 2 && buf[0] == delim && buf[1] == '\n') {
                if (write(out, delim2, sizeof(delim2)) != sizeof(delim2)) {
                    return false;
                }
                break;
            }
            if (in_comment /* far from the previous delimiter */ ||
                    (length <= BUF_SIZE /* near the previous delimiter */ &&
                     nbyte >= 2 && buf[0] == delim && buf[1] == delim)) {
                if (no_comment) continue;
                in_comment = buf[nbyte-1] != '\n';
                if (length <= BUF_SIZE) buf[0] = buf[1] = delim2[0];
            }
            if (no_rotate) rotate(buf, nbyte);
            if (write(out, buf, nbyte) != nbyte) return false;
        }
        if (nbyte == -1) return false;
    }
    if (offset == -1) return false;

    /* Update according to the properties of output. */
    header->total = total;
    header->flags = flags & ~no_comment & ~no_rotate & FORTUNE_DATA_MASK;
    header->delim = delim2[0];
    return true;
}

bool _(DumpDataFromFile)(FILE* in, FILE* idx, FILE* out, FortuneHeader* header)
{
    return _(DumpDataFromFd)(fileno(in), fileno(idx), fileno(out), header);
}

bool _(DumpDataFromPath)(char* in, char* idx, char* out, FortuneHeader* header)
{
    bool done = false;
    bool from_stdin = !strcmp(in, "-");
    bool from_stdin2 = !strcmp(idx, "-");
    bool to_stdout = !strcmp(out, "-");

    FILE* fin = from_stdin ? stdin : fopen(in, "r");
    if (!fin) {
        P_ERROR("Failed to read \"%s\"", from_stdin ? "stdin" : in);
        return false;
    }
    FILE* fidx = from_stdin2 ? stdin : fopen(idx, "r");
    if (!fidx) {
        P_ERROR("Failed to read index \"%s\"", from_stdin2 ? "stdin" : idx);
        if (!from_stdin) fclose(fin);
        return false;
    }
    FILE* fout = to_stdout ? stdout : fopen(out, "w");
    if (!fout) {
        P_ERROR("Failed to write \"%s\"", to_stdout ? "stdout" : out);
        if (!from_stdin) fclose(fin);
        if (!from_stdin2) fclose(fidx);
        return false;
    }

    done = _(DumpDataFromFd)(fileno(fin), fileno(fidx), fileno(fout), header);
    if (!done) P_ERROR("Failed to write \"%s\"", to_stdout ? "stdout" : out);
    if (!from_stdin) fclose(fin);
    if (!from_stdin2) fclose(fidx);
    if (!to_stdout) fclose(fout);
    return done;
}

static bool seek(int fd, off_t offset, bool seekable)
{
    if (offset < 0) return false;
    if (seekable) return lseek(fd, offset, SEEK_SET) != -1;
    char c;
    for (off_t i = 0; i < offset; i++) {
        if (read(fd, &c, sizeof(c)) != 1) return false;
    }
    return true;
}

static bool getOptions(int* pargc, char*** pargv, FortuneHeader* opts)
{
    int argc = *pargc;
    char** argv = *pargv;
    uint8_t flags = 0;
    char delim = '%';

    bool to_help = false;
    int c;
    while ((c = getopt(argc, argv, "d:cxsh")) != -1) {
        switch (c) {
        case 'd': delim = *optarg; break;
        case 'c': flags |= FORTUNE_DATA_COMMENT; break;
        case 'x': flags |= FORTUNE_DATA_ROTATED; break;
        case 's': flags |= FORTUNE_OPT_SUMMARY; break;
        case 'h': to_help = true; break;
        default: return false;
        }
    }
    if (to_help) {
        help(optind > 2 ? stderr : stdout);
        exit(optind > 2 ? EXIT_FAILURE : EXIT_SUCCESS);
    }

    opts->flags = flags;
    opts->delim = delim;
    *pargc = argc - optind;
    *pargv = argv + optind;
    return true;
}

static void help(FILE* stream) {
    fprintf(stream,
            "Usage:\n"
            "    unstr [-h] [-d <char>] [-cxs] <input> [output] [data]\n"
            "\n"
            "Options:\n"
            "    -h       Show this manual.\n"
            "    -d char  Change the delimiter of data to <char>.\n"
            "    -c       Remove comments.\n"
            "    -x       Rotate each letter by 13 places if it was rotated.\n"
            "    -s       Print a summary.\n"
            "\n"
            "    The file name '-' represents standard input or output.\n"
            "    If not specified, <output> defaults to standard output.\n"
            "    <data> defaults to \"<input>.dat\" if <input> isn't '-'.\n"
           );
}


#ifndef FORTUNE_MAKE_LIB
int main(int argc, char** argv)
{
    if (_(DumpData)(argc, argv)) {
        return EXIT_SUCCESS;
    } else {
        help(stderr);
        return EXIT_FAILURE;
    }
}
#endif /* FORTUNE_MAKE_LIB */
