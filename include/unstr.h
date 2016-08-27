#ifndef FORTUNE_UNSTR_H
#define FORTUNE_UNSTR_H 1

#include <stdbool.h>
#include <stdio.h>
#include "data.h"

#define _(cookie) fortune ## cookie
bool _(DumpData)(int argc, char** argv);
bool _(DumpDataFromFd)(int in, int idx, int out, FortuneHeader*);
bool _(DumpDataFromFile)(FILE* in, FILE* idx, FILE* out, FortuneHeader*);
bool _(DumpDataFromPath)(char* in, char* idx, char* out, FortuneHeader*);
#undef _

#endif /* FORTUNE_UNSTR_H */
