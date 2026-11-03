#ifndef STRCON_H
#define STRCON_H

#include "parse.h"
#include "runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* buf;
    size_t cap;
    size_t len;
} SBuf;

void sbuf_init(SBuf* sb, char* buf, size_t cap);
void sbuf_cat(SBuf* sb, const char* s);
void sbuf_cat_n(SBuf* sb, const char* s, size_t n);
void sbuf_cat_num(SBuf* sb, double x);

int parse_string_primitive(Lexer* lx, SBuf* sb);
int parse_string_primitive(Lexer* lx, SBuf* sb);
void parse_string_term_to_buf(Lexer* lx, SBuf* sb);
void parse_string_expr_to_buf(Lexer* lx, SBuf* sb);

#ifdef __cplusplus
}
#endif

#endif // STRCON_H
