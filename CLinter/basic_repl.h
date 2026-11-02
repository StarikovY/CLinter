#ifndef CLINTER_BASIC_REPL_H
#define CLINTER_BASIC_REPL_H
#include <stddef.h>
/* Returns 1 for input, 0 for EOF; returns -1 for cancelled input. */
int basic_readline(const char *prompt, char *out, size_t capacity);
void basic_history_add(const char *line);
void basic_history_print(int limit);
void basic_history_clear(void);
/* Resolve !N to a prior entry; 1=success, 0=invalid or missing. */
int basic_history_expand(const char *input, char *out, size_t capacity);
#endif
