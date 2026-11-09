#ifndef CLINTER_B1B2_H
#define CLINTER_B1B2_H
#include "runtime.h"
#ifdef __cplusplus
extern "C" {
#endif
int b1b2_statement(Lexer *lx, int duringRun, int currentLine, int *jump);
int b1b2_function(const char *name, Lexer *lx, double *value);
void b1b2_reset(void);
#ifdef __cplusplus
}
#endif
#endif
