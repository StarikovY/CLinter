#include "basic_b3.h"
#include "basic_b1b2.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "runtime.h"
#include "parse.h"
#include "wxecut.h"
#include "printfunc.h"
#include "strcon.h"
#include "basic_fileio.h"
#include "screen.h"
#include "graphics.h"
#include "sprite.h"
#include "music.h"
#include "music_psg.h"
#include "msx_events.h"
#include "keyboard.h"
int exec_assignment_tail(Lexer lx, int duringRun, int currentLine);
extern int exec_multi(const char*, int, int, int*);
extern int read_filename_after(Lexer *lx, char *out, size_t cap);
static char** g_data_vals = NULL;
static int    g_data_count = 0;
static int    g_data_ptr = 0;
static int    g_data_built = 0;
/* Example structure: adjust names to your existing ones */
// extern int g_data_index;  /* current read index */
// extern int g_data_count;
/* new (minimal) */
static int* g_data_lines = NULL;   /* line number for each DATA item */
static int    g_data_cap = 0;      /* shared capacity for vals/lines */
static void data_ensure_cap(int need) {
	if (need <= g_data_cap) return;
	int newcap = g_data_cap ? g_data_cap * 2 : 64;
	while (newcap < need) newcap *= 2;
	char** nv = (char**)realloc(g_data_vals, newcap * sizeof(char*));
	int* nl = (int*)realloc(g_data_lines, newcap * sizeof(int));
	if (!nv || !nl) { /* handle OOM simply */
		fprintf(stderr, "ERROR: out of memory in DATA table\n");
		exit(1);
	}
	g_data_vals = nv;
	g_data_lines = nl;
	g_data_cap = newcap;
}
// g_trace_enabled = 0; 
static void data_clear(void) {
    int i;
    if (g_data_vals) {
        for (i = 0; i < g_data_count; i++) if (g_data_vals[i]) free(g_data_vals[i]);
        free(g_data_vals);
    }
    g_data_vals = NULL; g_data_count = 0; g_data_ptr = 0; g_data_built = 0;
    free(g_data_lines); g_data_lines = NULL; g_data_cap = 0;
}
static void data_push(const char* s) {
    char** nv = (char**)realloc(g_data_vals, sizeof(char*) * (g_data_count + 1));
    if (!nv) return;
    g_data_vals = nv;
    g_data_vals[g_data_count++] = strdup_c(s ? s : "");
}
static void data_build_from_program(void) {
    int i;
    data_clear();
    sort_program();  /* ensure order */
    for (i = 0; i < g_prog_count; i++) {
        const char* src = g_prog[i].text;
        Lexer lx; lx_init(&lx, src); lx_next(&lx);
        if (lx.cur.type != T_DATA) continue;
        int before_count = g_data_count;
        /* After DATA: comma-separated list of string literals or numeric expressions */
        lx_next(&lx);
        while (lx.cur.type != T_END) {
            if (lx.cur.type == T_STRING) {
                data_push(lx.cur.text);
                lx_next(&lx);
            }
            else {
                /* evaluate as number using parser */
                double v = parse_rel(&lx);
                char buf[64];
                /* match your numeric print precision style */
                _snprintf(buf, sizeof(buf), "%.15g", v);
                data_push(buf);
            }
            if (lx.cur.type == T_COMMA) { lx_next(&lx); continue; }
            /* tolerate stray tokens; stop at line end */
            if (lx.cur.type != T_END) { /* consume unexpected */ lx_next(&lx); }
        }
        /* Record the source line for RESTORE <line>. */
        int* lines = (int*)realloc(g_data_lines, sizeof(int) * (size_t)g_data_count);
        if (!lines && g_data_count) { fprintf(stderr, "ERROR: OUT OF MEMORY\n"); exit(1); }
        g_data_lines = lines;
        for (int j = before_count; j < g_data_count; ++j) g_data_lines[j] = g_prog[i].number;
    }
    g_data_built = 1;
}
static const char* data_next_string(void) {
    if (!g_data_built) data_build_from_program();
    if (g_data_ptr >= g_data_count) { printf("ERROR: OUT OF DATA\n"); return ""; }
    return g_data_vals[g_data_ptr++];
}
static double data_next_number(void) {
    const char* s = data_next_string();
    return atof(s);
}
static void dump_vars(void) {
    int i; for (i = 0; i < g_var_count; i++) {
        if (g_vars[i].type == VT_STR) printf("%s$ = \"%s\"\n", g_vars[i].name, g_vars[i].str ? g_vars[i].str : "");
        else printf("%s = %.15g\n", g_vars[i].name, g_vars[i].num);
    }
}
static void dump_arrays(void) {
    int i; for (i = 0; i < g_array_count; i++) {
        size_t total = 1; int d; for (d = 0; d < g_arrays[i].ndims; d++) total *= g_arrays[i].dims[d];
        printf("%s(", g_arrays[i].name);
        for (d = 0; d < g_arrays[i].ndims; d++) { printf("%d%s", g_arrays[i].dims[d], d + 1 < g_arrays[i].ndims ? "," : ""); }
        printf(") total=%zu\n", total);
    }
    for (i = 0; i < g_sarray_count; i++) {
        size_t total = 1; int d; for (d = 0; d < g_sarrays[i].ndims; d++) total *= g_sarrays[i].dims[d];
        printf("%s(", g_sarrays[i].name);
        for (d = 0; d < g_sarrays[i].ndims; d++) { printf("%d%s", g_sarrays[i].dims[d], d + 1 < g_sarrays[i].ndims ? "," : ""); }
        printf(") total=%zu (string)\n", total);
    }
}
static void dump_stack(void) {
    int k; printf("FOR stack depth=%d\n", g_for_top);
    for (k = g_for_top - 1; k >= 0; k--) printf("  FOR %s to %.15g step %.15g (after=%d)\n",
        g_for_stack[k].var, g_for_stack[k].end, g_for_stack[k].step, g_for_stack[k].afterForLine);
    printf("GOSUB stack depth=%d\n", g_gosub_top);
    for (k = g_gosub_top - 1; k >= 0; k--) printf("  return to line %d\n", g_gosub_stack[k]);
}
/* ------- RENUM support ------- */
/* find mapped line number; returns old if unmapped */
static int renum_lookup(const int* oldL, const int* newL, int n, int oldv) {
    int i; for (i = 0; i < n; i++) if (oldL[i] == oldv) return newL[i];
    return oldv;
}
/* Rebuild a statement string, replacing numbers after THEN/GOTO/GOSUB using map. */
static char* renum_rewrite_stmt(const char* src, const int* oldL, const int* newL, int nmap) {
    Lexer lx; char outbuf[MAX_LINE_LEN * 2]; size_t oi = 0;
    int expectLineNum = 0; /* 1 if last emitted token was THEN/GOTO/GOSUB */
    lx_init(&lx, src); lx_next(&lx);
    /* helper: append a token-sized string with a leading space if needed */
#define APPEND_STR(S) do{ \
        const char*_s=(S); size_t _len=strlen(_s); \
        if(oi && oi<sizeof(outbuf)-1 && outbuf[oi-1]!=' ' && outbuf[oi-1]!=',' && outbuf[oi-1]!='(') { outbuf[oi++]=' '; } \
        if(oi+_len>=sizeof(outbuf)) _len = sizeof(outbuf)-1-oi; \
        memcpy(outbuf+oi,_s,_len); oi+=_len; outbuf[oi]=0; \
    }while(0)
    outbuf[0] = 0;
    while (lx.cur.type != T_END) {
        switch (lx.cur.type) {
        case T_STRING: {
            char tmp[140];
            _snprintf(tmp, sizeof(tmp), "\"%s\"", lx.cur.text);
            APPEND_STR(tmp);
            lx_next(&lx);
        } break;
        case T_IDENT: {
            APPEND_STR(lx.cur.text);
            lx_next(&lx);
        } break;
        case T_NUMBER: {
            char tmp[64]; int v = (int)lx.cur.number;
            if (expectLineNum) { v = renum_lookup(oldL, newL, nmap, v); }
            _snprintf(tmp, sizeof(tmp), "%.15g", (double)v);
            APPEND_STR(tmp);
            expectLineNum = 0;
            lx_next(&lx);
        } break;
                     // #ifdef TEST
        case T_THEN:   APPEND_STR("THEN");  expectLineNum = 1; lx_next(&lx); break;
        case T_GOTO:   APPEND_STR("GOTO");  expectLineNum = 1; lx_next(&lx); break;
        case T_GOSUB:  APPEND_STR("GOSUB"); expectLineNum = 1; lx_next(&lx); break;
        case T_PLUS:   APPEND_STR("+");  lx_next(&lx); break;
        case T_MINUS:  APPEND_STR("-");  lx_next(&lx); break;
        case T_STAR:   APPEND_STR("*");  lx_next(&lx); break;
        case T_SLASH:  APPEND_STR("/");  lx_next(&lx); break;
        case T_EQ:     APPEND_STR("=");  lx_next(&lx); break;
        case T_LT:     APPEND_STR("<");  lx_next(&lx); break;
        case T_GT:     APPEND_STR(">");  lx_next(&lx); break;
        case T_LE:     APPEND_STR("<="); lx_next(&lx); break;
        case T_GE:     APPEND_STR(">="); lx_next(&lx); break;
        case T_NE:     APPEND_STR("<>"); lx_next(&lx); break;
        case T_LPAREN: APPEND_STR("(");  lx_next(&lx); break;
        case T_RPAREN: APPEND_STR(")");  lx_next(&lx); break;
        case T_COMMA:  APPEND_STR(",");  lx_next(&lx); break;
        case T_SEMI:   APPEND_STR(";");  lx_next(&lx); break;
        default:       lx_next(&lx); break;
        }
    }
