
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "runtime.h"
#include "parse.h"
#include "wxecut.h"
#include "printfunc.h"

/* --- exec helpers (no parsing here) --- */
static int read_filename_after(Lexer*lx, char*out, size_t outsz){
    lx_next(lx);
    if(lx->cur.type==T_STRING){ strncpy(out,lx->cur.text,outsz-1); out[outsz-1]=0; return 1; }
    if(lx->cur.type==T_IDENT){ strncpy(out,lx->cur.text,outsz-1); out[outsz-1]=0; return 1; }
    return 0;
}
static FILE* file_from_handle(int n){
    if(n<0 || n>=MAX_FILES) return NULL;
    if(!g_files[n].used) return NULL;
    return g_files[n].fp;
}
static int read_token_from_file(FILE*fp, char*buf, size_t cap){
    int c, i=0;
    do { c=fgetc(fp); if(c==EOF) return 0; } while(c==' '||c=='\t'||c=='\r'||c=='\n'||c==',');
    while(c!=EOF && c!=',' && c!='\n' && c!='\r'){ if(i+1<(int)cap) buf[i++]=(char)c; c=fgetc(fp); }
    buf[i]='\0'; return 1;
}
static int read_line_from_file(FILE*fp, char*buf, size_t cap){
    if(!fgets(buf, (int)cap, fp)) return 0; buf[cap-1]=0; trim(buf); return 1;
}

/* -------- DATA pool (lazy build) -------- */
static char** g_data_vals = NULL;
static int    g_data_count = 0;
static int    g_data_ptr = 0;
static int    g_data_built = 0;

