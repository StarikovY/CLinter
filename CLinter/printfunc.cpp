/* printfunc.cpp — PRINT statement implementation split out of wxecut.cpp
   - Supports: PRINT, PRINT #n, string concatenation with '+',
               CHR$/STR$/SEG$/TRM$, string vars & string arrays,
               TAB(n) spacing, trailing ';' / ',' newline suppression.
   - Now binary-safe: CHR$(n) for any 0..255 prints correctly (no C-string truncation).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "runtime.h"
#include "parse.h"
#include "printfunc.h"

#ifndef PRINT_ZONE
#define PRINT_ZONE 14
#endif

int g_print_col = 0;   /* 0-based column */

int g_print_suppress_nl = 0;   /* start with normal behavior */

/* ----- File handle helper (uses your FileSlot array) ----- */
static FILE* pf_file_from_handle(int n) {
    if (n < 0 || n >= MAX_FILES) return NULL;
    if (!g_files[n].used) return NULL;
    return g_files[n].fp;
}

/* Return 1 if the next non-space char in the original source
   is a statement separator ':' or '\' or end-of-string. */
int lx_peek_stmt_sep(const Lexer* lx) {
    size_t j = lx->i;
    while (lx->s[j] && isspace((unsigned char)lx->s[j])) j++;
    int c = (int)lx->s[j];
    int ret = 0;

    if (c == ((int)':'))
        ret = 1;

    if (c == ((int)'\\'))
        ret = 1;

    if (c == ((int)'\0'))
        ret = 1;

    return ret;
}

/* ---- local helpers (binary-safe buffer) ---- */
/* Return 1 if the next non-space char in the ORIGINAL source line
   (from current lexer position) is ':' or '\' or end-of-string.
   This lets PRINT stop at statement separators even if the lexer
   doesn't tokenize them. */

typedef struct {
    unsigned char* data;
    size_t len;
    size_t cap;
} ByteBuf;

static void bb_init(ByteBuf* b, unsigned char* storage, size_t cap) {
    b->data = storage;
    b->len = 0;
    b->cap = cap;
}

static void bb_putc(ByteBuf* b, unsigned char ch) {
    if (b->len < b->cap) b->data[b->len++] = ch;
}

static void bb_append(ByteBuf* b, const void* src, size_t n) {
    if (!src || n == 0) return;
    size_t room = (b->cap > b->len) ? (b->cap - b->len) : 0;
    if (n > room) n = room;
    if (n) { memcpy(b->data + b->len, src, n); b->len += n; }
}

static void bb_append_cstr(ByteBuf* b, const char* s) {
    if (!s) return;
    bb_append(b, s, strlen(s));
}
/* convenience: append numeric as text (ASCII) */
static void bb_append_num(ByteBuf* b, double v) {
    char tmp[64];
#ifdef _MSC_VER
    _snprintf(tmp, sizeof(tmp), "%.15g", v);
#else
    snprintf(tmp, sizeof(tmp), "%.15g", v);
#endif
    bb_append(b, tmp, strlen(tmp));
}

/* Parse one PRINT term and append its textual bytes into ByteBuf.
   Consumes tokens for the term. Sets *is_string = 1 if result is string-ish (affects '+' behavior). */
