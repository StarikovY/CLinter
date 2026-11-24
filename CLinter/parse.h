
#ifndef PARSE_H
#define PARSE_H
#include "runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

void lx_init(Lexer *lx, const char *s);

void lx_next(Lexer *lx);

double parse_rel(Lexer *lx);
void parse_numeric_reset(void);
int parse_numeric_failed(void);
double parse_numeric_error(void);
int checked_parameter(double value);
int parse_subscript(Lexer *lx);
int parse_dimension(Lexer *lx);

#ifdef __cplusplus
}
#endif
#endif
