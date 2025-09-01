#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "parse.h"
#include "runtime.h"
#include "strcon.h"

void sbuf_init(SBuf* sb, char* buf, size_t cap) {
    sb->buf = buf;
    sb->cap = cap;
    sb->len = 0;
    if (cap > 0) sb->buf[0] = 0;
}

void sbuf_cat(SBuf* sb, const char* s) {
    size_t n = strlen(s);
    if (sb->len + n >= sb->cap) n = sb->cap - sb->len - 1;
    if (n > 0) {
        memcpy(sb->buf + sb->len, s, n);
        sb->len += n;
        sb->buf[sb->len] = 0;
    }
}

void sbuf_cat_n(SBuf* sb, const char* s, size_t n) {
    if (sb->len + n >= sb->cap) n = sb->cap - sb->len - 1;
    if (n > 0) {
        memcpy(sb->buf + sb->len, s, n);
        sb->len += n;
        sb->buf[sb->len] = 0;
    }
}

void sbuf_cat_num(SBuf* sb, double x) {
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%g", x);
    sbuf_cat(sb, tmp);
}

void parse_string_term_to_buf(Lexer* lx, SBuf* sb) {
    if (lx->cur.type == T_STRING) {
        sbuf_cat(sb, lx->cur.text);
        lx_next(lx);
    }
    else if (lx->cur.type == T_NUMBER) {
        sbuf_cat_num(sb, lx->cur.number);
        lx_next(lx);
    }
    else if (lx->cur.type == T_IDENT) {
        Variable* v = find_var(lx->cur.text);
        if (v && v->type == VT_STR) sbuf_cat(sb, v->str);
        else if (v && v->type == VT_NUM) sbuf_cat_num(sb, v->num);
        lx_next(lx);
    }
    else {
        // unsupported token: skip
        lx_next(lx);
    }
}

void parse_string_expr_to_buf(Lexer* lx, SBuf* sb) {
    parse_string_term_to_buf(lx, sb);
    while (lx->cur.type == T_PLUS) {
        lx_next(lx);
        parse_string_term_to_buf(lx, sb);
    }
}