// #endif // TEST
#undef APPEND_STR
	return strdup_c(outbuf);
}
/* ===== Variable helper implementations (minimal) =====
   These provide simple creation/assignment for numeric and string vars.
   They rely on:
	 - Variable { name[], int type; double num; char* str; }  (from runtime.h)
	 - VT_NUM / VT_STR enum values
	 - ensure_var(const char* name, int isString) -> Variable*
*/
static char* dup_cstr(const char* s) {
	if (!s) s = "";
	size_t n = strlen(s) + 1;
	char* p = (char*)malloc(n);
	if (p) memcpy(p, s, n);
	return p;
}
Variable* create_string_var(const char* name) {
	/* ensure var exists and is marked as string */
	Variable* v = ensure_var(name, 1);
	if (!v) return NULL;
	if (v->type != VT_STR) {
		if (v->str) { free(v->str); v->str = NULL; }
		v->type = VT_STR;
	}
	if (!v->str) { v->str = dup_cstr(""); }
	return v;
}
void set_string_var(Variable* v, const char* s) {
	if (!v) return;
	if (v->type != VT_STR) {
		if (v->str) { free(v->str); v->str = NULL; }
		v->type = VT_STR;
	}
	if (v->str) { free(v->str); v->str = NULL; }
	v->str = dup_cstr(s);
}
Variable* create_numeric_var(const char* name) {
	/* ensure var exists and is marked as numeric */
	Variable* v = ensure_var(name, 0);
	if (!v) return NULL;
	v->type = VT_NUM;
	v->num = 0.0;
	if (v->str) { free(v->str); v->str = NULL; }
	return v;
}
void set_numeric_var(Variable* v, double val) {
	if (!v) return;
	v->type = VT_NUM;
	v->num = b3_default_type(v->name)==1 ? (double)(long)val : val;
	if (v->str) { free(v->str); v->str = NULL; }
}
/* Execute one assignment statement:
   Accepts either:  LET <var>[subs...] = <expr>
			   or:  <var>[subs...] = <expr>
   Returns 0 on success, -1 on error.  Consumes the whole assignment. */
