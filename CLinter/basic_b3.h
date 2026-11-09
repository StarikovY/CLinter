#ifndef CLINTER_BASIC_B3_H
#define CLINTER_BASIC_B3_H
#include "runtime.h"
#ifdef __cplusplus
extern "C" {
#endif
void b3_reset(void);
int b3_statement(Lexer *lx, int duringRun, int currentLine, int *jump);
int b3_error_target(void);
int b3_handle_error(int line);
int b3_resume_target(int next);
int b3_default_type(const char *name);
#ifdef __cplusplus
}
#endif
#endif
