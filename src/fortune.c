#include <assert.h>
#include <dirent.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fortune.h"
#include "endian.h"
#include "string.h"
#include "utils.h"

#ifndef FORTUNE_MAKE_LIB
#include "str.h"
#include "unstr.h"
#endif

#define _(cookie) fortune ## cookie

typedef struct FileList_s {
    float percent;
    char* path;
    char* data;
    struct FileList_s* next;
    struct FileList_s* children;
    FortuneHeader header;
} FileList;

static FileList* newFile();
static FileList* gatherFiles(int argc, char** argv);
static FileList* pickFile(FileList*, bool equiprobable);
static FileList* addFile(FileList*, char* path, float percent);
static FileList* addFolder(FileList*, char* path, float percent);
static bool distributeProbability(FileList*, bool equiprobable);
static void freeFileList(FileList*);
static bool isFile(char* path);
static bool isFolder(char* path);
static bool readPercent(char* buf, int* percent);
static bool seek(int fd, off_t offset, bool seekable);
static void help(FILE* stream);
static void showVersion(FILE* stream);

bool _(PrintCookie)(int argc, char** argv)
{
    bool to_help = false;
    bool equiprobable = false;
    int c;
    while ((c = getopt(argc, argv, "eh")) != -1) {
        switch (c) {
        case 'e': equiprobable = true; break;
        case 'h': to_help = true; break;
        default: return false;
        }
    }
    if (to_help) {
        help(optind > 2 ? stderr : stdout);
        exit(optind > 2 ? EXIT_FAILURE : EXIT_SUCCESS);
    }
    argc -= optind;
    argv += optind;

    srandomdev();

    FileList* files = gatherFiles(argc, argv);
    if (!distributeProbability(files, equiprobable)) {
        freeFileList(files);
        return false;
    }
    FileList* file = pickFile(files, equiprobable);
    if (!file) {
        L_ERROR("%s\n", "No cookies found");
        freeFileList(files);
        return false;
    }

    uint32_t index = random() % file->header.total;
    char* cookie = _(GetCookieFromPath)(file->path, file->data, index,
            &(file->header));
    freeFileList(files);
    if (cookie) {
        printf("%s", cookie);
        FORTUNE_FREE(cookie);
        return true;
    } else {
        L_ERROR("Failed to fetch cookies from: %s\n", file->path);
        return false;
    }
}

char* _(GetCookieFromFd)(int in, int idx, FortuneIndex index,
        FortuneHeader* header)
{
    if (read(idx, header, sizeof(*header)) != sizeof(*header)) return NULL;
    header->version = be16toh(header->version);
    header->total = be32toh(header->total);
    if (header->version != FORTUNE_DATA_VERSION) return NULL;
    if (index >= header->total) return NULL;

    FortuneIndex offset = -1;
    bool seekable_in = lseek(in, 0, SEEK_CUR) != -1;
    bool seekable_idx = lseek(idx, 0, SEEK_CUR) != -1;

    offset = sizeof(*header) + index * sizeof(FortuneIndex);
    if (!seek(idx, offset, seekable_idx)) return NULL;
    if (read(idx, &offset, sizeof(offset)) != sizeof(offset)) return NULL;
    offset = be64toh(offset);
    if (!seek(in, offset, seekable_in)) return NULL;

    char* cookie = NULL;
    size_t length = 0;
    char delim = header->delim;
    bool commented = header->flags & FORTUNE_DATA_COMMENT;
    bool rotated = header->flags & FORTUNE_DATA_ROTATED;

    char buf[BUF_SIZE+1];
    size_t line_length = 0;
    bool in_comment = false;
    ssize_t nbyte = 0;
    while ((nbyte = fdgets(in, buf, BUF_SIZE)) > 0) {
        line_length += nbyte;
        if (line_length <= BUF_SIZE /* near the previous newline */ &&
                nbyte == 2 && buf[0] == delim && buf[1] == '\n') {
            break;
        }
        if (commented) {
            if (in_comment ||
                    (line_length <= BUF_SIZE /* near the previous newline */ &&
                     nbyte >= 2 && buf[0] == delim && buf[1] == delim)) {
                in_comment = buf[nbyte-1] != '\n';
                if (buf[nbyte-1] == '\n') line_length = 0;
                continue;
            }
            if (buf[nbyte-1] == '\n') line_length = 0;
        }
        length += nbyte;
        FORTUNE_ALLOC(cookie, length + 1);
        memcpy(cookie + length - nbyte, buf, nbyte);
        cookie[length] = '\0';
    }
    if (nbyte == -1) {
        if (cookie) FORTUNE_FREE(cookie);
        return NULL;
    }

    if (cookie && rotated) rotate(cookie, length);
    return cookie;
}