static int exec_assignment(Lexer* lx) {
	/* If your top-level code already handles both LET and implicit assignment,
	   just paste that code here and replace any 'return' with 'return 0' on success.
	   The function must:
		 - accept T_LET (optional) or T_IDENT as the first token
		 - support string/numeric scalars and arrays as you already do
		 - parse '=' and RHS using your existing expression functions
	*/
	/* ---------- START: paste your existing assignment logic ---------- */
	/* Example skeleton (replace with your real code): */
	if (lx->cur.type == T_LET) lx_next(lx);
	if (lx->cur.type != T_IDENT) { printf("ERROR: variable expected\n"); return -1; }
	char name[64]; strncpy(name, lx->cur.text, sizeof(name) - 1); name[sizeof(name) - 1] = 0;
	int isStr = is_string_var_name(name);
	lx_next(lx);
	/* optional array subscripts */
	int subs[MAX_DIMS], nsubs = 0;
	int isArray = 0;
	if (lx->cur.type == T_LPAREN) {
		isArray = 1; lx_next(lx);
		while (lx->cur.type != T_RPAREN && lx->cur.type != T_END) {
			if (nsubs >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); return -1; }
			subs[nsubs++] = (int)parse_rel(lx);
			if (lx->cur.type == T_COMMA) { lx_next(lx); continue; }
			else break;
		}
		if (lx->cur.type == T_RPAREN) lx_next(lx);
	}
	if (lx->cur.type != T_EQ) { printf("ERROR: '=' expected\n"); return -1; }
	lx_next(lx);
	if (isStr) {
		/* Minimal RHS: string literal or string var/array; otherwise numeric->string */
		const char* ssrc = NULL;
		char tmp[1024]; tmp[0] = 0;
		if (lx->cur.type == T_STRING) { ssrc = lx->cur.text; lx_next(lx); }
		else if (lx->cur.type == T_IDENT && is_string_var_name(lx->cur.text)) {
			/* copy from existing string var/array element */
			char rhsn[64]; strncpy(rhsn, lx->cur.text, sizeof(rhsn) - 1); rhsn[sizeof(rhsn) - 1] = 0; lx_next(lx);
			if (lx->cur.type == T_LPAREN) {
				int rsubs[MAX_DIMS], rn = 0; SArray* sa = sarray_find(rhsn);
				lx_next(lx);
				while (lx->cur.type != T_RPAREN && lx->cur.type != T_END) {
					if (rn >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); return -1; }
					rsubs[rn++] = (int)parse_rel(lx);
					if (lx->cur.type == T_COMMA) { lx_next(lx); continue; }
					else break;
				}
				if (lx->cur.type == T_RPAREN) lx_next(lx);
				ssrc = sa ? sarray_get(sa, rsubs, rn) : "";
			}
			else {
				Variable* rv = find_var(rhsn);
				ssrc = (rv && rv->type == VT_STR && rv->str) ? rv->str : "";
			}
		}
		else {
			/* fallback: numeric expr -> string */
			double v = parse_rel(lx);
#ifdef _MSC_VER
			_snprintf(tmp, sizeof(tmp), "%.15g", v);
#else
			snprintf(tmp, sizeof(tmp), "%.15g", v);
#endif
			ssrc = tmp;
		}
		if (isArray) {
			SArray* sa = sarray_find(name);
			if (!sa) { printf("ERROR: UNDIM'D STRING ARRAY %s\n", name); return -1; }
			sarray_set(sa, subs, nsubs, ssrc);
		}
		else {
			Variable* v = find_var(name);
			if (!v) { v = create_string_var(name); }           /* use your own creator */
			set_string_var(v, ssrc);                         /* or inline: free/strdup into v->str, set v->type=VT_STR */
		}
		return 0;
	}
	else {
		double val = parse_rel(lx);
		if (isArray) {
			Array* a = array_find(name);
			if (!a) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
			array_set(a, subs, nsubs, val);
		}
		else {
			Variable* v = find_var(name);
			if (!v) { v = create_numeric_var(name); }          /* use your own creator */
			set_numeric_var(v, val);                         /* or inline: v->type=VT_NUM; v->num=val; free v->str if any */
		}
		return 0;
	}
	/* ---------- END: paste your existing assignment logic ---------- */
}
// EOF
double fn_eof(int fileno) {
	if (fileno < 0 || fileno >= MAX_FILES || !g_files[fileno].used || g_files[fileno].fp == NULL)
		return -1; // invalid file number
	int c = fgetc(g_files[fileno].fp);
	if (c == EOF) return -1;
	ungetc(c, g_files[fileno].fp);
	return 0;
}

