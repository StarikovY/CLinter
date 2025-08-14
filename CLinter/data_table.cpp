// Handling DATA/RESTORE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "runtime.h"

/* data_table.cpp — minimal DATA table (re)build for CLinter
   Keeps existing storage:
     - static char** g_data_vals
     - static int    g_data_count
     - static int    g_data_ptr
     - static int    g_data_built
   Adds:
     - static int*   g_data_lines  (line number for each DATA item)
     - static int    g_data_cap    (shared capacity)
   Exposes:
     - void data_rebuild_from_program(void)
     - void data_restore(void)
     - void data_restore_at_line(int line)
*/

/* ---- Your existing globals (declared here to define) ---- */
static char** g_data_vals = NULL;
static int    g_data_count = 0;
static int    g_data_ptr = 0;
static int    g_data_built = 0;

/* ---- New parallel storage for RESTORE <line> ---- */
static int* g_data_lines = NULL;   /* line number for each DATA item */
static int    g_data_cap = 0;



ProgLine* g_prog_head = NULL;

/* ---- small portable strdup ---- */
static char* strdup_c(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) { fprintf(stderr, "OUT OF MEMORY\n"); exit(1); }
    memcpy(p, s, n);
    return p;
}

void data_mark_dirty(void) {
    /* table is stale; rebuild on demand */
    g_data_built = 0;
    g_data_ptr = 0;
}

void data_maybe_rebuild(void) {
    if (!g_data_built) {
        data_rebuild_from_program();   /* sets g_data_built = 1 inside */
    }
}

/* ---- grow arrays together ---- */
static void data_ensure_cap(int need) {
    if (need <= g_data_cap) return;
    int newcap = g_data_cap ? g_data_cap * 2 : 64;
    while (newcap < need) newcap *= 2;

    char** nv = (char**)realloc(g_data_vals, newcap * sizeof(char*));
    int* nl = (int*)realloc(g_data_lines, newcap * sizeof(int));
    if (!nv || !nl) {
        fprintf(stderr, "ERROR: out of memory while growing DATA table\n");
        exit(1);
    }
    g_data_vals = nv;
    g_data_lines = nl;
    g_data_cap = newcap;
}

/* ---- push one DATA item ---- */
static void data_push_value(const char* val, int line) {
    data_ensure_cap(g_data_count + 1);
    g_data_vals[g_data_count] = strdup_c(val ? val : "");
    g_data_lines[g_data_count] = line;
    g_data_count++;
}

/* ---- utils: trim spaces ---- */
static void trim_span(const char* s, int* lo, int* hi) {
    int i = *lo, j = *hi;
    while (i <= j && isspace((unsigned char)s[i])) i++;
    while (j >= i && isspace((unsigned char)s[j])) j--;
    *lo = i; *hi = j;
}

/* ---- parse a DATA list in one statement segment:
       input:  segment points at the 'D' in DATA (case-insensitive)
       behavior: extracts comma-separated literals (numbers or "strings")
       returns count of values appended
---- */
static int parse_data_segment(const char* seg, int line) {
    /* Expect leading DATA */
    int i = 0;
    while (isspace((unsigned char)seg[i])) i++;
    /* check "DATA" */
    int j = i;
    while (seg[j] && isalpha((unsigned char)seg[j])) j++;
    /* uppercase compare of seg[i..j) to "DATA" */
    int k, ok = 1; const char* KW = "DATA";
    if (j - i != 4) ok = 0; else {
        for (k = 0; k < 4; k++) {
            char c = seg[i + k];
            if ((c >= 'a' && c <= 'z')) c = (char)(c - 'a' + 'A');
            if (c != KW[k]) { ok = 0; break; }
        }
    }
    if (!ok) return 0;
    i = j; /* after DATA */

    /* now comma-separated items until end of segment */
    int added = 0;
    while (seg[i]) {
        while (isspace((unsigned char)seg[i])) i++;
        if (!seg[i]) break;

        if (seg[i] == '"') {
            /* string literal with "" escape -> quote char */
            i++; /* past opening " */
            char buf[4096]; int bl = 0;
            while (seg[i]) {
                if (seg[i] == '"') {
                    if (seg[i + 1] == '"') { /* escaped quote */
                        if (bl < (int)sizeof(buf) - 1) buf[bl++] = '"';
                        i += 2;
                        continue;
                    }
                    else {
                        i++; /* closing quote */
                        break;
                    }
                }
                if (bl < (int)sizeof(buf) - 1) buf[bl++] = seg[i];
                i++;
            }
            buf[bl] = 0;
            data_push_value(buf, line);
            added++;

            /* skip spaces, optional comma */
            while (isspace((unsigned char)seg[i])) i++;
            if (seg[i] == ',') { i++; continue; }
            else break;
        }
        else {
            /* number literal (or bare token) up to comma or end */
            int start = i;
            while (seg[i] && seg[i] != ',') i++;
            int end = i - 1;
            trim_span(seg, &start, &end);
            if (end >= start) {
                char tmp[256]; int l = end - start + 1;
                if (l > (int)sizeof(tmp) - 1) l = (int)sizeof(tmp) - 1;
                memcpy(tmp, seg + start, l); tmp[l] = 0;
                data_push_value(tmp, line);
                added++;
            }
            if (seg[i] == ',') { i++; continue; }
            else break;
        }
    }
    return added;
}

