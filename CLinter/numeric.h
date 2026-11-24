#ifndef CLINTER_NUMERIC_H
#define CLINTER_NUMERIC_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    NUM_ROUND, NUM_TRUNCATE, NUM_FLOOR, NUM_EXACT
} NumericPolicy;
/* Return 1 on success, 0 on failure. Output is unchanged on failure.
 * Integer bounds apply after conversion. */
int numeric_finite(double value);
int numeric_range(double value, double minimum, double maximum);
int numeric_i32(double value, NumericPolicy policy, int32_t minimum,
                int32_t maximum, int32_t *out);
int numeric_i64(double value, NumericPolicy policy, int64_t minimum,
                int64_t maximum, int64_t *out);
/* Full decimal token, optional surrounding ASCII whitespace, optional exponent.
 * Uses the C numeric locale independently of the process locale. */
int numeric_parse(const char *text, double *out);
/* Round-trip decimal representation in the C numeric locale. */
int numeric_format(double value, char *out, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
