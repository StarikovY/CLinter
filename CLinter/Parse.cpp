/* Parse.cpp — full classic-C implementation of lexer + expression parser
   Features:
   - Tokens/keywords for the interpreter (LET, PRINT, INPUT, IF/THEN, GOTO, END, STOP, REM, RUN, NEW, LIST,
     SAVE/LOAD, SAVEVARS/LOADVARS, FOR/TO/STEP/NEXT, GOSUB/RETURN, OPEN/CLOSE/AS/APPEND/OUTPUT, HASH/LINE,
     DIM, DATA, READ, RESTORE, RENUM, QUIT, BYE)
   - Operators: + - * / ^ (right-assoc), relational (=, <>, <, >, <=, >=)
   - Logical: AND, OR, XOR, NOT (bitwise semantics on integer-coerced operands)
   - Numbers, string literals, identifiers, scalar variables, numeric & string arrays
   - Math functions: ATN, COS, SIN, TAN, EXP, LOG (ln), LOG10, SQR, ABS, POW(base,exp)
   - Misc functions/constants: RND(), INT(), SGN(), PI, LEN(), ASC(), VAL(), CHR$(), STR$()
   - RT-11-style helpers usable in expressions: POS(hay$,needle$), TAB(n) (returns n), SEG$(s$,start,len), TRM$(s$)
   Notes:
   - String-returning funcs in numeric context coerce via atof("") per existing interpreter behavior.

   Created by Yuri Starikov with ChatGPT 5.0
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "runtime.h"
#include "parse.h"

/* --- Lexer --- */
static void lx_skip_space(Lexer*lx){ while(lx->s[lx->i] && isspace((unsigned char)lx->s[lx->i])) lx->i++; }

void lx_init(Lexer*lx,const char*s)
{ 
    lx->s=s ? s : "";
    lx->i=0; 
    lx->cur.type=T_END; 
    lx->cur.text[0]=0; 
    lx->cur.number=0.0; 
}

