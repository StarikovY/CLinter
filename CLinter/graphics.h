#ifndef CLINTER_GRAPHICS_H
#define CLINTER_GRAPHICS_H
#include "runtime.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
int graphics_mode(int mode);
int graphics_active(void);
void graphics_clear(void);
void graphics_color(int foreground, int background);
void graphics_border(int color);
void graphics_locate(int col, int row);
void graphics_text(const char *text, size_t count);
int graphics_statement(Lexer *lx);
int graphics_point(int x, int y);
void graphics_shutdown(void);
/* 0=SDL input completed, 1=console fallback, -1=window closed. */
int graphics_input(const char *prompt, char *out, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