/* ----- Parse a single PRINT term, append its textual BYTES, note if string-ish ----- */
/* Sets *is_string = 1 if term is naturally string (affects '+' concat vs numeric add). */
static void parse_print_term_to_bb(Lexer* lx, ByteBuf* bb, int* is_string) {
    if (is_string) *is_string = 0;

    /* string literal */
    if (lx->cur.type == T_STRING) {
        bb_append_cstr(bb, lx->cur.text);
        if (is_string) *is_string = 1;
        lx_next(lx);
        return;
    }

    /* string identifiers / functions (CHR$/STR$/SEG$/TRM$), string vars/arrays */
    if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
        char nbuf[64]; strncpy(nbuf, lx->cur.text, sizeof(nbuf) - 1); nbuf[sizeof(nbuf) - 1] = 0;
        lx_next(lx);

        /* CHR$(n) -> single byte (only if >= 32, per your current design) */
        if (_stricmp(nbuf, "CHR$") == 0) {
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                double v = parse_rel(lx);
                int code = (int)v;
                if (code < 0)   code = 0;
                if (code > 255) code = 255;
                if (code >= 32) {
                    bb_putc(bb, (unsigned char)code);
                }
            }
            if (is_string) *is_string = 1;
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return;
        }

        /* STR$(x) -> textual number */
        if (_stricmp(nbuf, "STR$") == 0) {
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            { double v = parse_rel(lx); bb_append_num(bb, v); }
            if (is_string) *is_string = 1;
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return;
        }

        /* SEG$(s$, start[, len]) -> substring (1-based) */
        if (_stricmp(nbuf, "SEG$") == 0) {
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                const char* s = ""; int start = 1, len = 0;
                if (lx->cur.type == T_STRING) { s = lx->cur.text; lx_next(lx); }
                else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
                    Variable* v = find_var(lx->cur.text);
                    s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(lx);
                }
                if (lx->cur.type == T_COMMA) { lx_next(lx); start = (int)parse_rel(lx); }
                if (lx->cur.type == T_COMMA) { lx_next(lx); len = (int)parse_rel(lx); }
                int sl = (int)strlen(s);
                int i0 = start < 1 ? 0 : start - 1; if (i0 > sl) i0 = sl;
                int l = (len > 0 ? len : (sl - i0)); if (i0 + l > sl) l = sl - i0; if (l < 0) l = 0;
                bb_append(bb, s + i0, (size_t)l);
            }
            if (is_string) *is_string = 1;
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return;
        }

        /* TRM$(s$) -> trim spaces */
        if (_stricmp(nbuf, "TRM$") == 0) {
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                const char* s = "";
                if (lx->cur.type == T_STRING) { s = lx->cur.text; lx_next(lx); }
                else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
                    Variable* v = find_var(lx->cur.text);
                    s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(lx);
                }
                size_t n = strlen(s), a = 0, b = n;
                while (a < b && isspace((unsigned char)s[a])) a++;
                while (b > a && isspace((unsigned char)s[b - 1])) b--;
                bb_append(bb, s + a, b - a);
            }
            if (is_string) *is_string = 1;
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return;
        }

        /* string var or array element */
        if (lx->cur.type == T_LPAREN) {
            int subs[MAX_DIMS], nsubs = 0; SArray* sa = sarray_find(nbuf);
            lx_next(lx);
            while (lx->cur.type != T_RPAREN && lx->cur.type != T_END && !lx_peek_stmt_sep(lx)) {
                if (nsubs >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); break; }
                subs[nsubs++] = (int)parse_rel(lx);
                if (lx->cur.type == T_COMMA) { lx_next(lx); if (lx_peek_stmt_sep(lx)) break; continue; }
                else break;
            }
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            { const char* s = sa ? sarray_get(sa, subs, nsubs) : ""; bb_append_cstr(bb, s); }
        }
        else {
            Variable* v = find_var(nbuf);
            { const char* s = (v && v->type == VT_STR && v->str) ? v->str : ""; bb_append_cstr(bb, s); }
        }
        if (is_string) *is_string = 1;
        return;
    }

    /* Default: numeric expression -> text */
    { double v = parse_rel(lx); bb_append_num(bb, v); }
}

#ifdef NEED
/* treat statement separators as end-of-statement for PRINT */
static inline int is_stmt_end_token(int t) {
    /* Adjust token names if yours differ: e.g., T_COLON / T_BSLASH / T_BACKSLASH */
    return (t == T_END) || (t == T_COLON) || (t == T_BSLASH);
}
#endif

/* Return 1 if the next non-space char in the original source
   is a statement separator ':' or '\' or end-of-string. */

/* Public: execute PRINT statement */

