#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include <ctype.h>
Variable* create_string_var(const char* name);
void set_string_var(Variable* v, const char* s);
#include "parse.h"
#include "strcon.h"

int exec_assignment_tail(Lexer lx, int duringRun, int currentLine) {
	/* Handle assignment to variable or array element */
	if (lx.cur.type == T_LET) lx_next(&lx);

	if (lx.cur.type == T_IDENT) {
		int isStr; char name[32]; strncpy(name, lx.cur.text, sizeof(name) - 1); name[sizeof(name) - 1] = 0; isStr = is_string_var_name(name);
		lx_next(&lx);

		/* Array element assignment: NAME '(' subs ')' '=' expr/string */
		if (lx.cur.type == T_LPAREN) {
			int subs[MAX_DIMS], nsubs = 0;
			lx_next(&lx);
			while (lx.cur.type != T_RPAREN && lx.cur.type != T_END) {
				if (nsubs >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); return -1; }
				subs[nsubs++] = (int)parse_rel(&lx);
				if (lx.cur.type == T_COMMA) { lx_next(&lx); continue; }
				else break;
			}
			if (lx.cur.type != T_RPAREN) { printf("ERROR: missing ')'\n"); return -1; }
			lx_next(&lx);
			if (lx.cur.type != T_EQ) { printf("ERROR: '=' expected\n"); return -1; }
			lx_next(&lx);

			if (isStr) {
				/* RHS: string literal, scalar string var, or string array elem */
				if (lx.cur.type == T_STRING) {

					/* ---- STRING assignment RHS with '+' concat ---- */
					char obuf[4096]; SBuf sb; sbuf_init(&sb, obuf, sizeof(obuf));
					parse_string_expr_to_buf(&lx, &sb);

					SArray* sa = sarray_find(name);
					if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
					sarray_set(sa, subs, nsubs, obuf);
				}
				else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
					char srcname[32]; strncpy(srcname, lx.cur.text, sizeof(srcname) - 1); srcname[sizeof(srcname) - 1] = 0; lx_next(&lx);
					if (_stricmp(srcname, "CHR$") == 0) {
						if (lx.cur.type == T_LPAREN) { lx_next(&lx); }
						{
							double v = parse_rel(&lx); char ch[2]; ch[0] = (char)((int)v); ch[1] = 0;
							SArray* sa = sarray_find(name); if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
							sarray_set(sa, subs, nsubs, ch);
						}
						if (lx.cur.type == T_RPAREN) lx_next(&lx);

					}
					else if (_stricmp(srcname, "STR$") == 0)
					{
						if (lx.cur.type == T_LPAREN) { lx_next(&lx); }
						{
							double v = parse_rel(&lx); char buf[64];
#ifdef _MSC_VER
							_snprintf(buf, sizeof(buf), "%.15g", v);
#else
							snprintf(buf, sizeof(buf), "%.15g", v);
#endif
							SArray* sa = sarray_find(name); if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
							sarray_set(sa, subs, nsubs, buf);
						}
						if (lx.cur.type == T_RPAREN) lx_next(&lx);
					}
					else if (_stricmp(srcname, "SEG$") == 0) {
						if (lx.cur.type == T_LPAREN) { lx_next(&lx); }
						{
							const char* s = ""; int start = 1, len = 0;
							if (lx.cur.type == T_STRING) { s = lx.cur.text; lx_next(&lx); }
							else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
								Variable* v = find_var(lx.cur.text); s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(&lx);
							}
							if (lx.cur.type == T_COMMA) { lx_next(&lx); start = (int)parse_rel(&lx); }
							if (lx.cur.type == T_COMMA) { lx_next(&lx); len = (int)parse_rel(&lx); }
							if (lx.cur.type == T_RPAREN) lx_next(&lx);
							{
								int sl = (int)strlen(s), i0 = start < 1 ? 0 : start - 1; if (i0 > sl) i0 = sl; int l = len; if (l < 0) l = 0; if (i0 + l > sl) l = sl - i0;
								char tmp[1024]; if (l > (int)sizeof(tmp) - 1) l = (int)sizeof(tmp) - 1; memcpy(tmp, s + i0, l); tmp[l] = 0;
								SArray* sa = sarray_find(name); if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
								sarray_set(sa, subs, nsubs, tmp);
							}
						}
					}
					else if (_stricmp(srcname, "TRM$") == 0) {
						if (lx.cur.type == T_LPAREN) { lx_next(&lx); }
						{
							const char* s = ""; if (lx.cur.type == T_STRING) { s = lx.cur.text; lx_next(&lx); }
							else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
								Variable* v = find_var(lx.cur.text); s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(&lx);
							}
							if (lx.cur.type == T_RPAREN) lx_next(&lx);
							{
								size_t n = strlen(s), a = 0, b = n; while (a < b && isspace((unsigned char)s[a]))a++; while (b > a && isspace((unsigned char)s[b - 1]))b--;
								char tmp[1024]; size_t l = b - a; if (l > sizeof(tmp) - 1) l = sizeof(tmp) - 1; memcpy(tmp, s + a, l); tmp[l] = 0;
								SArray* sa = sarray_find(name); if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
								sarray_set(sa, subs, nsubs, tmp);
							}
						}
					}

					else if (lx.cur.type == T_LPAREN) {
						int s2[MAX_DIMS], n2 = 0;
						lx_next(&lx);
						while (lx.cur.type != T_RPAREN && lx.cur.type != T_END) {
							if (n2 >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); return -1; }
							s2[n2++] = (int)parse_rel(&lx);
							if (lx.cur.type == T_COMMA) { lx_next(&lx); continue; }
							else break;
						}
						if (lx.cur.type != T_RPAREN) { printf("ERROR: missing ')'\n"); return -1; }
						lx_next(&lx);
						{
							SArray* sb = sarray_find(srcname); const char* sval = sb ? sarray_get(sb, s2, n2) : "";
							SArray* sa = sarray_find(name); if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
							sarray_set(sa, subs, nsubs, sval);
						}
					}
					else {
						/* source is scalar string var */
						Variable* sv = find_var(srcname); const char* sval = (sv && sv->type == VT_STR && sv->str) ? sv->str : "";
						SArray* sa = sarray_find(name); if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
						sarray_set(sa, subs, nsubs, sval);
					}
				}

				else {
					printf("ERROR: string array assignment needs a string\n"); return -1;
				}
			}
			else {
				double vnum = parse_rel(&lx);
				{
					Array* a = array_find(name); if (!a) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
					array_set(a, subs, nsubs, vnum);
				}
			}
			return 0;
		}

		/* Scalar assignment */
		if (lx.cur.type != T_EQ) { printf("ERROR: '=' expected at line %d\n", currentLine); return -1; }
		lx_next(&lx);

		if (isStr && lx.cur.type == T_IDENT &&
            (!strcmp(lx.cur.text,"LEFT$") || !strcmp(lx.cur.text,"RIGHT$") ||
             !strcmp(lx.cur.text,"MID$") || !strcmp(lx.cur.text,"SEG$"))) {
            char result[4096]; SBuf sb;
            sbuf_init(&sb, result, sizeof(result));
            parse_string_expr_to_buf(&lx, &sb);
            set_string_var(ensure_var(name, 1), result);
            return 0;
        }
		if (isStr) {
			if (lx.cur.type == T_STRING)
			{
				char obuf[4096];
				SBuf sb;
				sbuf_init(&sb, obuf, sizeof(obuf));
				parse_string_expr_to_buf(&lx, &sb);  // <-- parses "Hello, " + "World!" + STR$(N) + ...

				Variable* v = find_var(name);
				if (!v) v = create_string_var(name);				

				if (!v) { printf("ERROR: OOM/VAR\n"); return -1; }
				set_string_var(v, obuf);
			}
			else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
				char sname[32]; strncpy(sname, lx.cur.text, sizeof(sname) - 1); sname[sizeof(sname) - 1] = 0; lx_next(&lx);
				if (_stricmp(sname, "UPPER$") == 0 || _stricmp(sname, "LOWER$") == 0) {
                    int upper = (_stricmp(sname, "UPPER$") == 0);
                    if (lx.cur.type == T_LPAREN) lx_next(&lx);
                    char tmp[1024]; SBuf sb;
                    sbuf_init(&sb, tmp, sizeof(tmp));
                    parse_string_expr_to_buf(&lx, &sb);
                    if (lx.cur.type == T_RPAREN) lx_next(&lx);
                    for (size_t k=0; k<sb.len; ++k)
                        tmp[k] = (char)(upper ? toupper((unsigned char)tmp[k]) : tolower((unsigned char)tmp[k]));
                    Variable *dst = ensure_var(name, 1);
                    set_string_var(dst, tmp);
                }
                else if (_stricmp(sname, "CHR$") == 0) {
					if (lx.cur.type == T_LPAREN) { lx_next(&lx); }
					{
						double v = parse_rel(&lx); char ch[2]; ch[0] = (char)((int)v); ch[1] = 0;
						Variable* dst = ensure_var(name, 1); if (dst->str) free(dst->str); dst->str = strdup_c(ch);
					}
					if (lx.cur.type == T_RPAREN) lx_next(&lx);
				}
				else if (_stricmp(sname, "STR$") == 0) {
					if (lx.cur.type == T_LPAREN) { lx_next(&lx); }
					{
						double v = parse_rel(&lx); char buf[64];
#ifdef _MSCVER
						_snprintf(buf, sizeof(buf), "%.15g", v);
#else
						snprintf(buf, sizeof(buf), "%.15g", v);
#endif
						Variable* dst = ensure_var(name, 1); if (dst->str) free(dst->str); dst->str = strdup_c(buf);
					}
					if (lx.cur.type == T_RPAREN) lx_next(&lx);
				}
				else if (_stricmp(sname, "SEG$") == 0) {
					if (lx.cur.type == T_LPAREN) { lx_next(&lx); }
					{
						const char* s = ""; int start = 1, len = 0;
						if (lx.cur.type == T_STRING) { s = lx.cur.text; lx_next(&lx); }
						else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
							Variable* v = find_var(lx.cur.text); s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(&lx);
						}
						if (lx.cur.type == T_COMMA) { lx_next(&lx); start = (int)parse_rel(&lx); }
						if (lx.cur.type == T_COMMA) { lx_next(&lx); len = (int)parse_rel(&lx); }
						if (lx.cur.type == T_RPAREN) lx_next(&lx);
						{
							int sl = (int)strlen(s), i0 = start < 1 ? 0 : start - 1; if (i0 > sl) i0 = sl; int l = len; if (l < 0) l = 0; if (i0 + l > sl) l = sl - i0;
							char tmp[1024]; if (l > (int)sizeof(tmp) - 1) l = (int)sizeof(tmp) - 1; memcpy(tmp, s + i0, l); tmp[l] = 0;
							Variable* dst = ensure_var(name, 1); if (dst->str) free(dst->str); dst->str = strdup_c(tmp);
						}
					}
				}
				else if (_stricmp(sname, "TRM$") == 0) {
					if (lx.cur.type == T_LPAREN) { lx_next(&lx); }
					{
						const char* s = ""; if (lx.cur.type == T_STRING) { s = lx.cur.text; lx_next(&lx); }
						else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
							Variable* v = find_var(lx.cur.text); s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(&lx);
						}
						if (lx.cur.type == T_RPAREN) lx_next(&lx);
						{
							size_t n = strlen(s), a = 0, b = n; while (a < b && isspace((unsigned char)s[a]))a++; while (b > a && isspace((unsigned char)s[b - 1]))b--;
							char tmp[1024]; size_t l = b - a; if (l > sizeof(tmp) - 1) l = sizeof(tmp) - 1; memcpy(tmp, s + a, l); tmp[l] = 0;
							Variable* dst = ensure_var(name, 1); if (dst->str) free(dst->str); dst->str = strdup_c(tmp);
						}
					}
				}
				// YST Code
				else if (_stricmp(sname, "LEFT$") == 0)
				{
					if (lx.cur.type == T_LPAREN) 
						lx_next(&lx); 
					
					char str[1024];
					str[0] = 0;
					int n = 0;

					if (lx.cur.type == T_STRING) 
					{ 
						strcpy (str,lx.cur.text); 
						lx_next(&lx); 
					}
					else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) 
					{
						Variable* v = find_var(lx.cur.text); 
						strcpy(str, (v && v->type == VT_STR && v->str) ? v->str : ""); 
						lx_next(&lx);
					}

					if (lx.cur.type == T_COMMA) 
					{ 
						lx_next(&lx); 
						n = (int)parse_rel(&lx);
					}

					if (lx.cur.type == T_RPAREN) 
						lx_next(&lx);

					int sl = (int)strlen(str); 
					if (n < 0) 
						n = 0; 
					if (n > sl) 
						n = sl;
					
					char tmp[1024]; 
					int l = n; 
					
					if (l > (int)sizeof(tmp) - 1) // 1 for '\0'
						l = (int)sizeof(tmp) - 1;
					
					memcpy(tmp, str, l); 
					tmp[l] = '\0';

					Variable* dst = ensure_var(name, 1); 
					if (dst->str) 
						free(dst->str); 
					
					dst->str = strdup_c(tmp);				
				}

				/* RIGHT$(s$, n) */
				else if (_stricmp(sname, "RIGHT$") == 0) 
				{
					lx_next(&lx); 
					if (lx.cur.type == T_LPAREN) 
						lx_next(&lx);

					char str[1024];
					str[0] = 0;
					int n = 0;

					if (lx.cur.type == T_STRING) 
					{ 
						strcpy(str, lx.cur.text);
						lx_next(&lx); 
					}

					else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) 
					{
						Variable* v = find_var(lx.cur.text); 
						strcpy(str, (v&& v->type == VT_STR && v->str) ? v->str : "");
						lx_next(&lx);
					}
					
					if (lx.cur.type == T_COMMA) 
					{ 
						lx_next(&lx); 
						n = (int)parse_rel(&lx); 
					}

					if (lx.cur.type == T_RPAREN) 
						lx_next(&lx);

					int sl = (int)strlen(str); 
					if (n < 0) 
						n = 0; 
					if (n > sl) 
						n = sl;

					int i0 = sl - n; 
					if (i0 < 0) 
						i0 = 0;
					
					char tmp[1024]; 
					int l = sl - i0; 
					
					if (l > (int)sizeof(tmp) - 1) 
						l = (int)sizeof(tmp) - 1;
					
					memcpy(tmp, str + i0, l); 
					tmp[l] = '\0';

					Variable* dst = ensure_var(name, 1);
					if (dst->str)
						free(dst->str);

					dst->str = strdup_c(tmp);
				}

				/* MID$(s$, start[, len]) -> same as SEG$ 
				if (!strcmp(fname, "MID$")) {
					lx_next(lx); if (lx->cur.type == T_LPAREN) lx_next(lx);
					const char* s = ""; int start = 1, len = 0;
					if (lx->cur.type == T_STRING) { s = lx->cur.text; lx_next(lx); }
					else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
						Variable* v = find_var(lx->cur.text); s = (v && v->type == VT_STR && v->str) ? v->str : ""; lx_next(lx);
					}
					if (lx->cur.type == T_COMMA) { lx_next(lx); start = (int)parse_logic(lx); }
					if (lx->cur.type == T_COMMA) { lx_next(lx); len = (int)parse_logic(lx); }
					if (lx->cur.type == T_RPAREN) lx_next(lx);
					if (start < 1) start = 1; int sl = (int)strlen(s); int i0 = start - 1; if (i0 > sl) i0 = sl;
					int l = (len > 0 ? len : (sl - i0)); if (i0 + l > sl) l = sl - i0; if (l < 0) l = 0;
					char tmp[1024]; if (l > (int)sizeof(tmp) - 1) l = (int)sizeof(tmp) - 1;
					memcpy(tmp, s + i0, l); tmp[l] = 0; return atof(tmp);
				}

				*/

				//End YST Code

				else if (lx.cur.type == T_LPAREN) {
					int s2[MAX_DIMS], n2 = 0;
					lx_next(&lx);
					while (lx.cur.type != T_RPAREN && lx.cur.type != T_END) {
						if (n2 >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); return -1; }
						s2[n2++] = (int)parse_rel(&lx);
						if (lx.cur.type == T_COMMA) { lx_next(&lx); continue; }
						else break;
					}
					if (lx.cur.type != T_RPAREN) { printf("ERROR: missing ')'\n"); return -1; }
					lx_next(&lx);
					{
						SArray* sb = sarray_find(sname); const char* sval = sb ? sarray_get(sb, s2, n2) : "";
						Variable* dst = ensure_var(name, 1); if (dst->str) free(dst->str); dst->str = strdup_c(sval);
					}
				}
				else {
					Variable* sv = find_var(sname); const char* sval = (sv && sv->type == VT_STR && sv->str) ? sv->str : "";
					Variable* dst = ensure_var(name, 1); if (dst->str) free(dst->str); dst->str = strdup_c(sval);
				}
			}
			else { printf("ERROR: string assignment needs a string\n"); return -1; }
		}
		else { double vnum = parse_rel(&lx); ensure_var(name, 0)->num = vnum; }
		return 0;
	}

	if (!duringRun) { printf("ERROR: syntax\n"); return -1; }
	printf("ERROR: syntax at line %d\n", currentLine); return -1;

}