void lx_next(Lexer* lx) {
    lx_skip_space(lx);
    if (!lx->s[lx->i]) { lx->cur.type = T_END; lx->cur.text[0] = 0; return; }

    /* two-char relational operators */
    if (lx->s[lx->i] && lx->s[lx->i + 1]) {
        if (lx->s[lx->i] == '<' && lx->s[lx->i + 1] == '>') { lx->i += 2; lx->cur.type = T_NE; return; }
        if (lx->s[lx->i] == '<' && lx->s[lx->i + 1] == '=') { lx->i += 2; lx->cur.type = T_LE; return; }
        if (lx->s[lx->i] == '>' && lx->s[lx->i + 1] == '=') { lx->i += 2; lx->cur.type = T_GE; return; }
    }

    /* number */
    if (isdigit((unsigned char)lx->s[lx->i]) || (lx->s[lx->i] == '.' && isdigit((unsigned char)lx->s[lx->i + 1]))) {
        size_t start = lx->i; while (isdigit((unsigned char)lx->s[lx->i]) || lx->s[lx->i] == '.') lx->i++;
        { char buf[64]; size_t len = lx->i - start; if (len > 63) len = 63; memcpy(buf, lx->s + start, len); buf[len] = 0; lx->cur.type = T_NUMBER; lx->cur.number = (double)atof(buf); return; }
    }

    /* string literal: "..." */
    if (lx->s[lx->i] == '"') {
        size_t start; lx->i++; start = lx->i;
        while (lx->s[lx->i] && lx->s[lx->i] != '"') lx->i++;
        { size_t len = lx->i - start; if (len > 127) len = 127; memcpy(lx->cur.text, lx->s + start, len); lx->cur.text[len] = 0; lx->cur.type = T_STRING; }
        if (lx->s[lx->i] == '"') lx->i++;
        return;
    }

    /* identifier / keyword (letters, _, may include $ at end) */
    if (isalpha((unsigned char)lx->s[lx->i]) || lx->s[lx->i] == '_') {
        size_t start = lx->i++;
        while (isalnum((unsigned char)lx->s[lx->i]) || lx->s[lx->i] == '_' || lx->s[lx->i] == '$') lx->i++;
        {
            char t[128], u[128]; size_t len = lx->i - start; if (len > 127) len = 127; memcpy(t, lx->s + start, len); t[len] = 0;
            { size_t k; for (k = 0; k < len; k++) { u[k] = (char)toupper((unsigned char)t[k]); } u[len] = 0; }

            /* Statement/command keywords */
            if (!strcmp(u, "LET")) { lx->cur.type = T_LET; return; }
            if (!strcmp(u, "PRINT") || !strcmp(u, "?")) { lx->cur.type = T_PRINT; return; }
            if (!strcmp(u, "INPUT")) { lx->cur.type = T_INPUT; return; }
            if (!strcmp(u, "LINE")) { lx->cur.type = T_LINE; return; }
            if (!strcmp(u, "IF")) { lx->cur.type = T_IF; return; }
            if (!strcmp(u, "THEN")) { lx->cur.type = T_THEN; return; }
            if (!strcmp(u, "GOTO")) { lx->cur.type = T_GOTO; return; }
            if (!strcmp(u, "END")) { lx->cur.type = T_ENDKW; return; }
            if (!strcmp(u, "STOP")) { lx->cur.type = T_STOP; return; }
            if (!strcmp(u, "REM")) { lx->cur.type = T_REM; return; }
            if (!strcmp(u, "RUN")) { lx->cur.type = T_RUN; return; }
            if (!strcmp(u, "NEW")) { lx->cur.type = T_NEW; return; }
            if (!strcmp(u, "LIST")) { lx->cur.type = T_LIST; return; }
            if (!strcmp(u, "SAVE")) { lx->cur.type = T_SAVE; return; }
            if (!strcmp(u, "LOAD")) { lx->cur.type = T_LOAD; return; }
            if (!strcmp(u, "SAVEVARS")) { lx->cur.type = T_SAVEVARS; return; }
            if (!strcmp(u, "LOADVARS")) { lx->cur.type = T_LOADVARS; return; }
            if (!strcmp(u, "FOR")) { lx->cur.type = T_FOR; return; }
            if (!strcmp(u, "TO")) { lx->cur.type = T_TO; return; }
            if (!strcmp(u, "STEP")) { lx->cur.type = T_STEP; return; }
            if (!strcmp(u, "NEXT")) { lx->cur.type = T_NEXT; return; }
            if (!strcmp(u, "GOSUB")) { lx->cur.type = T_GOSUB; return; }
            if (!strcmp(u, "RETURN")) { lx->cur.type = T_RETURN; return; }
            if (!strcmp(u, "OPEN")) { lx->cur.type = T_OPEN; return; }
            if (!strcmp(u, "CLOSE")) { lx->cur.type = T_CLOSE; return; }
            if (!strcmp(u, "AS")) { lx->cur.type = T_AS; return; }
            if (!strcmp(u, "APPEND")) { lx->cur.type = T_APPEND; return; }
            if (!strcmp(u, "OUTPUT")) { lx->cur.type = T_OUTPUTKW; return; }
            if (!strcmp(u, "DIM")) { lx->cur.type = T_DIM; return; }
            if (!strcmp(u, "DATA")) { lx->cur.type = T_DATA; return; }
            if (!strcmp(u, "READ")) { lx->cur.type = T_READ; return; }
            if (!strcmp(u, "RESTORE")) { lx->cur.type = T_RESTORE; return; }
            if (!strcmp(u, "TRACE")) { lx->cur.type = T_TRACE; return; }
            if (!strcmp(u, "ON")) { lx->cur.type = T_ON; return; }
            if (!strcmp(u, "OFF")) { lx->cur.type = T_OFF; return; }
            if (!strcmp(u, "DUMP")) { lx->cur.type = T_DUMP; return; }
            if (!strcmp(u, "VARS")) { lx->cur.type = T_VARS; return; }
            if (!strcmp(u, "ARRAYS")) { lx->cur.type = T_ARRAYS; return; }
            if (!strcmp(u, "STACK")) { lx->cur.type = T_STACK; return; }
            if (!strcmp(u, "QUIT")) { lx->cur.type = T_QUIT; return; }
            if (!strcmp(u, "RENUM")) { lx->cur.type = T_RENUM; return; }
            if (!strcmp(u, "BYE")) { lx->cur.type = T_BYE; return; }

            /* logical ops */
            if (!strcmp(u, "AND")) { lx->cur.type = T_AND; return; }
            if (!strcmp(u, "OR")) { lx->cur.type = T_OR;  return; }
            if (!strcmp(u, "XOR")) { lx->cur.type = T_XOR; return; }
            if (!strcmp(u, "NOT")) { lx->cur.type = T_NOT; return; }

            /* default ident */
            lx->cur.type = T_IDENT; strncpy(lx->cur.text, t, sizeof(lx->cur.text) - 1); lx->cur.text[sizeof(lx->cur.text) - 1] = 0; return;
        }
    }

    /* single-char tokens */
    {
        char c = lx->s[lx->i++]; /* single-char tokens */
        switch (c) {
        case '#': lx->cur.type = T_HASH; return;
        case '=': lx->cur.type = T_EQ; return;
        case '<': lx->cur.type = T_LT; return;
        case '>': lx->cur.type = T_GT; return;
        case '+': lx->cur.type = T_PLUS; return;
        case '-': lx->cur.type = T_MINUS; return;
        case '*': lx->cur.type = T_STAR; return;
        case '/': lx->cur.type = T_SLASH; return;
        case '(': lx->cur.type = T_LPAREN; return;
        case ')': lx->cur.type = T_RPAREN; return;
        case ',': lx->cur.type = T_COMMA; return;
        case ';': lx->cur.type = T_SEMI; return;
        case '^': lx->cur.type = T_POWOP; return;
        }
    }
    lx_next(lx); /* skip unknown */
}