int exec_print(Lexer* lx) {
    FILE* out = stdout;
    int suppress_nl = 0;   /* local to this PRINT only */

    /* must start at PRINT */
    if (lx->cur.type != T_PRINT) return -1;
    lx_next(lx);

    /* Optional: PRINT #n, ... */
    if (lx->cur.type == T_HASH) {
        lx_next(lx);
        if (lx->cur.type != T_NUMBER) { printf("ERROR: PRINT # needs handle\n"); return -1; }
        int h = (int)lx->cur.number;
        FILE* fp = NULL;
        if (h >= 0 && h < MAX_FILES && g_files[h].used) fp = g_files[h].fp;
        if (!fp) { printf("ERROR: bad handle\n"); return -1; }
        out = fp;
        lx_next(lx);
        if (lx->cur.type == T_COMMA || lx->cur.type == T_SEMI) lx_next(lx);
    }

    /* main loop: run until end of this statement segment */
//OLD:     while (lx->cur.type != T_END) {
    while (!lx_peek_stmt_sep(lx) && lx->cur.type != T_END && lx->cur.type != T_ELSE) {
        /* TAB(n) absolute column (1-based) */
        if (lx->cur.type == T_IDENT) {
            char id[16]; strncpy(id, lx->cur.text, 15); id[15] = 0;
            for (int i = 0; id[i]; ++i) id[i] = (char)toupper((unsigned char)id[i]);
            if (!strcmp(id, "TAB")) {
                lx_next(lx);
                if (lx->cur.type == T_LPAREN) lx_next(lx);
                {
                    int n = (int)parse_rel(lx);
                    if (n < 1) n = 1;
                    int target = n - 1; /* 0-based */
                    while (g_print_col < target) { fputc(' ', out); g_print_col++; }
                }
                if (lx->cur.type == T_RPAREN) lx_next(lx);

                // right after emitting the item (before handling , or ;)
                if (lx->cur.type == T_ELSE) break;

                if (lx->cur.type == T_COMMA) {
                    suppress_nl = 1; lx_next(lx);
                    int nextZone = ((g_print_col / PRINT_ZONE) + 1) * PRINT_ZONE;
                    while (g_print_col < nextZone) { fputc(' ', out); g_print_col++; }
                    if (lx_peek_stmt_sep(lx) || lx->cur.type == T_END || lx->cur.type == T_ELSE) break;
                    continue;
                }
                if (lx->cur.type == T_SEMI) {
                    suppress_nl = 1; lx_next(lx);
                    if (lx_peek_stmt_sep(lx) || lx->cur.type == T_END || lx->cur.type == T_ELSE) break;
                    continue;
                }
                // if (lx->cur.type == T_END) break;
                if (lx_peek_stmt_sep(lx) || lx->cur.type == T_END || lx->cur.type == T_ELSE) break;
                continue;
            }
        }

        /* Build one item (concat-aware) */
        unsigned char Lbuf[4096]; ByteBuf L; bb_init(&L, Lbuf, sizeof(Lbuf));
        int is_str = 0;
        parse_print_term_to_bb(lx, &L, &is_str);

        /* '+' chain: concat if any side is string-ish; else numeric add */
        while (lx->cur.type == T_PLUS) {
            lx_next(lx);
            unsigned char Rbuf[4096]; ByteBuf R; bb_init(&R, Rbuf, sizeof(Rbuf));
            int rstr = 0;
            parse_print_term_to_bb(lx, &R, &rstr);

            if (is_str || rstr) {
                bb_append(&L, R.data, R.len);
                is_str = 1;
            }
            else {
                char ls[128], rs[128];
                size_t ln = (L.len < sizeof(ls) - 1) ? L.len : (sizeof(ls) - 1);
                size_t rn = (R.len < sizeof(rs) - 1) ? R.len : (sizeof(rs) - 1);
                memcpy(ls, L.data, ln); ls[ln] = 0;
                memcpy(rs, R.data, rn); rs[rn] = 0;
                double sum = atof(ls) + atof(rs);
                L.len = 0; bb_append_num(&L, sum);
            }
        }

        /* Emit item and update column */
        for (size_t i = 0; i < L.len; ++i) {
            unsigned char c = L.data[i];
            fputc((char)c, out);
            if (c == '\n' || c == '\r') g_print_col = 0;
            else                    g_print_col++;
            suppress_nl = 0;
        }

        // right after emitting the item (before handling , or ;)
        if (lx->cur.type == T_ELSE) break;

        /* Optional separators after an item */
        if (lx->cur.type == T_COMMA) {
            suppress_nl = 1;
            lx_next(lx);
            int nextZone = ((g_print_col / PRINT_ZONE) + 1) * PRINT_ZONE;
            while (g_print_col < nextZone) { fputc(' ', out); g_print_col++; }
            if (lx_peek_stmt_sep(lx) || lx->cur.type == T_END || lx->cur.type == T_ELSE) break;
            continue;
        }
        if (lx->cur.type == T_SEMI) {
            suppress_nl = 1;
            lx_next(lx);
            if (lx_peek_stmt_sep(lx) || lx->cur.type == T_END || lx->cur.type == T_ELSE) break;
            continue;
        }

        /* Nothing else recognized -> stop to avoid spinning */
        break;
    }

    /* End-of-statement behavior: print newline unless suppressed in THIS statement */
    if (!suppress_nl) {
        fputc('\n', out);
        g_print_col = 0;
    }
    return 0;
}
