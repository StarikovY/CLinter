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

int g_print_col = 0;   /* 0-based column */

/* ---- local helpers (binary-safe buffer) ---- */
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

static FILE* pf_file_from_handle(int n) {
    if (n < 0 || n >= MAX_FILES) return NULL;
    if (!g_files[n].used) return NULL;
    return g_files[n].fp;
}

/* Parse one PRINT term and append its textual bytes into ByteBuf.
   Consumes tokens for the term. Sets *is_string = 1 if result is string-ish (affects '+' behavior). */
static void parse_print_term_to_bb(Lexer* lx, ByteBuf* bb, int* is_string) {
    if (is_string) *is_string = 0;

    /* string literal */
    if (lx->cur.type == T_STRING) {
        bb_append(bb, lx->cur.text, strlen(lx->cur.text));
        if (is_string) *is_string = 1;
        lx_next(lx);
        return;
    }

    /* string identifiers / functions (CHR$/STR$/SEG$/TRM$), string vars/arrays */
    if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
        char nbuf[64]; strncpy(nbuf, lx->cur.text, sizeof(nbuf) - 1); nbuf[sizeof(nbuf) - 1] = 0;
        lx_next(lx);

        if (_stricmp(nbuf, "CHR$") == 0) {
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                double v = parse_rel(lx);
                int code = (int)v;
                if (code < 0)   code = 0;
                if (code > 255) code = 255;

                /* Only display printable ASCII (>= 32). Below 32: ignore. */
                if (code >= 32) {
                    bb_putc(bb, (unsigned char)code);
                }
            }
            if (is_string) *is_string = 1;
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return;
        }

        if (_stricmp(nbuf, "STR$") == 0) {
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            { double v = parse_rel(lx); bb_append_num(bb, v); }
            if (is_string) *is_string = 1;
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return;
        }
        if (_stricmp(nbuf, "SEG$") == 0) {
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                const char* s = ""; int start = 1, len = 0;
                if (lx->cur.type == T_STRING) { s = lx->cur.text; lx_next(lx); }
                else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
                    Variable* v = find_var(lx->cur.text); s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(lx);
                }
                if (lx->cur.type == T_COMMA) { lx_next(lx); start = (int)parse_rel(lx); }
                if (lx->cur.type == T_COMMA) { lx_next(lx); len = (int)parse_rel(lx); }
                int sl = (int)strlen(s), i0 = start < 1 ? 0 : start - 1; if (i0 > sl) i0 = sl;
                int l = len ? len : (sl - i0); if (i0 + l > sl) l = sl - i0; if (l < 0) l = 0;
                bb_append(bb, s + i0, (size_t)l);
            }
            if (is_string) *is_string = 1;
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return;
        }
        if (_stricmp(nbuf, "TRM$") == 0) {
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                const char* s = "";
                if (lx->cur.type == T_STRING) { s = lx->cur.text; lx_next(lx); }
                else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
                    Variable* v = find_var(lx->cur.text); s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(lx);
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
            while (lx->cur.type != T_RPAREN && lx->cur.type != T_END) {
                if (nsubs >= MAX_DIMS) break;
                subs[nsubs++] = (int)parse_rel(lx);
                if (lx->cur.type == T_COMMA) { lx_next(lx); continue; }
                else break;
            }
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            { const char* s = sa ? sarray_get(sa, subs, nsubs) : ""; bb_append(bb, s, strlen(s)); }
        }
        else {
            Variable* v = find_var(nbuf);
            { const char* s = (v && v->type == VT_STR && v->str) ? v->str : ""; bb_append(bb, s, strlen(s)); }
        }
        if (is_string) *is_string = 1;
        return;
    }

    /* numeric default */
    { double v = parse_rel(lx); bb_append_num(bb, v); }
}

