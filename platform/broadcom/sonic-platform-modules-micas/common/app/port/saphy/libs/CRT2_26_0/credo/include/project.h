#ifndef PROJECT_H
#define PROJECT_H

#define QUOTEME(x)      QUOTEME_1(x)
#define QUOTEME_1(x)    #x
#define INCLUDE_FILE(x) QUOTEME(x.h)

#include INCLUDE_FILE(PROJECT)

#endif
