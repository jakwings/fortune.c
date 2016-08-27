#ifndef FORTUNE_ROTATE_H
#define FORTUNE_ROTATE_H 1

#include <stdbool.h>

#define _(cookie) fortune ## cookie
char* _(Rotate)(char* s, size_t n);
bool _(RotateData)(int argc, char** argv);
#undef _

#endif /* FORTUNE_ROTATE_H */