/* ---- split a line's text into statement segments by ':' or '\' ---- */
static void scan_line_for_data(const char* lineText, int lineNo) {
    if (!lineText) return;

    const char* p = lineText;
    while (*p) {
        /* find end of this segment, respecting quotes */
        const char* segStart = p;
        int inq = 0;
        while (*p) {
            char c = *p;
            if (c == '"') {
                inq = !inq;
                p++;
                continue;
            }
            if (!inq && (c == ':' || c == '\\')) break;
            p++;
        }
        /* we have segment [segStart, p) */
        /* Fast-check if it starts with DATA after leading spaces */
        const char* s = segStart;
        while (s < p && isspace((unsigned char)*s)) s++;
        /* compare first non-space token to DATA */
        const char* t = s;
        while (t < p && isalpha((unsigned char)*t)) t++;
        int tokenLen = (int)(t - s);
        int isDATA = 0;
        if (tokenLen == 4) {
            char kw[5]; for (int i = 0; i < 4; i++) { char ch = s[i]; if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A'); kw[i] = ch; } kw[4] = 0;
            if (strcmp(kw, "DATA") == 0) isDATA = 1;
        }
        if (isDATA) {
            /* parse this DATA list */
            char* seg = (char*)malloc((size_t)(p - s + 1));
            if (!seg) { fprintf(stderr, "OOM\n"); exit(1); }
            memcpy(seg, s, (size_t)(p - s)); seg[p - s] = 0;
            parse_data_segment(seg, lineNo);
            free(seg);
        }

        /* step over separator if present */
        if (*p == ':' || *p == '\\') p++;
    }
}

/* ---- public: rebuild DATA table by scanning the whole program ---- */
void data_rebuild_from_program(void) {
    /* clear old */
    for (int i = 0; i < g_data_count; i++) {
        free(g_data_vals[i]);
    }
    free(g_data_vals);  g_data_vals = NULL;
    free(g_data_lines); g_data_lines = NULL;
    g_data_count = 0;
    g_data_ptr = 0;
    g_data_cap = 0;

    /* walk program lines and collect DATA items */
    ProgLine* ln = g_prog_head;
    while (ln) {
        if (ln->text && *ln->text) {
            scan_line_for_data(ln->text, ln->number);
        }
        ln = ln->next;
    }

    g_data_built = 1;
}

/* ---- public: RESTORE (no-arg) ---- */
void data_restore(void) {
    g_data_ptr = 0;
}

/* ---- public: RESTORE <line> ---- */
void data_restore_at_line(int line) {
    if (!g_data_built) {
        /* If you lazily build, you may call data_rebuild_from_program() here */
        g_data_ptr = 0;
        return;
    }
    int i = 0;
    while (i < g_data_count && g_data_lines[i] < line) i++;
    g_data_ptr = i; /* if no match, points to end */
}

/* ---- (optional) helper: read next DATA item into buffer; returns 1 ok / 0 end ---- */
int data_read_next(char* dst, size_t cap) {
    if (g_data_ptr >= g_data_count) return 0;
    const char* src = g_data_vals[g_data_ptr++];
    if (!dst || cap == 0) return 1;
    size_t n = strlen(src);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n); dst[n] = 0;
    return 1;
}
