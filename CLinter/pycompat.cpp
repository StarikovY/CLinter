/* PYLinter numeric compatibility extensions (classic C). */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include "pycompat.h"
#include "parse.h"

static int py_seeded = 0;
void pycompat_seed(unsigned int seed) { srand(seed); py_seeded = 1; }
double pycompat_rnd(void) {
    if (!py_seeded) pycompat_seed(1);
    return (double)rand() / ((double)RAND_MAX + 1.0);
}
double pycompat_rndint(double low, double high) {
    double a = ceil(low), b = floor(high), range;
    if (!isfinite(a) || !isfinite(b) || a > b) return 0.0;
    range = b - a + 1.0;
    return a + floor(pycompat_rnd() * range);
}
double pycompat_round(double x, int digits) {
    double scale;
    if (!isfinite(x) || digits > 15 || digits < -15) return x;
    scale = pow(10.0, (double)digits);
    return round(x * scale) / scale;
}
double pycompat_mod(double x, double y) {
    long a, b;
    if (!isfinite(x) || !isfinite(y) || y == 0.0 ||
        x >= (double)LONG_MAX || x < (double)LONG_MIN ||
        y >= (double)LONG_MAX || y < (double)LONG_MIN) return 0.0;
    a = (long)x; b = (long)y;
    if (b == 0 || (a == LONG_MIN && b == -1)) return 0.0;
    return (double)(a % b);
}
int pycompat_randomize_statement(Lexer *lx) {
    unsigned int seed;
    lx_next(lx);
    if (lx->cur.type == T_END) seed = (unsigned int)time(NULL);
    else {
        double value = parse_rel(lx);
        if (!isfinite(value)) { printf("ERROR: RANDOMIZE seed\n"); return -1; }
        seed = (unsigned int)fmod(fabs(trunc(value)), 4294967296.0);
    }
    if (lx->cur.type != T_END) { printf("ERROR: RANDOMIZE syntax\n"); return -1; }
    pycompat_seed(seed);
    return 0;
}