/* Public: execute PRINT statement */
int exec_print(Lexer* lx) {
    FILE* out = stdout;

    /* current token must be PRINT */
    if (lx->cur.type != T_PRINT) return -1;
    lx_next(lx);

    /* Optional: PRINT #n, ... */
    if (lx->cur.type == T_HASH) {
        lx_next(lx);
        if (lx->cur.type != T_NUMBER) { printf("ERROR: PRINT # needs handle\n"); return -1; }
        out = pf_file_from_handle((int)lx->cur.number);
        if (!out) { printf("ERROR: bad handle\n"); return -1; }
        lx_next(lx);
        if (lx->cur.type == T_COMMA || lx->cur.type == T_SEMI) lx_next(lx);
    }

    int suppress_nl = 0;

    while (lx->cur.type != T_END) {
        /* Special-case TAB(n) (spacing) */
        if (lx->cur.type == T_IDENT) {
            char id[16]; strncpy(id, lx->cur.text, sizeof(id) - 1); id[sizeof(id) - 1] = 0;
            if (_stricmp(id, "TAB") == 0) {
#ifdef OLD
                lx_next(lx);
                if (lx->cur.type == T_LPAREN) lx_next(lx);
                { int n = (int)parse_rel(lx); if (n < 0) n = 0; while (n--) fputc(' ', out); }
                if (lx->cur.type == T_RPAREN) lx_next(lx);
                if (lx->cur.type == T_COMMA || lx->cur.type == T_SEMI) { suppress_nl = 1; lx_next(lx); }
                if (lx->cur.type == T_END) break;
                continue;
#else
                /* TAB to absolute column n (1-based); clamp at current if n <= current */
                int n = (int)parse_rel(lx);
                if (n < 1) n = 1;
                int target = n - 1;               /* convert to 0-based */
                while (g_print_col < target) { fputc(' ', out); g_print_col++; }
#endif //OLD
            }
        }

        /* Build one item (with '+' concat support) in a binary-safe buffer */
        unsigned char lstore[4096]; ByteBuf L; bb_init(&L, lstore, sizeof(lstore));
        int is_str = 0;
        parse_print_term_to_bb(lx, &L, &is_str);

        while (lx->cur.type == T_PLUS) {
            lx_next(lx);
            unsigned char rstore[4096]; ByteBuf R; bb_init(&R, rstore, sizeof(rstore));
            int rstr = 0;
            parse_print_term_to_bb(lx, &R, &rstr);

            if (is_str || rstr) {
                bb_append(&L, R.data, R.len); /* concatenate bytes */
                is_str = 1;
            }
            else {
                /* numeric addition: convert both buffers to temporary C-strings (safe because numeric text has no NUL) */
                char ls[128], rs[128];
                size_t ln = (L.len < sizeof(ls) - 1) ? L.len : (sizeof(ls) - 1);
                size_t rn = (R.len < sizeof(rs) - 1) ? R.len : (sizeof(rs) - 1);
                memcpy(ls, L.data, ln); ls[ln] = 0;
                memcpy(rs, R.data, rn); rs[rn] = 0;
                double sum = atof(ls) + atof(rs);
                L.len = 0; bb_append_num(&L, sum);
            }
        }

        /* emit the item (binary-safe) */
        if (L.len) {
            /* write and update column */
            for (size_t i = 0; i < L.len; i++) {
                unsigned char c = L.data[i];
                fputc((char)c, out);
                if (c == '\n') { g_print_col = 0; }
                else if (c == '\r') { g_print_col = 0; }
                else { g_print_col++; }
                suppress_nl = 0;
            }
        }

        /* optional separator suppresses trailing newline */
        if (lx->cur.type == T_COMMA) {
            suppress_nl = 1;
            lx_next(lx);
            /* advance to next zone boundary */
            int nextZone = ((g_print_col / PRINT_ZONE) + 1) * PRINT_ZONE;
            while (g_print_col < nextZone) { fputc(' ', out); g_print_col++; }
            if (lx->cur.type == T_END) break;
            continue;
        }

        if (lx->cur.type == T_SEMI) {
            suppress_nl = 1;
            lx_next(lx);
            if (lx->cur.type == T_END) break;
            continue;
        }


        if (lx->cur.type == T_END) break;
    }

    if (!suppress_nl) { fputc('\n', out); g_print_col = 0; }

    return 0;
}