/* Helper for IF THEN/ELSE single statement execution */
static int run_if_single_stmt(Lexer* plx, int currentLine, int* outJump) {
	/* THEN <line> */
	if (plx->cur.type == T_NUMBER) {
		*outJump = (int)plx->cur.number;
		return 1;
	}
	/* THEN GOTO <line> */
	if (plx->cur.type == T_GOTO) {
		lx_next(plx);
		if (plx->cur.type != T_NUMBER) {
			printf("ERROR: line number expected\n");
			return -1;
		}
		*outJump = (int)plx->cur.number;
		return 1;
	}
	/* THEN GOSUB <line> */
	if (plx->cur.type == T_GOSUB) {
		int nextLine = next_line_number_after(currentLine);
		lx_next(plx);
		if (plx->cur.type != T_NUMBER) {
			printf("ERROR: line number expected\n");
			return -1;
		}
		if (nextLine < 0) {
			printf("ERROR: GOSUB at last line\n");
			return -1;
		}
		if (g_gosub_top >= MAX_STACK) {
			printf("ERROR: GOSUB stack overflow\n");
			return -1;
		}
		g_gosub_stack[g_gosub_top++] = nextLine;
		*outJump = (int)plx->cur.number;
		return 1;
	}
	/* THEN PRINT ... */
	if (plx->cur.type == T_PRINT) {
		return exec_print(plx); /* must stop on T_ELSE */
	}
	/* THEN LET or assignment */
	if (plx->cur.type == T_LET || plx->cur.type == T_IDENT) {
		return exec_assignment(plx);
	}

	printf("ERROR: unsupported statement after THEN/ELSE\n");
	return -1;
}

extern int matrix_statement(Lexer *lx, double (*read_number)(void));
extern int pycompat_randomize_statement(Lexer *lx);