/* ---------- Expression Parser ---------- */
/* Precedence (high->low):
   factor (numbers, vars, arrays, funcs, (expr), unary -, unary NOT)
   ^        (right-assoc)
   * /
   + -
   relations (=, <>, <, >, <=, >=)  -> boolean 0/1
   AND / OR / XOR (bitwise on ints)
*/

static double parse_factor(Lexer*lx);
static double parse_power(Lexer* lx); 
static double parse_term(Lexer*lx);
static double parse_expr(Lexer*lx);
static double parse_relation(Lexer* lx);
static double parse_logic(Lexer* lx);

/* helpers to uppercase a local copy of identifier */

static void upcopy(char* dst, const char* src, size_t cap) {
    size_t i; if (cap == 0) return;
    for (i = 0; src && src[i] && i + 1 < cap; ++i) dst[i] = (char)toupper((unsigned char)src[i]);
    dst[i] = 0;
}

static double parse_factor(Lexer* lx) {
    Token t = lx->cur;

    /* unary */
    if (t.type == T_MINUS) { lx_next(lx); return -parse_factor(lx); }
    if (t.type == T_PLUS) { lx_next(lx); return  parse_factor(lx); }
    if (t.type == T_NOT) { lx_next(lx); { long v = (long)parse_factor(lx); return (double)(~v); } }

    if (t.type == T_NUMBER) { double v = t.number; lx_next(lx); return v; }

    if (t.type == T_STRING) {
        /* numeric context: coerce string literal with atof() */
        double v = atof(t.text);
        lx_next(lx);
        return v;
    }

    if (t.type == T_LPAREN) {
        double v; lx_next(lx); v = parse_logic(lx); /* full precedence inside parens */
        if (lx->cur.type == T_RPAREN) lx_next(lx);
        return v;
    }


    if (t.type == T_IDENT)
    {
        /* handle functions / constants first */
        char fname[32]; upcopy(fname, t.text, sizeof(fname));

        /* --- numeric-returning classic math --- */

        /* List of supported functions: one argument unless POW (two) */
        if (!strcmp(fname, "RND")) {
            static int seeded = 0;
            lx_next(lx); if (lx->cur.type == T_LPAREN) { lx_next(lx); /* optional arg ignored */ (void)parse_rel(lx); if (lx->cur.type == T_RPAREN) lx_next(lx); }
            if (!seeded) { srand(1); seeded = 1; } /* deterministic unless you later add RANDOMIZE */
            return (double)rand() / (double)RAND_MAX;
        }

        if (!strcmp(fname, "INT")) {
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            { double v = parse_rel(lx); if (lx->cur.type == T_RPAREN) lx_next(lx); return floor(v); }
        }
        if (!strcmp(fname, "SGN")) {
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            { double v = parse_rel(lx); if (lx->cur.type == T_RPAREN) lx_next(lx); return (v > 0) - (v < 0); }
        }
        if (!strcmp(fname, "LOG10")) {
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            { double v = parse_rel(lx); if (lx->cur.type == T_RPAREN) lx_next(lx); return log10(v); }
        }

        if (!strcmp(fname, "LEN")) {
            /* LEN(string) -> number */
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                double n = 0.0;
                if (lx->cur.type == T_STRING) { n = (double)strlen(lx->cur.text); lx_next(lx); }
                else if (lx->cur.type == T_IDENT) {
                    /* string var or string array element or STR$/CHR$ call */
                    char nbuf[32]; strncpy(nbuf, lx->cur.text, sizeof(nbuf) - 1); nbuf[sizeof(nbuf) - 1] = 0;
                    if (is_string_var_name(nbuf)) {
                        lx_next(lx);
                        if (lx->cur.type == T_LPAREN) {
                            /* string array element */
                            int subs[MAX_DIMS], nsubs = 0; SArray* sa = sarray_find(nbuf);
                            lx_next(lx);
                            while (lx->cur.type != T_RPAREN && lx->cur.type != T_END) {
                                if (nsubs >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); break; }
                                subs[nsubs++] = (int)parse_rel(lx);
                                if (lx->cur.type == T_COMMA) { lx_next(lx); continue; }
                                else break;
                            }
                            if (lx->cur.type == T_RPAREN) lx_next(lx);
                            { const char* s = sa ? sarray_get(sa, subs, nsubs) : ""; n = (double)strlen(s); }
                        }
                        else {
                            Variable* v = find_var(nbuf); const char* s = (v && v->type == VT_STR && v->str) ? v->str : ""; n = (double)strlen(s);
                        }
                    }
                    else {
                        /* maybe STR$() or CHR$()? Treat as 0 length if unknown here */
                        n = 0.0;
                    }
                }
                if (lx->cur.type == T_RPAREN) lx_next(lx);
                return n;
            }
        }
        if (!strcmp(fname, "ASC")) {
            /* ASC(string) -> numeric code of first char (0 if empty) */
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                int c = 0;
                if (lx->cur.type == T_STRING) { c = (unsigned char)lx->cur.text[0]; lx_next(lx); }
                else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
                    char nbuf[32]; strncpy(nbuf, lx->cur.text, sizeof(nbuf) - 1); nbuf[sizeof(nbuf) - 1] = 0; lx_next(lx);
                    if (lx->cur.type == T_LPAREN) {
                        int subs[MAX_DIMS], nsubs = 0; SArray* sa = sarray_find(nbuf); lx_next(lx);
                        while (lx->cur.type != T_RPAREN && lx->cur.type != T_END) {
                            if (nsubs >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); break; }
                            subs[nsubs++] = (int)parse_rel(lx);
                            if (lx->cur.type == T_COMMA) { lx_next(lx); continue; }
                            else break;
                        }
                        if (lx->cur.type == T_RPAREN) lx_next(lx);
                        { const char* s = sa ? sarray_get(sa, subs, nsubs) : ""; c = (unsigned char)(s[0] ? s[0] : 0); }
                    }
                    else {
                        Variable* v = find_var(nbuf); const char* s = (v && v->type == VT_STR && v->str) ? v->str : ""; c = (unsigned char)(s[0] ? s[0] : 0);
                    }
                }
                if (lx->cur.type == T_RPAREN) lx_next(lx);
                return (double)c;
            }
        }
        if (!strcmp(fname, "VAL")) {
            /* VAL(string) -> number */
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                double out = 0.0;
                if (lx->cur.type == T_STRING) { out = atof(lx->cur.text); lx_next(lx); }
                else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
                    Variable* v = find_var(lx->cur.text); const char* s = (v && v->type == VT_STR && v->str) ? v->str : ""; out = atof(s); lx_next(lx);
                }
                if (lx->cur.type == T_RPAREN) lx_next(lx);
                return out;
            }
        }

        /* NOTE: CHR$ and STR$ return strings; in numeric context they coerce via atof(""). */
        if (!strcmp(fname, "CHR$")) {
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            { int code = (int)parse_rel(lx); if (lx->cur.type == T_RPAREN) lx_next(lx); char tmp[4]; tmp[0] = (char)code; tmp[1] = 0; return atof(tmp); }
        }
        if (!strcmp(fname, "STR$")) {
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            { double v = parse_rel(lx); if (lx->cur.type == T_RPAREN) lx_next(lx); char buf[64]; _snprintf(buf, sizeof(buf), "%.15g", v); return atof(buf); }
        }

        /* PI constant (no args; optional parentheses tolerated) */
        if (!strcmp(fname, "PI")) {
            lx_next(lx);
            if (lx->cur.type == T_LPAREN) { lx_next(lx); if (lx->cur.type == T_RPAREN) lx_next(lx); }
            return 3.14159265358979323846;
        }

        /* POS(hay$, needle$) -> 1-based index (0 if not found) */
        if (!strcmp(fname, "POS")) {
            char* p1 = NULL, * p2=NULL;
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                const char *hay=NULL, *nee = NULL;
                if (lx->cur.type == T_STRING) { 
                    hay = lx->cur.text; 
                    p1 = (char *) malloc(strlen(hay)+1); 
                    if (p1 != NULL)
                        strcpy(p1, hay);
                    else 
                        return 0.0;
                    lx_next(lx);
                }
                else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
                    Variable* v = find_var(lx->cur.text); hay = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(lx);
                }
                if (lx->cur.type == T_COMMA) lx_next(lx);

                if (lx->cur.type == T_STRING) 
                { 
                    nee = lx->cur.text; 
                    p2 = (char*)malloc(strlen(nee) + 1);
                    if (p2 != NULL)
                        strcpy(p2, nee);
                    else
                        return 0.0;
                    lx_next(lx);  
                }

                else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
                    Variable* v = find_var(lx->cur.text); nee = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(lx);
                }
                if (lx->cur.type == T_RPAREN) lx_next(lx);
                if (!p1 || !p2) return 0.0;
                { 
                    const char* p = strstr(p1, p2); 
                    int reti = (int)(p - p1);
                    double ret = (double)(reti);
                    ret++;
                    free(p1);
                    free(p2);
                    return p ? ret : 0.0; 
                }
            }
        }

        /* TAB(n) — in numeric context just returns n (PRINT handles spacing) */
        if (!strcmp(fname, "TAB")) {
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            { double v = parse_rel(lx); if (lx->cur.type == T_RPAREN) lx_next(lx); return v; }
        }

        /* SEG$(s$, start, len) — numeric context coerces with atof result */
        if (!strcmp(fname, "SEG$")) {
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                const char* s = ""; int start = 1, len = 0;
                if (lx->cur.type == T_STRING) { s = lx->cur.text; lx_next(lx); }
                else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
                    Variable* v = find_var(lx->cur.text); s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(lx);
                }
                if (lx->cur.type == T_COMMA) { lx_next(lx); start = (int)parse_rel(lx); }
                if (lx->cur.type == T_COMMA) { lx_next(lx); len = (int)parse_rel(lx); }
                if (lx->cur.type == T_RPAREN) lx_next(lx);
                if (start < 1) start = 1; if (len < 0) len = 0;
                {
                    int sl = (int)strlen(s); int i0 = start - 1; if (i0 > sl) i0 = sl; if (i0 < 0) i0 = 0;
                    int l = len; if (i0 + l > sl) l = sl - i0; if (l < 0) l = 0;
                    char tmp[1024]; if (l > (int)sizeof(tmp) - 1) l = (int)sizeof(tmp) - 1;
                    memcpy(tmp, s + i0, l); tmp[l] = 0; return atof(tmp);
                }
            }
        }

        /* TRM$(s$) — trims leading and trailing spaces; numeric context coerces */
        if (!strcmp(fname, "TRM$")) {
            lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
            {
                const char* s = ""; if (lx->cur.type == T_STRING) { s = lx->cur.text; lx_next(lx); }
                else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
                    Variable* v = find_var(lx->cur.text); s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(lx);
                }
                if (lx->cur.type == T_RPAREN) lx_next(lx);
                {
                    char buf[1024]; size_t n = strlen(s), a = 0, b = n;
                    while (a < b && isspace((unsigned char)s[a])) a++;
                    while (b > a && isspace((unsigned char)s[b - 1])) b--;
                    { size_t l = b - a; if (l > sizeof(buf) - 1) l = sizeof(buf) - 1; memcpy(buf, s + a, l); buf[l] = 0; return atof(buf); }
                }
            }
        }

        if (strcmp(fname, "ATN") == 0) {
            lx_next(lx);
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            double v = parse_rel(lx);
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return atan(v);
        }
        if (strcmp(fname, "COS") == 0) {
            lx_next(lx);
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            double v = parse_rel(lx);
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return cos(v);
        }
        if (strcmp(fname, "SIN") == 0) {
            lx_next(lx);
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            double v = parse_rel(lx);
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return sin(v);
        }
        if (strcmp(fname, "TAN") == 0) {
            lx_next(lx);
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            double v = parse_rel(lx);
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return tan(v);
        }
        if (strcmp(fname, "EXP") == 0) {
            lx_next(lx);
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            double v = parse_rel(lx);
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return exp(v);
        }
        if (strcmp(fname, "LOG") == 0) { /* natural log */
            lx_next(lx);
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            double v = parse_rel(lx);
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return log(v);
        }
        if (strcmp(fname, "POW") == 0) { /* two args */
            lx_next(lx);
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            double b = parse_rel(lx);
            if (lx->cur.type == T_COMMA) { lx_next(lx); }
            double e = parse_rel(lx);
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return pow(b, e);
        }
        if (strcmp(fname, "SQR") == 0) { /* sqrt */
            lx_next(lx);
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            double v = parse_rel(lx);
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return sqrt(v);
        }
        if (strcmp(fname, "ABS") == 0) {
            lx_next(lx);
            if (lx->cur.type == T_LPAREN) lx_next(lx);
            double v = parse_rel(lx);
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            return fabs(v);
        }

        /* If not a recognized function: variable / array lookup 
           look ahead: array element? */

        lx_next(lx);
        if (lx->cur.type == T_LPAREN) {
            int subs[MAX_DIMS], nsubs = 0;
            int isStrName = is_string_var_name(t.text);
            lx_next(lx);
            while (lx->cur.type != T_RPAREN && lx->cur.type != T_END) {
                if (nsubs >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); break; }
                subs[nsubs++] = (int)parse_rel(lx);
                if (lx->cur.type == T_COMMA) { lx_next(lx); continue; }
                else break;
            }
            if (lx->cur.type == T_RPAREN) lx_next(lx);
            if (isStrName) {
                SArray* sa = sarray_find(t.text);
                const char* sval = sa ? sarray_get(sa, subs, nsubs) : "";
                return (double)atof(sval);
            }
            else {
                Array* a = array_find(t.text);
                if (!a) { printf("ERROR: UNDIM'D ARRAY %s\n", t.text); return 0.0; }
                return array_get(a, subs, nsubs);
            }
        }
        /* scalar variable fallback */
        {
            Variable* v = find_var(t.text);
            if (!v) return 0.0;
            return v->type == VT_NUM ? v->num : (double)atof(v->str ? v->str : "0");
        }
    }
    return 0.0;
}

