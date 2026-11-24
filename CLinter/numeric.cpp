#if !defined(_WIN32) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include "numeric.h"
#include <locale.h>
#if defined(__APPLE__)
#include <xlocale.h>
#endif
#include <math.h>
#include <float.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int numeric_finite(double value) { return isfinite(value) != 0; }
int numeric_range(double value, double minimum, double maximum) {
    return numeric_finite(value) && numeric_finite(minimum) &&
           numeric_finite(maximum) && minimum <= value && value <= maximum;
}
static int convert(double value, NumericPolicy policy, double *out) {
    if (!numeric_finite(value)) return 0;
    switch (policy) {
    case NUM_ROUND: value = round(value); break;
    case NUM_TRUNCATE: value = trunc(value); break;
    case NUM_FLOOR: value = floor(value); break;
    case NUM_EXACT: if (trunc(value) != value) return 0; break;
    default: return 0;
    }
    *out = value;
    return 1;
}
int numeric_i64(double value, NumericPolicy policy, int64_t minimum,
                int64_t maximum, int64_t *out) {
    double v;
    int64_t result;
    /* INT64_MAX rounds to 2^63 as a double; use exclusive upper bound. */
    if (!out || minimum > maximum || !convert(value, policy, &v) ||
        v < -9223372036854775808.0 || v >= 9223372036854775808.0) return 0;
    result = (int64_t)v;
    if (result < minimum || result > maximum) return 0;
    *out = result;
    return 1;
}
int numeric_i32(double value, NumericPolicy policy, int32_t minimum,
                int32_t maximum, int32_t *out) {
    int64_t result;
    if (!out || !numeric_i64(value, policy, minimum, maximum, &result)) return 0;
    *out = (int32_t)result;
    return 1;
}
static int space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}
static int digit(char c) { return c >= '0' && c <= '9'; }
int numeric_parse(const char *text, double *out) {
    const char *p, *start, *finish;
    char *end;
    int digits = 0, nonzero = 0;
    double value;
    if (!text || !out) return 0;
    p = text;
    while (space(*p)) ++p;
    start = p;
    if (*p == '+' || *p == '-') ++p;
    while (digit(*p)) { digits = 1; nonzero |= *p != '0'; ++p; }
    if (*p == '.') {
        ++p;
        while (digit(*p)) { digits = 1; nonzero |= *p != '0'; ++p; }
    }
    if (!digits) return 0;
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '+' || *p == '-') ++p;
        if (!digit(*p)) return 0;
        while (digit(*p)) ++p;
    }
    finish = p;
    while (space(*p)) ++p;
    if (*p) return 0;
#ifdef _WIN32
    _locale_t locale = _create_locale(LC_NUMERIC, "C");
    if (!locale) return 0;
    errno = 0;
    value = _strtod_l(start, &end, locale);
    int parse_errno = errno;
    _free_locale(locale);
#else
    locale_t locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    if (!locale) return 0;
    errno = 0;
    value = strtod_l(start, &end, locale);
    int parse_errno = errno;
    freelocale(locale);
#endif
    /* Allow representable subnormals, reject overflow and underflow to zero. */
    if (end != finish || !numeric_finite(value) ||
        (parse_errno == ERANGE && fabs(value) >= DBL_MAX) ||
        (value == 0.0 && nonzero)) return 0;
    *out = value;
    return 1;
}
int numeric_format(double value, char *out, size_t capacity) {
    char buffer[64];
    int n;
    if (!out || !numeric_finite(value)) return 0;
#ifdef _WIN32
    _locale_t locale = _create_locale(LC_NUMERIC, "C");
    if (!locale) return 0;
    n = _snprintf_l(buffer, sizeof(buffer), "%.17g", locale, value);
    _free_locale(locale);
#else
    locale_t locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    if (!locale) return 0;
    locale_t previous = uselocale(locale);
    if (!previous) { freelocale(locale); return 0; }
    n = snprintf(buffer, sizeof(buffer), "%.17g", value);
    uselocale(previous);
    freelocale(locale);
#endif
    if (n < 0 || (size_t)n >= sizeof(buffer) || (size_t)n >= capacity) return 0;
    memcpy(out, buffer, (size_t)n + 1);
    return 1;
}