int exec_statement(const char* src, int duringRun, int currentLine, int* outJump)
{
	Lexer lx;
	lx_init(&lx, src);
	lx_next(&lx);

    { Lexer probe=lx; int b=b3_statement(&probe,duringRun,currentLine,outJump); if(b!=-2)return b; }
    { int b=b1b2_statement(&lx,duringRun,currentLine,outJump); if(b!=-2)return b; }
    if(lx.cur.type==T_ONKW) {
        int result=msx_event_definition(&lx);
        if(result!=-2)return result;
    }
    if (lx.cur.type == T_IDENT && !_stricmp(lx.cur.text,"SPRITE")) return sprite_event_statement(&lx);
    if (lx.cur.type == T_IDENT && (!_stricmp(lx.cur.text,"PUT") || !_stricmp(lx.cur.text,"SPRITE$"))) return sprite_statement(&lx);
    if (lx.cur.type == T_IDENT && _stricmp(lx.cur.text, "PAUSE") == 0) {
        double seconds;
        lx_next(&lx);
        seconds = parse_rel(&lx);
        if (lx.cur.type != T_END) { puts("ERROR: PAUSE syntax"); return -1; }
        return clinter_pause(seconds);
    }
    if (lx.cur.type == T_IDENT && (!_stricmp(lx.cur.text,"SOUND") || !_stricmp(lx.cur.text,"BEEP"))) return music_psg_statement(&lx);
    if (lx.cur.type == T_IDENT && _stricmp(lx.cur.text,"PLAY")==0) return music_statement(&lx);
	if (lx.cur.type == T_IDENT && (_stricmp(lx.cur.text, "SCREEN") == 0 || _stricmp(lx.cur.text, "CLS") == 0 || _stricmp(lx.cur.text, "LOCATE") == 0 || _stricmp(lx.cur.text, "COLOR") == 0)) return screen_statement(&lx);
	if (lx.cur.type == T_LINE) {
        size_t gi=lx.i;
        while (lx.s[gi]==' ' || lx.s[gi]=='\t') gi++;
        if (lx.s[gi]=='(' || lx.s[gi]=='-') { lx.cur.type=T_IDENT; strcpy(lx.cur.text,"LINE"); return graphics_statement(&lx); }
    }
	if (lx.cur.type == T_IDENT && (!_stricmp(lx.cur.text,"PSET") || !_stricmp(lx.cur.text,"PRESET") || !_stricmp(lx.cur.text,"LINE") || !_stricmp(lx.cur.text,"CIRCLE") || !_stricmp(lx.cur.text,"PAINT") || !_stricmp(lx.cur.text,"DRAW") || !_stricmp(lx.cur.text,"COPY"))) return graphics_statement(&lx);
	if (lx.cur.type == T_IDENT && _stricmp(lx.cur.text, "RANDOMIZE") == 0) return pycompat_randomize_statement(&lx);
	if (lx.cur.type == T_IDENT && _stricmp(lx.cur.text, "MAT") == 0) return matrix_statement(&lx, data_next_number);
	if (lx.cur.type == T_REM) return 0;
	if (lx.cur.type == T_DATA) return 0;

	if (lx.cur.type == T_BYE) {
		if (duringRun) { printf("ERROR: BYE not allowed during RUN\n"); return -1; }
		exit(0);
	}

		if (lx.cur.type == T_NEW) {
		prog_clear();
		vars_clear();
		arrays_clear();
		sarrays_clear();
		g_for_top = 0;
		g_gosub_top = 0;
		files_clear();
		data_clear();
		data_mark_dirty();    /* program is empty now; table is stale */
		printf("NEW PROGRAM\n");
		return 0;
	}

	/* ON <expr> GOTO l1[,l2,...]   |   ON <expr> GOSUB l1[,l2,...] */
	if (lx.cur.type == T_ONKW) {
		lx_next(&lx);
		int n = (int)parse_rel(&lx);  // 1-based index
		int is_gosub = 0;

		if (lx.cur.type == T_GOTO) { is_gosub = 0; lx_next(&lx); }
		else if (lx.cur.type == T_GOSUB) { is_gosub = 1; lx_next(&lx); }
		else { printf("ERROR: expected GOTO or GOSUB\n"); return -1; }

		int lines[64], count = 0;
		while (lx.cur.type == T_NUMBER && count < (int)(sizeof(lines) / sizeof(lines[0]))) {
			lines[count++] = (int)lx.cur.number;
			lx_next(&lx);
			if (lx.cur.type == T_COMMA) { lx_next(&lx); continue; }
			else break;
		}
		if (count == 0) { printf("ERROR: line list expected\n"); return -1; }

		if (n >= 1 && n <= count) {
			int target = lines[n - 1];
			if (is_gosub) {
				int nextLine = next_line_number_after(currentLine);
				if (nextLine < 0) { printf("ERROR: GOSUB at last line\n"); return -1; }
				if (g_gosub_top >= MAX_STACK) { printf("ERROR: GOSUB stack overflow\n"); return -1; }
				g_gosub_stack[g_gosub_top++] = nextLine;
			}
			*outJump = target;
			return 1;
		}
		return 0; // out-of-range index -> no jump
	}

	if (lx.cur.type == T_HELP) {
		lx_next(&lx);   /* no args */
		print_help();
		return 0;
	}

	if (lx.cur.type == T_DUMP) {
		// Save current lexer
		Lexer lxSave;
		lxSave.i = lx.i;
		lxSave.s = lx.s;
		lxSave.cur.number = lx.cur.number;
		strcpy(lxSave.cur.text, lx.cur.text);
		lxSave.cur.type = lx.cur.type;

		lx_next(&lx);
		if (lx.cur.type == T_VARS) { dump_vars();   return 0; }
		if (lx.cur.type == T_ARRAYS) { dump_arrays(); return 0; }
		if (lx.cur.type == T_STACK) { dump_stack();  return 0; }
		// printf("ERROR: DUMP VARS|ARRAYS|STACK\n"); return -1;
		// Restore prev lx
		lx.i = lxSave.i;
		lx.s = lxSave.s;
		lx.cur.number = lxSave.cur.number;
		strcpy(lx.cur.text, lxSave.cur.text);
		lx.cur.type = T_PRINT;
	}

	/* LIST [start [end]] */
	if (lx.cur.type == T_LIST) {
		Token a; lx_next(&lx); a = lx.cur;
		if (a.type == T_END) { cmd_list(0, 0, 0, 0); return 0; }
		if (a.type == T_NUMBER) {
			int start = (int)a.number; Token b; lx_next(&lx); b = lx.cur;
			if (b.type == T_NUMBER) { int end = (int)b.number; cmd_list(1, start, 1, end); return 0; }
			cmd_list(1, start, 0, 0); return 0;
		}
		printf("ERROR: LIST syntax\n"); return -1;
	}

	/* program SAVE/LOAD: R9.6 error handling; preserve fixed line limits. */
	if (lx.cur.type == T_SAVE || lx.cur.type == T_LOAD)
	{
		int isLoad = (lx.cur.type == T_LOAD);
		char fname[260]; fname[0] = 0;
		if (!read_filename_after(&lx, fname, sizeof(fname))) { printf("ERROR: filename\n"); return -1; }
		if (isLoad) {
			FILE* f = fopen(fname, "rb");
			char linebuf[1024];
			int ln, nread, count = 0, valid = 1;
			if (!f) { printf("ERROR: cannot open file %s\n", fname); return -1; }
			/* Preflight with the same reader and limits as the actual load.
			   A bad record must not destroy the program already in memory. */
			for (;;) {
				nread = fscanf(f, "%d", &ln);
				if (nread == EOF) { if (ferror(f)) valid = 0; break; }
				if (nread != 1 || ln < 0 || !fgets(linebuf, sizeof(linebuf), f)) { valid = 0; break; }
				/* Do not accept a truncated physical line. No buffer-size change. */
				if (!strchr(linebuf, '\n') && !feof(f)) {
					int ch = fgetc(f);
					if (ch != EOF) { valid = 0; break; }
					if (ferror(f)) { valid = 0; break; }
				}
				if (++count > MAX_PROG_LINES) { valid = 0; break; }
			}
			if (ferror(f)) valid = 0;
			if (!valid || fseek(f, 0, SEEK_SET) != 0) {
				printf("ERROR: invalid or unreadable program file %s\n", fname);
				fclose(f); return -1;
			}
			prog_clear();
			while (fscanf(f, "%d", &ln) == 1) {
				if (fgets(linebuf, sizeof(linebuf), f)) {
					char* q = linebuf;
					if (*q == ' ') q++;
					trim(q); prog_set_line(ln, q);
				}
			}
			{
				int readFailed = ferror(f);
				if (fclose(f) != 0) readFailed = 1;
				if (readFailed) { printf("ERROR: read failure loading %s\n", fname); return -1; }
			}
			printf("Loaded %s (%d lines)\n", fname, g_prog_count);
			return 0;
		}
		else {
			FILE* f = fopen(fname, "wb"); int i, failed = 0;
			if (!f) { printf("ERROR: cannot write file %s\n", fname); return -1; }
			sort_program();
			for (i = 0; i < g_prog_count; i++) {
				if (fprintf(f, "%d %s\n", g_prog[i].number, g_prog[i].text) < 0) { failed = 1; break; }
			}
			if (fflush(f) != 0 || ferror(f)) failed = 1;
			if (fclose(f) != 0) failed = 1;
			if (failed) { printf("ERROR: write failure saving %s\n", fname); return -1; }
			printf("Saved to %s\n", fname); return 0;
		}
	}

	/* SAVEVARS / LOADVARS (scalars only, as before) */
	if (lx.cur.type == T_SAVEVARS || lx.cur.type == T_LOADVARS) {
		int isLoad = (lx.cur.type == T_LOADVARS); char fname[260]; fname[0] = 0;
		if (!read_filename_after(&lx, fname, sizeof(fname))) { printf("ERROR: filename\n"); return -1; }
		if (isLoad) {
			FILE* f = fopen(fname, "rb"); char line[512];
			if (!f) { printf("ERROR: cannot open file: %s\n", fname); return -1; }
			vars_clear();
			while (fgets(line, sizeof(line), f)) {
				char* name = strtok(line, "\t\r\n");
				char* type = strtok(NULL, "\t\r\n");
				char* val = strtok(NULL, "\r\n");
				if (name && type && val) {
					Variable* v = ensure_var(name, (type[0] == 'S'));
					if (v->type == VT_STR) { if (v->str) free(v->str); v->str = strdup_c(val); }
					else v->num = atof(val);
				}
			}
			fclose(f); printf("Variables loaded from %s (%d)\n", fname, g_var_count); return 0;
		}
		else {
			FILE* f = fopen(fname, "wb"); int i; if (!f) { printf("ERROR: cannot write file\n"); return -1; }
			for (i = 0; i < g_var_count; i++) {
				if (g_vars[i].type == VT_STR) fprintf(f, "%s\tS\t%s\n", g_vars[i].name, g_vars[i].str ? g_vars[i].str : "");
				else fprintf(f, "%s\tN\t%.15g\n", g_vars[i].name, g_vars[i].num);
			}
			fclose(f); printf("Variables saved to %s\n", fname); return 0;
		}
	}

	if (lx.cur.type == T_INPUT) {
		Lexer probe = lx;
		lx_next(&probe);
		if (probe.cur.type == T_HASH) return basic_fileio_statement(&lx, outJump);
	}

	/* Console INPUT: INPUT N[,M] or INPUT "prompt"; N */
	if (lx.cur.type == T_INPUT) {
		char input_prompt[256] = "? ";
		lx_next(&lx);
		if (lx.cur.type == T_STRING) {
			strncpy(input_prompt, lx.cur.text, sizeof(input_prompt)-1);
			input_prompt[sizeof(input_prompt)-1] = 0;
			lx_next(&lx);
			if (lx.cur.type != T_SEMI && lx.cur.type != T_COMMA) {
				printf("ERROR: INPUT prompt needs ';' or ','\n"); return -1;
			}
			lx_next(&lx);
		}
		if (lx.cur.type != T_IDENT) { printf("ERROR: INPUT needs variable\n"); return -1; }
		for (;;) {
			char name[sizeof(lx.cur.text)];
			char value[1024];
			char *end;
			double number;
			int string_var;
			if (lx.cur.type != T_IDENT) { printf("ERROR: INPUT needs variable\n"); return -1; }
			strncpy(name, lx.cur.text, sizeof(name)-1); name[sizeof(name)-1] = 0;
			string_var = is_string_var_name(name);
			lx_next(&lx);
			if (lx.cur.type != T_COMMA && lx.cur.type != T_END) {
				printf("ERROR: unsupported INPUT target\n"); return -1;
			}
			{
                int native = graphics_input(input_prompt, value, sizeof(value));
                if (native < 0) { printf("ERROR: graphics window closed during INPUT\n"); return -1; }
                if (native > 0) {
                    fputs(input_prompt, stdout); fflush(stdout);
                    if (!fgets(value, sizeof(value), stdin)) { printf("ERROR: INPUT end of file\n"); return -1; }
                }
            }
			value[strcspn(value, "\r\n")] = 0;
			if (string_var) {
				Variable *v = ensure_var(name, 1);
				char *copy = strdup_c(value);
				if (!copy) { printf("ERROR: out of memory\n"); return -1; }
				free(v->str); v->str = copy;
			} else {
				number = strtod(value, &end);
				while (*end == ' ' || *end == '\t') end++;
				if (end == value || *end) { printf("ERROR: INPUT expects number\n"); return -1; }
				ensure_var(name, 0)->num = number;
			}
			if (lx.cur.type != T_COMMA) break;
			lx_next(&lx);
		}
		return 0;
	}

	/* Sequential file I/O is implemented in basic_fileio.cpp. */
	if (lx.cur.type == T_OPEN || lx.cur.type == T_FSEEK || lx.cur.type == T_CLOSE || lx.cur.type == T_LINE)
		return basic_fileio_statement(&lx, outJump);

	/* RUN / END */
	if (lx.cur.type == T_RUN) { return 2; }

	if (lx.cur.type == T_ENDKW || lx.cur.type == T_STOP) { if (duringRun) return 9; printf("OK\n"); return 0; }

	if (lx.cur.type == T_QUIT) {
		exit(0);  /* terminate the whole app immediately */
	}

	// RENUM
	/* RENUM [start [step]] � immediate mode only */
	if (lx.cur.type == T_RENUM) {
		if (duringRun) { printf("ERROR: RENUM not allowed during RUN\n"); return -1; }

		/* defaults */
		int start = 10, step = 10;
		lx_next(&lx);
		if (lx.cur.type == T_NUMBER) {
			start = (int)lx.cur.number; lx_next(&lx);
			if (lx.cur.type == T_NUMBER) { step = (int)lx.cur.number; lx_next(&lx); }
		}

		if (g_prog_count <= 0) { printf("NO PROGRAM\n"); return 0; }

		sort_program();

		/* Build old->new map */
		int i; int* oldL = (int*)malloc(sizeof(int) * g_prog_count);
		int* newL = (int*)malloc(sizeof(int) * g_prog_count);
		if (!oldL || !newL) { printf("ERROR: OUT OF MEMORY\n"); if (oldL)free(oldL); if (newL)free(newL); return -1; }

		for (i = 0; i < g_prog_count; i++) {
			oldL[i] = g_prog[i].number;
			newL[i] = start + i * step;
		}

		/* Rewrite each line's text for THEN/GOTO/GOSUB numeric targets */
		for (i = 0; i < g_prog_count; i++) {
			char* re = renum_rewrite_stmt(g_prog[i].text, oldL, newL, g_prog_count);
			if (re) {
				free(g_prog[i].text);
				g_prog[i].text = re;
			}
		}

		/* Apply new line numbers */
		for (i = 0; i < g_prog_count; i++) {
			g_prog[i].number = newL[i];
		}

		free(oldL); free(newL);
		sort_program();
		printf("RENUM OK (start=%d, step=%d)\n", start, step);
		data_mark_dirty();
		return 0;
	}

	// PRINT 
	if (lx.cur.type == T_PRINT) {
		return exec_print(&lx);
	}

	/* IF <cond> THEN
		  <line>
		| GOTO <line>
		| GOSUB <line>
		| <single statement>   (e.g., LET X=1 or X=1 or PRINT ... )
	*/
	/* IF <cond> THEN
		 <line>
	   | GOTO <line>
	   | GOSUB <line>
	   | <single statement>  (PRINT �, LET �, or <ident>=�)
	*/
	/* IF <cond> THEN
	 <line>
   | GOTO <line>
   | GOSUB <line>
   | <single statement>
   [ ELSE <single statement> ]
*/
	if (lx.cur.type == T_IF) {
		/* Evaluate the condition once, then execute exactly one branch.
		   Colons and backslashes are statement separators inside branches. */
		lx_next(&lx);
		double cond = parse_rel(&lx);
		if (lx.cur.type != T_THEN && lx.cur.type != T_GOTO) {
			printf("ERROR: THEN or GOTO expected\n");
			return -1;
		}
		/* Lexer offset is immediately after THEN or MSX shorthand GOTO. */
		const char *thenStart = src + lx.i;
		const char *elsePos = NULL;
		int quoted = 0;
		for (const char *q = thenStart; *q; ++q) {
			if (*q == '"') quoted = !quoted;
			if (!quoted && (q == thenStart || !isalnum((unsigned char)q[-1])) &&
			    (q[0]=='E'||q[0]=='e') && (q[1]=='L'||q[1]=='l') &&
			    (q[2]=='S'||q[2]=='s') && (q[3]=='E'||q[3]=='e') &&
			    !isalnum((unsigned char)q[4]) && q[4]!='_') {
				elsePos = q; break;
			}
		}
		const char *chosen = NULL;
		size_t chosenLen = 0;
		if (cond != 0.0) {
			chosen = thenStart;
			chosenLen = elsePos ? (size_t)(elsePos - thenStart) : strlen(thenStart);
		} else if (elsePos) {
			chosen = elsePos + 4;
			chosenLen = strlen(chosen);
		}
		if (!chosen) return 0;
		while (chosenLen && isspace((unsigned char)*chosen)) { chosen++; chosenLen--; }
		while (chosenLen && isspace((unsigned char)chosen[chosenLen-1])) chosenLen--;
		if (!chosenLen) return 0;
		char *branch = (char*)malloc(chosenLen + 1);
		if (!branch) { printf("ERROR: out of memory\n"); return -1; }
		memcpy(branch, chosen, chosenLen); branch[chosenLen] = 0;
		/* Bare line number after THEN or ELSE means GOTO. */
		char *endNumber = NULL;
		long target = strtol(branch, &endNumber, 10);
		while (endNumber && isspace((unsigned char)*endNumber)) endNumber++;
		int rv;
		if (endNumber && endNumber != branch && !*endNumber) {
			*outJump = (int)target; rv = 1;
		} else {
			rv = exec_multi(branch, duringRun, currentLine, outJump);
		}
		free(branch);
		return rv;
	}

	if (lx.cur.type == T_GOTO) { lx_next(&lx); if (lx.cur.type != T_NUMBER) { printf("ERROR: GOTO needs line\n"); return -1; } *outJump = (int)lx.cur.number; return 1; }

	if (lx.cur.type == T_GOSUB)
	{
		int nextLine = next_line_number_after(currentLine); lx_next(&lx);
		if (lx.cur.type != T_NUMBER) { printf("ERROR: GOSUB needs line\n"); return -1; }
		if (nextLine < 0) { printf("ERROR: GOSUB at last line\n"); return -1; }
		if (g_gosub_top >= MAX_STACK) { printf("ERROR: GOSUB stack overflow\n"); return -1; }
		g_gosub_stack[g_gosub_top++] = nextLine; *outJump = (int)lx.cur.number; return 1;
	}

	if (lx.cur.type == T_RETURN) { if (g_gosub_top <= 0) { printf("ERROR: RETURN without GOSUB\n"); return -1; } *outJump = g_gosub_stack[--g_gosub_top]; return 1; }

	/* FOR / NEXT */
	if (lx.cur.type == T_FOR)
	{
		char vname[32]; double start, toVal, step = 1.0; int afterFor;
		lx_next(&lx); if (lx.cur.type != T_IDENT) { printf("ERROR: FOR needs var\n"); return -1; }
		strncpy(vname, lx.cur.text, sizeof(vname) - 1); vname[sizeof(vname) - 1] = 0;
		lx_next(&lx); if (lx.cur.type != T_EQ) { printf("ERROR: FOR needs '='\n"); return -1; }
		lx_next(&lx); start = parse_rel(&lx);
		if (lx.cur.type != T_TO) { printf("ERROR: FOR needs TO\n"); return -1; }
		lx_next(&lx); toVal = parse_rel(&lx);
		if (lx.cur.type == T_STEP) { lx_next(&lx); step = parse_rel(&lx); }
		ensure_var(vname, 0)->num = start;
		afterFor = next_line_number_after(currentLine);
		if (afterFor < 0) { printf("ERROR: FOR cannot be last line\n"); return -1; }
		if (g_for_top >= MAX_STACK) { printf("ERROR: FOR stack overflow\n"); return -1; }
		strncpy(g_for_stack[g_for_top].var, vname, sizeof(g_for_stack[g_for_top].var) - 1);
		g_for_stack[g_for_top].var[sizeof(g_for_stack[g_for_top].var) - 1] = 0;
		g_for_stack[g_for_top].end = toVal; g_for_stack[g_for_top].step = step;
		g_for_stack[g_for_top].afterForLine = afterFor; g_for_stack[g_for_top].forLine = currentLine;
		g_for_top++; return 0;
	}

	if (lx.cur.type == T_NEXT)
	{
		int idx = g_for_top - 1; lx_next(&lx);
		if (lx.cur.type == T_IDENT) {
			int k; for (k = g_for_top - 1; k >= 0; k--) { if (_stricmp(g_for_stack[k].var, lx.cur.text) == 0) { idx = k; break; } }
			if (k < 0) { printf("ERROR: NEXT for unknown FOR var\n"); return -1; }
		}
		if (idx < 0) { printf("ERROR: NEXT without FOR\n"); return -1; }
		{
			ForFrame fr = g_for_stack[idx]; Variable* v = ensure_var(fr.var, 0);
			double cur = v->num + fr.step; int cont = (fr.step >= 0) ? (cur <= fr.end) : (cur >= fr.end);
			v->num = cur; if (cont) { *outJump = fr.afterForLine; return 1; }
			else { int m; for (m = idx; m < g_for_top - 1; m++) g_for_stack[m] = g_for_stack[m + 1]; g_for_top--; return 0; }
		}
	}

	// TRACE ON/OFF
/* TRACE ON|OFF */
	if (lx.cur.type == T_TRACE) {
		lx_next(&lx);
		int on = g_trace; // default to ON if omitted
		
		if ((lx.cur.type == T_ON) || (lx.cur.type == T_ONKW))
			on = 1;
		else if (lx.cur.type == T_OFF)
			on = 0;
		else if (lx.cur.type == T_IDENT) {
			char up[8]; strncpy(up, lx.cur.text, 7); up[7] = 0;
			for (int i = 0; up[i]; ++i) up[i] = (char)toupper((unsigned char)up[i]);
			if (!strcmp(up, "ON"))  on = 1;
			else if (!strcmp(up, "OFF")) on = 0;
			else { printf("ERROR: TRACE expects ON or OFF\n"); return -1; }
			lx_next(&lx);
		}
		g_trace = on;
		printf("TRACE %s\n", on ? "ON" : "OFF");
		return 0;
	}

	/* Handle DIM (numeric + string arrays) */
	if (lx.cur.type == T_DIM)
	{
		for (;;) {
			char aname[32]; int dims[MAX_DIMS]; int nd = 0;
			lx_next(&lx);
			if (lx.cur.type != T_IDENT) { printf("ERROR: DIM needs name\n"); return -1; }
			strncpy(aname, lx.cur.text, sizeof(aname) - 1); aname[sizeof(aname) - 1] = 0;
			lx_next(&lx);
			if (lx.cur.type != T_LPAREN) { printf("ERROR: DIM needs '('\n"); return -1; }
			lx_next(&lx);
			while (lx.cur.type != T_RPAREN && lx.cur.type != T_END) {
				if (nd >= MAX_DIMS) { printf("ERROR: > %d DIMENSIONS\n", MAX_DIMS); return -1; }
				dims[nd++] = (int)parse_rel(&lx); /* sizes; zero-based indexing for elements */
				if (lx.cur.type == T_COMMA) { lx_next(&lx); continue; }
				else break;
			}
			if (lx.cur.type != T_RPAREN) { printf("ERROR: DIM missing ')'\n"); return -1; }
			lx_next(&lx);
			if (is_string_var_name(aname)) {
				if (!sarray_dim(aname, nd, dims)) return -1;
			}
			else {
				if (!array_dim(aname, nd, dims)) return -1;
			}
			if (lx.cur.type == T_COMMA) { /* DIM A(10),B$(2,2) */ continue; }
			break;
		}
		return 0;
	}

	if (lx.cur.type == T_RESTORE) {
		lx_next(&lx);
		if (!g_data_built) data_build_from_program();

		if (lx.cur.type == T_NUMBER) {
			int ln = (int)lx.cur.number;
			lx_next(&lx);
			{
                int pos = 0;
                while (pos < g_data_count && g_data_lines[pos] < ln) ++pos;
                g_data_ptr = pos;
            }
		}
		else {
			g_data_ptr = 0;
		}
		return 0;
	}

	if (lx.cur.type == T_READ)
	{
		/* Lazily build DATA pool on first READ */
		if (!g_data_built) data_build_from_program();

		for (;;) {
			lx_next(&lx);
			if (lx.cur.type != T_IDENT) { printf("ERROR: READ needs variable\n"); return -1; }
			/* capture name and whether string */
			char name[32]; int isStr = is_string_var_name(lx.cur.text);
			strncpy(name, lx.cur.text, sizeof(name) - 1); name[sizeof(name) - 1] = 0;
			lx_next(&lx);
			/* Array element? */
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
				if (isStr) {
					SArray* sa = sarray_find(name);
					if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
					sarray_set(sa, subs, nsubs, data_next_string());
				}
				else {
					Array* a = array_find(name);
					if (!a) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
					array_set(a, subs, nsubs, data_next_number());
				}
			}
			else {
				/* scalar */
				if (isStr) {
					Variable* v = ensure_var(name, 1);
					if (!v) return -1;
					if (v->str) { free(v->str); v->str = NULL; }
					v->str = strdup_c(data_next_string());
				}
				else {
					ensure_var(name, 0)->num = data_next_number();
				}
			}
			/* More variables? READ A,B$,C(1) */
			if (lx.cur.type == T_COMMA) continue;
			break;
		}
		return 0;
	}
	return exec_assignment_tail(lx, duringRun, currentLine);
}
/* Reset the READ pool at the beginning of each RUN. */
void exec_data_reset_for_run(void) { data_clear(); }
