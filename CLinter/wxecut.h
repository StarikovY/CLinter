
#ifndef WXECUT_H
#define WXECUT_H
#include "runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

int exec_statement(const char *src, int duringRun, int currentLine, int *outJump);

#ifdef __cplusplus
}
#endif

#endif
