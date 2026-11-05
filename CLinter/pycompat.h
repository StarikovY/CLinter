#ifndef PYCOMPAT_H
#define PYCOMPAT_H
#include "runtime.h"
#ifdef __cplusplus
extern "C" {
#endif
void pycompat_seed(unsigned int seed);
double pycompat_rnd(void);
double pycompat_rndint(double low, double high);
double pycompat_round(double x, int digits);
double pycompat_mod(double x, double y);
int pycompat_randomize_statement(Lexer *lx);
#ifdef __cplusplus
}
#endif
#endif