static void data_clear(void) {
    int i;
    if (g_data_vals) {
        for (i = 0; i < g_data_count; i++) if (g_data_vals[i]) free(g_data_vals[i]);
        free(g_data_vals);
    }
    g_data_vals = NULL; g_data_count = 0; g_data_ptr = 0; g_data_built = 0;
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

/* ----------------- executor ----------------- */
int exec_statement(const char* src, int duringRun, int currentLine, int* outJump)
{
	Lexer lx;
	lx_init(&lx, src);
	lx_next(&lx);

	if (lx.cur.type == T_REM) return 0;
	if (lx.cur.type == T_DATA) return 0;

	if (lx.cur.type == T_BYE) {
		if (duringRun) { printf("ERROR: BYE not allowed during RUN\n"); return -1; }
		exit(0);
	}

	/* Example: handle NEW command */
	if (lx.cur.type == T_NEW) {
		prog_clear();
		vars_clear();
		arrays_clear();
		sarrays_clear();
		g_for_top = 0;
		g_gosub_top = 0;
		files_clear();
		data_clear();
		printf("NEW PROGRAM\n");
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

	/* program SAVE/LOAD */
	if (lx.cur.type == T_SAVE || lx.cur.type == T_LOAD)
	{
		int isLoad = (lx.cur.type == T_LOAD);
		char fname[260]; fname[0] = 0;
		if (!read_filename_after(&lx, fname, sizeof(fname))) { printf("ERROR: filename\n"); return -1; }
		if (isLoad) {
			FILE* f = fopen(fname, "rb"); char linebuf[1024]; int ln;
			if (!f) { printf("ERROR: cannot open file\n"); return -1; }
			prog_clear();
			while (fscanf(f, "%d", &ln) == 1) {
				if (fgets(linebuf, sizeof(linebuf), f)) { char* p = linebuf; if (*p == ' ') p++; trim(p); prog_set_line(ln, p); }
			}
			fclose(f); printf("Loaded %s (%d lines)\n", fname, g_prog_count); return 0;
		}
		else {
			FILE* f = fopen(fname, "wb"); int i; if (!f) { printf("ERROR: cannot write file %s\n", fname); return -1; }
			sort_program(); for (i = 0; i < g_prog_count; i++) fprintf(f, "%d %s\n", g_prog[i].line, g_prog[i].text);
			fclose(f); printf("Saved to %s\n", fname); return 0;
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

	/* FILE I/O */
	if (lx.cur.type == T_OPEN) {
		char fname[260]; fname[0] = 0; int mode = 0; int handle = -1; FILE* fp;
		if (!read_filename_after(&lx, fname, sizeof(fname))) { printf("ERROR: OPEN needs filename\n"); return -1; }
		if (lx.cur.type != T_FOR) { printf("ERROR: OPEN needs FOR\n"); return -1; }
		lx_next(&lx);
		if (lx.cur.type == T_INPUT) mode = 0;
		else if (lx.cur.type == T_OUTPUTKW) mode = 1;
		else if (lx.cur.type == T_APPEND) mode = 2;
		else { printf("ERROR: OPEN mode\n"); return -1; }
		lx_next(&lx);
		if (lx.cur.type != T_AS) { printf("ERROR: OPEN needs AS\n"); return -1; }
		lx_next(&lx);
		if (lx.cur.type == T_HASH) lx_next(&lx);
		if (lx.cur.type != T_NUMBER) { printf("ERROR: OPEN needs handle number\n"); return -1; }
		handle = (int)lx.cur.number;
		if (handle < 0 || handle >= MAX_FILES) { printf("ERROR: handle out of range\n"); return -1; }
		if (g_files[handle].used) { printf("ERROR: handle already open\n"); return -1; }
		if (mode == 0) fp = fopen(fname, "rb"); else if (mode == 1) fp = fopen(fname, "wb"); else fp = fopen(fname, "ab");
		if (!fp) { printf("ERROR: cannot open file\n"); return -1; }
		g_files[handle].used = 1; g_files[handle].fp = fp; return 0;
	}

	if (lx.cur.type == T_CLOSE) {
		lx_next(&lx);
		if (lx.cur.type == T_HASH || lx.cur.type == T_NUMBER) {
			int handle; if (lx.cur.type == T_HASH) lx_next(&lx);
			if (lx.cur.type != T_NUMBER) { printf("ERROR: CLOSE needs number\n"); return -1; }
			handle = (int)lx.cur.number;
			if (handle >= 0 && handle < MAX_FILES && g_files[handle].used) { fclose(g_files[handle].fp); g_files[handle].used = 0; g_files[handle].fp = NULL; }
		}
		else { files_clear(); }
		return 0;
	}

	/* RUN / END */
	if (lx.cur.type == T_RUN) { return 2; }

	if (lx.cur.type == T_ENDKW || lx.cur.type == T_STOP) { if (duringRun) return 9; printf("OK\n"); return 0; }

	if (lx.cur.type == T_QUIT) {
		exit(0);  /* terminate the whole app immediately */
	}

	// RENUM
	/* RENUM [start [step]] — immediate mode only */
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
			oldL[i] = g_prog[i].line;
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
			g_prog[i].line = newL[i];
		}

		free(oldL); free(newL);
		sort_program();
		printf("RENUM OK (start=%d, step=%d)\n", start, step);
		return 0;
	}

	// PRINT 
	if (lx.cur.type == T_PRINT) {
		return exec_print(&lx);
	}

	/* IF <cond> THEN [GOTO|GOSUB] <line> */
	if (lx.cur.type == T_IF) {
		double cond;
		lx_next(&lx);
		cond = parse_rel(&lx);                  /* full boolean parsing incl. AND/OR/XOR */

		if (lx.cur.type != T_THEN) { printf("ERROR: THEN expected\n"); return -1; }
		lx_next(&lx);

		/* if false, ignore trailing tokens in this statement */
		if (cond == 0.0) return 0;

		/* THEN <line>  */
		if (lx.cur.type == T_NUMBER) {
			*outJump = (int)lx.cur.number;
			return 1;
		}

		/* THEN GOTO <line> */
		if (lx.cur.type == T_GOTO) {
			lx_next(&lx);
			if (lx.cur.type != T_NUMBER) { printf("ERROR: line number expected\n"); return -1; }
			*outJump = (int)lx.cur.number;
			return 1;
		}

		/* THEN GOSUB <line> */
		if (lx.cur.type == T_GOSUB) {
			int nextLine = next_line_number_after(currentLine);
			lx_next(&lx);
			if (lx.cur.type != T_NUMBER) { printf("ERROR: line number expected\n"); return -1; }
			if (nextLine < 0) { printf("ERROR: GOSUB at last line\n"); return -1; }
			if (g_gosub_top >= MAX_STACK) { printf("ERROR: GOSUB stack overflow\n"); return -1; }
			g_gosub_stack[g_gosub_top++] = nextLine;
			*outJump = (int)lx.cur.number;
			return 1;
		}

		printf("ERROR: line number expected\n");
		return -1;
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
	if (lx.cur.type == T_TRACE)
	{
		lx_next(&lx);
		if (lx.cur.type == T_ON) { g_trace = 1; return 0; }
		if (lx.cur.type == T_OFF) { g_trace = 0; return 0; }
		printf("ERROR Line %d: TRACE ON|OFF\n", currentLine); return -1;
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

	if (lx.cur.type == T_RESTORE)
	{
		/* BASIC RESTORE resets the DATA pointer to the start */
		if (!g_data_built) data_build_from_program(); /* ok if already built */
		g_data_ptr = 0;
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
					SArray* sa = sarray_find(name);
					if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
					sarray_set(sa, subs, nsubs, lx.cur.text); lx_next(&lx);
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

		if (isStr) {
			if (lx.cur.type == T_STRING)
			{
				Variable* v = ensure_var(name, 1);
				if (!v) return -1;
				if (v->str) free(v->str); v->str = strdup_c(lx.cur.text); lx_next(&lx);
			}
			else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
				char sname[32]; strncpy(sname, lx.cur.text, sizeof(sname) - 1); sname[sizeof(sname) - 1] = 0; lx_next(&lx);
				if (_stricmp(sname, "CHR$") == 0) {
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

