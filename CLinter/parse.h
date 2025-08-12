
#ifndef PARSE_H
#define PARSE_H
#include "runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

void lx_init(Lexer *lx, const char *s);

void lx_next(Lexer *lx);

double parse_rel(Lexer *lx);

#ifdef __cplusplus
}
#endif
#endif
