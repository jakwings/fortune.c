#ifndef FORTUNE_H
#define FORTUNE_H 1

#include <stdbool.h>
#include <stdio.h>
#include "data.h"

/* X.Y.Z = major.minor.patch */
#define FORTUNE_VERSION_X 2
#define FORTUNE_VERSION_Y 0
#define FORTUNE_VERSION_Z 2

#ifndef FORTUNE_DATA_DIR
#define FORTUNE_DATA_DIR "/usr/share/fortune"
#endif

#define _(cookie) fortune ## cookie
bool _(PrintCookie)(int argc, char** argv);
char* _(GetCookieFromFd)(int in, int idx, FortuneIndex, FortuneHeader*);
char* _(GetCookieFromFile)(FILE* in, FILE* idx, FortuneIndex, FortuneHeader*);
char* _(GetCookieFromPath)(char* in, char* idx, FortuneIndex, FortuneHeader*);
#undef _

#endif /* FORTUNE_H */
