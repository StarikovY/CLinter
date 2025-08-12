
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "runtime.h"
#include "parse.h"

/* --- Lexer --- */
static void lx_skip_space(Lexer*lx){ while(lx->s[lx->i] && isspace((unsigned char)lx->s[lx->i])) lx->i++; }

void lx_init(Lexer*lx,const char*s)
{ 
    lx->s=s; 
    lx->i=0; 
    lx->cur.type=T_END; 
    lx->cur.text[0]=0; 
    lx->cur.number=0.0; 
}

void lx_next(Lexer* lx) {
    lx_skip_space(lx);
    if (!lx->s[lx->i]) { lx->cur.type = T_END; lx->cur.text[0] = 0; return; }

    if (lx->s[lx->i] && lx->s[lx->i + 1]) {
        if (lx->s[lx->i] == '<' && lx->s[lx->i + 1] == '>') { lx->i += 2; lx->cur.type = T_NE; return; }
        if (lx->s[lx->i] == '<' && lx->s[lx->i + 1] == '=') { lx->i += 2; lx->cur.type = T_LE; return; }
        if (lx->s[lx->i] == '>' && lx->s[lx->i + 1] == '=') { lx->i += 2; lx->cur.type = T_GE; return; }
    }

    if (isdigit((unsigned char)lx->s[lx->i]) || (lx->s[lx->i] == '.' && isdigit((unsigned char)lx->s[lx->i + 1]))) {
        size_t start = lx->i; while (isdigit((unsigned char)lx->s[lx->i]) || lx->s[lx->i] == '.') lx->i++;
        { char buf[64]; size_t len = lx->i - start; if (len > 63) len = 63; memcpy(buf, lx->s + start, len); buf[len] = 0; lx->cur.type = T_NUMBER; lx->cur.number = (double)atof(buf); return; }
    }

    if (lx->s[lx->i] == '"') {
        size_t start; lx->i++; start = lx->i;
        while (lx->s[lx->i] && lx->s[lx->i] != '"') lx->i++;
        { size_t len = lx->i - start; if (len > 127) len = 127; memcpy(lx->cur.text, lx->s + start, len); lx->cur.text[len] = 0; lx->cur.type = T_STRING; }
        if (lx->s[lx->i] == '"') lx->i++;
        return;
    }

    if (isalpha((unsigned char)lx->s[lx->i]) || lx->s[lx->i] == '_') {
        size_t start = lx->i++;
        while (isalnum((unsigned char)lx->s[lx->i]) || lx->s[lx->i] == '_' || lx->s[lx->i] == '$') lx->i++;
        {
            char t[128], u[128]; size_t len = lx->i - start; if (len > 127) len = 127; memcpy(t, lx->s + start, len); t[len] = 0;
            { size_t k; for (k = 0; k < len; k++) { u[k] = (char)toupper((unsigned char)t[k]); } u[len] = 0; }
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
            /* default ident */
            lx->cur.type = T_IDENT; strncpy(lx->cur.text, t, sizeof(lx->cur.text) - 1); lx->cur.text[sizeof(lx->cur.text) - 1] = 0; return;
        }
    }

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
        }
    }
    lx_next(lx); /* skip unknown */
}

/* --- Parser (expressions) --- */
static double parse_factor(Lexer*lx);
static double parse_term(Lexer*lx);
static double parse_expr(Lexer*lx);

#ifdef OLD
static double parse_factor(Lexer*lx)
{
    Token t = lx->cur;
    if(t.type==T_MINUS){ lx_next(lx); return -parse_factor(lx); }
    if(t.type==T_NUMBER){ double v=t.number; lx_next(lx); return v; }
    if(t.type==T_LPAREN){ double v; lx_next(lx); v=parse_rel(lx); if(lx->cur.type==T_RPAREN) lx_next(lx); return v; }
    
	if (t.type == T_IDENT) 
	{
		/* Look ahead for '(' => array element (numeric or string) */
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
		/* scalar fallback */
		{
			Variable* v = find_var(t.text);
			if (!v) return 0.0;
			return v->type == VT_NUM ? v->num : (double)atof(v->str ? v->str : "0");
		}
	}

	return 0.0;
}
#endif // OLD

static double parse_factor(Lexer* lx) {
    Token t = lx->cur;
    if (t.type == T_MINUS) { lx_next(lx); return -parse_factor(lx); }
    if (t.type == T_NUMBER) { double v = t.number; lx_next(lx); return v; }
    if (t.type == T_LPAREN) { double v; lx_next(lx); v = parse_rel(lx); if (lx->cur.type == T_RPAREN) lx_next(lx); return v; }

    if (t.type == T_IDENT) {
        /* possible array element? */
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

static double parse_term(Lexer*lx)
{
    double v=parse_factor(lx);
    while(lx->cur.type==T_STAR||lx->cur.type==T_SLASH){
        TokType op=lx->cur.type; lx_next(lx);
        { double rhs=parse_factor(lx); if(op==T_STAR) v*=rhs; else v/=rhs; }
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
double parse_rel(Lexer*lx)
{
    double lhs=parse_expr(lx);
    if(lx->cur.type==T_EQ||lx->cur.type==T_NE||lx->cur.type==T_LT||lx->cur.type==T_GT||lx->cur.type==T_LE||lx->cur.type==T_GE){
        TokType op=lx->cur.type; lx_next(lx);
        { double rhs=parse_expr(lx); int r=0;
          if(op==T_EQ) r=(lhs==rhs); else if(op==T_NE) r=(lhs!=rhs); else if(op==T_LT) r=(lhs<rhs);
          else if(op==T_GT) r=(lhs>rhs); else if(op==T_LE) r=(lhs<=rhs); else if(op==T_GE) r=(lhs>=rhs);
          return r?1.0:0.0; }
    }
    return lhs;
}
