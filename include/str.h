#ifndef FORTUNE_STR_H
#define FORTUNE_STR_H 1

#include <stdbool.h>
#include <stdio.h>
#include "data.h"

#define _(cookie) fortune ## cookie
bool _(CreateData)(int argc, char** argv);
bool _(CreateDataFromFd)(int in, int out, FortuneHeader*);
bool _(CreateDataFromFile)(FILE* in, FILE* out, FortuneHeader*);
bool _(CreateDataFromPath)(char* in, char* out, FortuneHeader*);
#undef _

#endif /* FORTUNE_STR_H */