/* Right-associative exponentiation */
static double parse_power(Lexer* lx) {
    double left = parse_factor(lx);
    if (lx->cur.type == T_POWOP) {
        lx_next(lx);
        /* recurse to stay right-associative: a^b^c = a^(b^c) */
        double right = parse_power(lx);
        left = pow(left, right);
    }
    return left;
}

static double parse_term(Lexer* lx)
{
    double v = parse_power(lx);           /* was parse_factor */
    while (lx->cur.type == T_STAR || lx->cur.type == T_SLASH) {
        TokType op = lx->cur.type; lx_next(lx);
        double rhs = parse_power(lx);     /* was parse_factor */
        if (op == T_STAR) v *= rhs; else v /= rhs;
    }
    return v;
}

static double parse_expr(Lexer*lx)
{
    double v=parse_term(lx);
    while(lx->cur.type==T_PLUS||lx->cur.type==T_MINUS){
        TokType op=lx->cur.type; lx_next(lx);
        { double rhs=parse_term(lx); if(op==T_PLUS) v+=rhs; else v-=rhs; }
    }
    return v;
}

/* comparisons -> boolean 0/1 */
static double parse_relation(Lexer* lx) {
    double lhs = parse_expr(lx);
    if (lx->cur.type == T_EQ || lx->cur.type == T_NE || lx->cur.type == T_LT || lx->cur.type == T_GT || lx->cur.type == T_LE || lx->cur.type == T_GE) {
        TokType op = lx->cur.type; lx_next(lx);
        {
            double rhs = parse_expr(lx); int r = 0;
            if (op == T_EQ) r = (lhs == rhs);
            else if (op == T_NE) r = (lhs != rhs);
            else if (op == T_LT) r = (lhs < rhs);
            else if (op == T_GT) r = (lhs > rhs);
            else if (op == T_LE) r = (lhs <= rhs);
            else if (op == T_GE) r = (lhs >= rhs);
            return r ? 1.0 : 0.0;
        }
    }
    return lhs;
}

/* logical chain (bitwise on ints) */
static double parse_logic(Lexer* lx) {
    double left = parse_relation(lx);
    while (lx->cur.type == T_AND || lx->cur.type == T_OR || lx->cur.type == T_XOR) {
        TokType op = lx->cur.type; lx_next(lx);
        double right = parse_relation(lx);
        int L = (left != 0.0);
        int R = (right != 0.0);
        if (op == T_AND) left = (L && R) ? 1.0 : 0.0;
        else if (op == T_OR) left = (L || R) ? 1.0 : 0.0;
        else              left = ((L && !R) || (!L && R)) ? 1.0 : 0.0; /* XOR */
    }
    return left;
}

/* Public entry used by executor */
double parse_rel(Lexer* lx) {
    return parse_logic(lx);
}

