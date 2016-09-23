#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "endian.h"
#include "str.h"
#include "string.h"
#include "utils.h"

#define _(cookie) fortune ## cookie

#define FORTUNE_OPT_SUMMARY 0x04

static bool push(FortuneIndex**, uint32_t len, off_t pos, int fd, bool dump);
static bool getOptions(int* pargc, char*** pargv, FortuneHeader*);
static void help(FILE*);

bool _(CreateData)(int argc, char** argv)
{
    FortuneHeader header = {0};
    if (!getOptions(&argc, &argv, &header)) return false;

    /* Save it before the flags get sanitized. */
    bool to_summary = header.flags & FORTUNE_OPT_SUMMARY;

    bool done = false;
    if (argc > 0) {
        char* input = argv[0];
        bool from_stdin = !strcmp(input, "-");
        char* output = argc >= 2 ? argv[1] :
            (from_stdin ? "-" : addSuffix(input, ".dat"));
        if (output) done = _(CreateDataFromPath)(input, output, &header);
        if (!done) {
            L_ERROR("Failed to create data file: %s\n", output);
        } else if (to_summary) {
            printf("Fortune cookies baked: %u\n", header.total);
        }
        if (argc < 2 && !from_stdin) free(output);
    }
    return done;
}

bool _(CreateDataFromFd)(int in, int out, FortuneHeader* header)
{
    uint32_t num_idx = 0;
    uint8_t flags = header->flags;
    char delim = header->delim;

    bool commented = flags & FORTUNE_DATA_COMMENT;
    bool seekable_in = lseek(in, 0, SEEK_CUR) != -1;
    bool seekable_out = lseek(out, 0, SEEK_CUR) != -1;

    /* Write indexes first if possible. */
    if (seekable_out) lseek(out, sizeof(*header), SEEK_SET);

    FortuneIndex* index = NULL;
    off_t last_pos = seekable_in ? lseek(in, 0, SEEK_CUR) : 0;
    size_t length = 0;
    size_t line_length = 0;
    size_t comment_length = 0;
    bool in_comment = false;
    char buf[BUF_SIZE+1];
    ssize_t nbyte = 0;
    while ((nbyte = fdgets(in, buf, BUF_SIZE)) >= 0) {
        length += nbyte;
        line_length += nbyte;
        if (nbyte == 0 ||
                (line_length <= BUF_SIZE /* near the previous newline */ &&
                 nbyte == 2 && buf[0] == delim && buf[1] == '\n')) {
            off_t pos = last_pos + length;
            length = length - nbyte - comment_length;
            if (0 < length && length < FORTUNE_DATA_MAX_LENGTH) {
                if (!push(&index, ++num_idx, last_pos, out, seekable_out)) {
                    goto FAIL;
                }
            }
            last_pos = pos;
            length = line_length = comment_length = 0;
            in_comment = false;
        } else if (commented) {
            if (in_comment ||
                    (line_length <= BUF_SIZE /* near the previous newline */ &&
                     nbyte >= 2 && buf[0] == delim && buf[1] == delim)) {
                comment_length += nbyte;
                in_comment = buf[nbyte-1] != '\n';
            }
        }
        if (nbyte == 0) break;
        if (buf[nbyte-1] == '\n') line_length = 0;
    }
    if (nbyte == -1) goto FAIL;

    header->version = htobe16(FORTUNE_DATA_VERSION);
    header->total = htobe32(num_idx);
    header->flags = flags & FORTUNE_DATA_MASK;
    header->delim = delim;

    /* Dump all remaining data. */
    if (seekable_out) lseek(out, 0, SEEK_SET);
    if (write(out, header, sizeof(*header)) != sizeof(*header)) goto FAIL;
    if (!seekable_out) {
        for (FortuneIndex* p = index; p < index + num_idx; p++) {
            *p = htobe64(*p);
        }
        size_t bytes = sizeof(*index) * num_idx;
        if (write(out, index, bytes) != bytes) goto FAIL;
    }

    header->version = FORTUNE_DATA_VERSION;
    header->total = num_idx;
    FORTUNE_FREE(index);
    return true;

FAIL:
    header->version = FORTUNE_DATA_VERSION;
    header->total = 0;
    header->flags = flags & FORTUNE_DATA_MASK;
    FORTUNE_FREE(index);
    return false;
}

bool _(CreateDataFromFile)(FILE* in, FILE* out, FortuneHeader* header)
{
    return _(CreateDataFromFd)(fileno(in), fileno(out), header);
}

bool _(CreateDataFromPath)(char* in, char* out, FortuneHeader* header)
{
    bool done = false;
    bool from_stdin = !strcmp(in, "-");
    bool to_stdout = !strcmp(out, "-");

    FILE* fin = from_stdin ? stdin : fopen(in, "r");
    if (!fin) {
        P_ERROR("Failed to read \"%s\"", from_stdin ? "stdin" : in);
        return false;
    }
    FILE* fout = to_stdout ? stdout : fopen(out, "w");
    if (!fout) {
        P_ERROR("Failed to write \"%s\"", to_stdout ? "stdout" : out);
        if (!from_stdin) fclose(fin);
        return false;
    }

    done = _(CreateDataFromFd)(fileno(fin), fileno(fout), header);
    if (!done) P_ERROR("Failed to write \"%s\"", to_stdout ? "stdout" : out);
    if (!from_stdin) fclose(fin);
    if (!to_stdout) fclose(fout);
    return done;
}

static bool push(FortuneIndex** idx, uint32_t len, off_t pos, int fd, bool dump)
{
    if (dump) {
        FortuneIndex pos_be = htobe64(pos);
        return write(fd, &pos_be, sizeof(pos_be)) == sizeof(pos_be);
    } else {
        FORTUNE_ALLOC(*idx, len);
        (*idx)[len] = pos;
        return true;
    }
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

static void help(FILE* stream)
{
    fprintf(stream,
            "Usage:\n"
            "    str [-h] [-d <char>] [-cxs] <input> [output]\n"
            "\n"
            "Options:\n"
            "    -h       Show this manual.\n"
            "    -d char  Change the default delimiter '%%' to <char>.\n"
            "    -c       Take lines started with two delimiters as comments.\n"
            "    -x       Each letter is rotated by 13 places.\n"
            "    -s       Print a summary.\n"
            "\n"
            "    The file name '-' represents standard input or output.\n"
            "    If not specified, <output> defaults to \"<input>.dat\".\n"
            "    If <input> is '-', <output> defaults to '-'.\n"
           );
}

#ifndef FORTUNE_MAKE_LIB
int main(int argc, char** argv)
{
    if (_(CreateData)(argc, argv)) {
        return EXIT_SUCCESS;
    } else {
        help(stderr);
        return EXIT_FAILURE;
    }
}
#endif /* FORTUNE_MAKE_LIB */
