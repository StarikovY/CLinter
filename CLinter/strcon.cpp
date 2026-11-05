#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "parse.h"
#include "runtime.h"
#include "strcon.h"
#include "keyboard.h"

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

/* R3/R4: shared string primitive evaluation; 1-based positions. */
int parse_string_primitive(Lexer* lx, SBuf* sb) {
    char fn[32], source[4096]; SBuf inner;
    int start = 1, count = 0, has_count = 0, length, offset;
    if (lx->cur.type != T_IDENT) return 0;
    strncpy(fn, lx->cur.text, sizeof(fn)-1); fn[sizeof(fn)-1]=0;
    if (strcmp(fn,"LEFT$") && strcmp(fn,"RIGHT$") && strcmp(fn,"MID$") && strcmp(fn,"SEG$")) return 0;
    lx_next(lx);
    if (lx->cur.type != T_LPAREN) return 1;
    lx_next(lx);
    sbuf_init(&inner, source, sizeof(source));
    parse_string_expr_to_buf(lx, &inner);
    if (lx->cur.type == T_COMMA) {
        lx_next(lx);
        start = (int)parse_rel(lx);
        if (lx->cur.type == T_COMMA) { lx_next(lx); count=(int)parse_rel(lx); has_count=1; }
    }
    if (lx->cur.type == T_RPAREN) lx_next(lx);
    length=(int)strlen(source);
    if (!strcmp(fn,"LEFT$")) { offset=0; count=start; }
    else if (!strcmp(fn,"RIGHT$")) { count=start; if(count<0) count=0; if(count>length) count=length; offset=length-count; }
    else { if(start<1) start=1; offset=start-1; if(offset>length) offset=length;
           if (!has_count) count=length-offset; }
    if(count<0) count=0;
    if(count>length-offset) count=length-offset;
    sbuf_cat_n(sb, source+offset, (size_t)count);
    return 1;
}

void parse_string_term_to_buf(Lexer* lx, SBuf* sb) {
    if (lx->cur.type == T_IDENT && !strcmp(lx->cur.text, "INKEY$")) {
        int ch;
        lx_next(lx);
        if (lx->cur.type == T_LPAREN) {
            lx_next(lx);
            if (lx->cur.type == T_RPAREN) lx_next(lx);
        }
        ch = clinter_inkey();
        if (ch > 0 && ch < 256) { char c = (char)ch; sbuf_cat_n(sb, &c, 1); }
        return;
    }

    if (parse_string_primitive(lx, sb)) return;
    if (lx->cur.type == T_IDENT && !strcmp(lx->cur.text, "IF$")) {
        double condition;
        char yes[4096], no[4096]; SBuf a, b;
        lx_next(lx);
        if (lx->cur.type != T_LPAREN) return;
        lx_next(lx); condition = parse_rel(lx);
        if (lx->cur.type != T_COMMA) return;
        lx_next(lx); sbuf_init(&a, yes, sizeof(yes));
        parse_string_expr_to_buf(lx, &a);
        if (lx->cur.type != T_COMMA) return;
        lx_next(lx); sbuf_init(&b, no, sizeof(no));
        parse_string_expr_to_buf(lx, &b);
        if (lx->cur.type == T_RPAREN) lx_next(lx);
        sbuf_cat(sb, condition != 0.0 ? yes : no);
        return;
    }
    if (lx->cur.type == T_STRING) {
        sbuf_cat(sb, lx->cur.text);
        lx_next(lx);
    }
    else if (lx->cur.type == T_NUMBER) {
        sbuf_cat_num(sb, lx->cur.number);
        lx_next(lx);
    }
    else if (lx->cur.type == T_IDENT &&
             (!strcmp(lx->cur.text, "UPPER$") || !strcmp(lx->cur.text, "LOWER$"))) {
        int upper = !strcmp(lx->cur.text, "UPPER$");
        lx_next(lx);
        if (lx->cur.type != T_LPAREN) return;
        lx_next(lx);
        char tmp[1024]; SBuf inner;
        sbuf_init(&inner, tmp, sizeof(tmp));
        parse_string_expr_to_buf(lx, &inner);
        if (lx->cur.type == T_RPAREN) lx_next(lx);
        for (size_t k=0; k<inner.len; ++k)
            tmp[k] = (char)(upper ? toupper((unsigned char)tmp[k]) : tolower((unsigned char)tmp[k]));
        sbuf_cat(sb, tmp);
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