char* _(GetCookieFromFile)(FILE* in, FILE* idx, FortuneIndex index,
                           FortuneHeader* header)
{
    return _(GetCookieFromFd)(fileno(in), fileno(idx), index, header);
}

char* _(GetCookieFromPath)(char* in, char* idx, FortuneIndex index,
                           FortuneHeader* header)
{
    bool from_stdin = !strcmp(in, "-");
    bool from_stdin2 = !strcmp(idx, "-");

    FILE* fin = from_stdin ? stdin : fopen(in, "r");
    if (!fin) {
        P_ERROR("Failed to read \"%s\"", from_stdin ? "stdin" : in);
        return NULL;
    }
    FILE* fidx = from_stdin2 ? stdin : fopen(idx, "r");
    if (!fidx) {
        P_ERROR("Failed to read index \"%s\"", from_stdin2 ? "stdin" : idx);
        if (!from_stdin) fclose(fin);
        return NULL;
    }

    char* cookie = _(GetCookieFromFile)(fin, fidx, index, header);
    if (!from_stdin) fclose(fin);
    if (!from_stdin2) fclose(fidx);
    return cookie;
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

static void freeFileList(FileList* files)
{
    FileList* next = NULL;
    while (files) {
        if (files->children) freeFileList(files->children);
        free(files->path);
        free(files->data);
        next = files->next;
        free(files);
        files = next;
    }
}

static bool isFile(char* path)
{
    struct stat buf;
    if (stat(path, &buf) == -1) return false;
    return buf.st_mode & S_IFREG;
}

static bool isFolder(char* path)
{
    struct stat buf;
    if (stat(path, &buf) == -1) return false;
    return buf.st_mode & S_IFDIR;
}

static FileList* newFile()
{
    FileList* file = malloc(sizeof(FileList));
    if (file) memset(file, 0, sizeof(FileList));
    return file;
}

static FileList* addFile(FileList* files, char* path, float percent)
{
    if (!isFile(path)) {
        P_ERROR("Cannot find file \"%s\"", path);
        return NULL;
    }
    FileList* file = newFile();
    assert(file);
    file->path = strdup(path);
    assert(file->path);
    file->percent = percent;

    file->data = addSuffix(path, ".dat");
    assert(file->data);
    if (!isFile(file->data)) {
        P_ERROR("Cannot find index file \"%s\"", file->data);
        freeFileList(file);
        return NULL;
    }
    FortuneHeader* header = &file->header;
    FILE* fin = NULL;
    if (!(fin = fopen(file->data, "r")) ||
            read(fileno(fin), header, sizeof(*header)) != sizeof(*header)) {
        P_ERROR("Cannot read index file \"%s\"", file->data);
        if (fin) fclose(fin);
        freeFileList(file);
        return NULL;
    }
    file->header.total = be32toh(file->header.total);

    if (files) files->next = file;
    return file;
}

static FileList* addFolder(FileList* files, char* path, float percent)
{
    if (!isFolder(path)) {
        P_ERROR("Cannot find folder \"%s\"", path);
        return NULL;
    }
    FileList* folder = newFile();
    assert(folder);
    folder->path = strdup(path);
    assert(folder->path);
    folder->percent = percent;

    DIR* dir = opendir(path);
    if (!dir) {
        P_ERROR("Failed to open folder \"%s\"", path);
        freeFileList(folder);
        return NULL;
    }

    FileList* file = NULL;
    struct dirent* entry = NULL;
    while ((entry = readdir(dir))) {
        char* filepath = joinPath(path, entry->d_name);
        assert(filepath);
        if (isFolder(filepath) || endsWith(filepath, ".dat")) {
            free(filepath);
            continue;
        }
        if (!folder->children) {
            file = folder->children = addFile(NULL, filepath, -1);
        } else {
            file = addFile(file ? file : folder->children, filepath, -1);
        }
        if (file) folder->header.total += file->header.total;
        free(filepath);
    }
    closedir(dir);
    if (!folder->children) {
        freeFileList(folder);
        return NULL;
    }

    if (files) files->next = folder;
    return folder;
}

static bool readPercent(char* buf, int* percent)
{
    int n = 0;
    char* p = buf;
    while ('0' <= *p && *p <= '9') n = *(p++) - '0' + n * 10;
    if (!(*p == '%' && *(p+1) == '\0')) return false;
    *percent = n;
    return true;
}

static FileList* gatherFiles(int argc, char** argv)
{
    FileList* files = NULL;
    if (argc > 0) {
        FileList* tail = NULL;
        for (int i = 0; i < argc; i++) {
            int percent = -1;
            if (endsWith(argv[i], "%")) {
                if (!readPercent(argv[i], &percent)) {
                    L_ERROR("Non-integral percent number: %s\n", argv[i]);
                    return NULL;
                }
                if (i + 1 >= argc) {
                    L_ERROR("Missing file/folder after percent: %s\n", argv[i]);
                    return NULL;
                }
                i++;
            }
            if (isFile(argv[i])) {
                tail = addFile(tail, argv[i], percent);
            } else if (isFolder(argv[i])) {
                tail = addFolder(tail, argv[i], percent);
            } else {
                L_ERROR("Invalid file: %s\n", argv[i]);
                return NULL;
            }
            if (!files) files = tail;
        }
    } else {
        files = addFolder(files, FORTUNE_DATA_DIR, 100.0);
    }
    return files;
}

static bool distributeProbability(FileList* files, bool equiprobable)
{
    float percent = 100.0f;
    size_t num_file = 0;
    uint64_t num_cookie = 0;
    for (FileList* p = files; p; p = p->next) {
        if (p->percent < 0) {
            num_file++;
            num_cookie += p->header.total;
            continue;
        }
        if (percent < p->percent) {
            L_ERROR("Remaining probability is less than %.2f%%\n", p->percent);
            return false;
        }
        percent -= p->percent;
    }
    if (num_file > 0) {
        for (FileList* p = files; p; p = p->next) {
            if (p->percent >= 0) continue;
            if (equiprobable) {
                p->percent = percent / num_file;
            } else {
                p->percent = percent * ((float)p->header.total / num_cookie);
            }
        }
    }
    return true;
}

static FileList* pickFile(FileList* files, bool equiprobable)
{
    float percent = 1.0 * (random() % 100);
    for (FileList* p = files; p; p = p->next) {
        if (!p->next || percent <= p->percent) {
            if (p->data) return p;
            if (distributeProbability(p->children, false)) {
                return pickFile(p->children, false);
            } else {
                return NULL;
            }
        }
        percent -= p->percent;
    }
    return NULL;
}

static void help(FILE* stream)
{
    fprintf(stream,
            "Usage:\n"
            "    fortune [OPTIONS] [[N%%] <FILE|FOLDER>]...\n"
            "\n"
            "Options:\n"
            "\n"
            "    -e       Distribute probability equally.\n"
            "    -h       Show this manual.\n"
            "\n"
            "    You can specify multiple files and folders to get cookies.\n"
            "    Each file should reside with its index file <filename>.dat.\n"
            "    A percentage N%% changes the probability of the file/folder.\n"
            "    The remaining probability will be distributed automatically.\n"
            "\n"
            "Other:\n"
            "\n"
            "    --index [-h] [-d <char>] [-cxs] <input> [output]\n"
            "    --dump [-h] [-d <char>] [-cxs] <input> [output] [data]\n"
            "    --version\n"
            "    --help\n"
           );
}

static void showVersion(FILE* stream)
{
    fprintf(stream,
            "fortune, version %d.%d.%d\n"
            "Copyright (C) 2016 Jak Wings https://github.com/jakwings/fortune\n"
            , FORTUNE_VERSION_X, FORTUNE_VERSION_Y, FORTUNE_VERSION_Z);
}

#ifndef FORTUNE_MAKE_LIB
int main(int argc, char** argv)
{
    bool done = false;
    if (argc > 1 && !strcmp("--index", argv[1])) {
        done = _(CreateData)(argc - 1, argv + 1);
    } else if (argc > 1 && !strcmp("--dump", argv[1])) {
        done = _(DumpData)(argc - 1, argv + 1);
    } else if (argc > 1 && !strcmp("--version", argv[1])) {
        showVersion(stdout);
        return EXIT_SUCCESS;
    } else if (argc > 1 && !strcmp("--help", argv[1])) {
        help(stdout);
        return EXIT_SUCCESS;
    } else {
        done = _(PrintCookie)(argc, argv);
    }

    if (done) {
        return EXIT_SUCCESS;
    } else {
        help(stderr);
        return EXIT_FAILURE;
    }
}
#endif /* FORTUNE_MAKE_LIB */
