
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "runtime.h"
#include "parse.h"
#include "wxecut.h"

/* --------- Globals --------- */
ProgLine g_prog[MAX_PROG_LINES]; 
int g_prog_count = 0;

Variable g_vars[MAX_VARS]; 
int g_var_count = 0;

ForFrame g_for_stack[MAX_STACK]; 
int g_for_top = 0;

int g_gosub_stack[MAX_STACK]; 
int g_gosub_top = 0;

FileSlot g_files[MAX_FILES];

/* arrays */
Array g_arrays[MAX_ARRAYS]; 
int g_array_count = 0;

/* string arrays */
SArray g_sarrays[MAX_SARRAYS]; 
int g_sarray_count = 0;

int g_trace = 0;

/* --------- Utilities & program mgmt --------- */

char* strdup_c(const char* s) { size_t n = strlen(s); char* p = (char*)malloc(n + 1); if (p) memcpy(p, s, n + 1); return p; }
char* trim(char* s) { char* a = s, * b = s + strlen(s); while (a < b && isspace((unsigned char)*a))++a; while (b > a && isspace((unsigned char)b[-1]))--b; memmove(s, a, (size_t)(b - a)); s[b - a] = '\0'; return s; }
static int cmp_lines(const void* a, const void* b) { const ProgLine* pa = (const ProgLine*)a; const ProgLine* pb = (const ProgLine*)b; return (pa->line > pb->line) - (pa->line < pb->line); }
void sort_program(void) { qsort(g_prog, (size_t)g_prog_count, sizeof(ProgLine), cmp_lines); }
int find_prog_index_by_line(int line) { int lo = 0, hi = g_prog_count - 1; while (lo <= hi) { int mid = (lo + hi) / 2; if (g_prog[mid].line == line) return mid; if (g_prog[mid].line < line) lo = mid + 1; else hi = mid - 1; } return -1; }
int next_line_number_after(int current) { int i; sort_program(); for (i = 0; i < g_prog_count; i++) { if (g_prog[i].line > current) return g_prog[i].line; } return -1; }
Variable* find_var(const char* name) { int i; for (i = 0; i < g_var_count; i++) { if (_stricmp(g_vars[i].name, name) == 0) return &g_vars[i]; } return NULL; }
int is_string_var_name(const char* name) { size_t n = strlen(name); return n > 0 && name[n - 1] == '$'; }
Variable* ensure_var(const char* name, int isStr) { Variable* v = find_var(name); if (!v) { if (g_var_count >= MAX_VARS) return NULL; v = &g_vars[g_var_count++]; memset(v, 0, sizeof(*v)); strncpy(v->name, name, sizeof(v->name) - 1); v->type = isStr ? VT_STR : VT_NUM; v->num = 0.0; } else { v->type = isStr ? VT_STR : VT_NUM; } return v; }

static size_t safe_mul(size_t a, size_t b) { return (a == 0 || b == 0) ? 0 : (a * b); }

Array* array_find(const char* name) {
	int i; for (i = 0; i < g_array_count; i++) if (_stricmp(g_arrays[i].name, name) == 0) return &g_arrays[i];
	return NULL;
}

Array* array_dim(const char* name, int ndims, int* dims) {
	int i; size_t total = 1;
	if (ndims <= 0 || ndims > MAX_DIMS) return NULL;
	for (i = 0; i < ndims; i++) { if (dims[i] < 1) dims[i] = 1; total = safe_mul(total, (size_t)dims[i]); }
	{
		Array* a = array_find(name);
		if (!a) {
			if (g_array_count >= MAX_ARRAYS) { printf("ERROR: ARRAY TABLE FULL\n"); return NULL; }
			a = &g_arrays[g_array_count++]; memset(a, 0, sizeof(*a)); strncpy(a->name, name, sizeof(a->name) - 1);
		}
		else if (a->data) { free(a->data); a->data = NULL; }
		a->ndims = ndims; for (i = 0; i < ndims; i++) a->dims[i] = dims[i];
		a->data = (double*)calloc(total, sizeof(double));
		if (!a->data) { printf("ERROR: OUT OF MEMORY\n"); return NULL; }
		return a;
	}
}

int array_index(Array* a, int* subs, int nsubs) {
	int i; size_t idx = 0, stride = 1;
	if (!a || nsubs != a->ndims) return -1;
	for (i = a->ndims - 1; i >= 0; i--) {
		int s = subs[i];
		if (s < 0 || s >= a->dims[i]) return -1;  /* zero-based indices */
		idx += (size_t)s * stride;
		stride *= (size_t)a->dims[i];
	}
	if (idx >= (size_t)-1) return -1;
	return (int)idx;
}

double array_get(Array* a, int* subs, int nsubs) {
	int k = array_index(a, subs, nsubs);
	if (k < 0) { printf("ERROR: SUBSCRIPT\n"); return 0.0; }
	return a->data[k];
}

void array_set(Array* a, int* subs, int nsubs, double val) {
	int k = array_index(a, subs, nsubs);
	if (k < 0) { printf("ERROR: SUBSCRIPT\n"); return; }
	a->data[k] = val;
}

void arrays_clear(void) {
	int i; for (i = 0; i < g_array_count; i++) { if (g_arrays[i].data) free(g_arrays[i].data); g_arrays[i].data = NULL; }
	g_array_count = 0;
}

SArray* sarray_find(const char* name) 
{
	int i; for (i = 0; i < g_sarray_count; i++) if (_stricmp(g_sarrays[i].name, name) == 0) return &g_sarrays[i];
	return NULL;
}

SArray* sarray_dim(const char* name, int ndims, int* dims) {
	int i; size_t total = 1;
	if (ndims <= 0 || ndims > MAX_DIMS) return NULL;
	for (i = 0; i < ndims; i++) { if (dims[i] < 1) dims[i] = 1; total = (total == 0 ? 0 : total * (size_t)dims[i]); }
	{
		SArray* a = sarray_find(name);
		if (!a) {
			if (g_sarray_count >= MAX_SARRAYS) { printf("ERROR: STRING ARRAY TABLE FULL\n"); return NULL; }
			a = &g_sarrays[g_sarray_count++]; memset(a, 0, sizeof(*a)); strncpy(a->name, name, sizeof(a->name) - 1);
		}
		else if (a->data) {
			size_t n = 1; for (i = 0; i < a->ndims; i++) n *= (size_t)a->dims[i];
			for (size_t k = 0; k < n; k++) { if (a->data[k]) free(a->data[k]); }
			free(a->data); a->data = NULL;
		}
		a->ndims = ndims; for (i = 0; i < ndims; i++) a->dims[i] = dims[i];
		a->data = (char**)calloc(total, sizeof(char*));
		if (!a->data) { printf("ERROR: OUT OF MEMORY\n"); return NULL; }
		return a;
	}
}

int sarray_index(SArray* a, int* subs, int nsubs) {
	int i; size_t idx = 0, stride = 1;
	if (!a || nsubs != a->ndims) return -1;
	for (i = a->ndims - 1; i >= 0; i--) {
		int s = subs[i]; if (s < 0 || s >= a->dims[i]) return -1;
		idx += (size_t)s * stride; stride *= (size_t)a->dims[i];
	}
	return (int)idx;
}
const char* sarray_get(SArray* a, int* subs, int nsubs) {
	int k = sarray_index(a, subs, nsubs);
	if (k < 0) { printf("ERROR: SUBSCRIPT\n"); return ""; }
	return a->data[k] ? a->data[k] : "";
}
void sarray_set(SArray* a, int* subs, int nsubs, const char* val) {
	int k = sarray_index(a, subs, nsubs);
	if (k < 0) { printf("ERROR: SUBSCRIPT\n"); return; }
	if (a->data[k]) { free(a->data[k]); a->data[k] = NULL; }
	a->data[k] = strdup_c(val ? val : "");
}
void sarrays_clear(void) {
	int i; for (i = 0; i < g_sarray_count; i++) {
		if (g_sarrays[i].data) {
			size_t n = 1; int d; for (d = 0; d < g_sarrays[i].ndims; d++) n *= (size_t)g_sarrays[i].dims[d];
			for (size_t k = 0; k < n; k++) if (g_sarrays[i].data[k]) free(g_sarrays[i].data[k]);
			free(g_sarrays[i].data); g_sarrays[i].data = NULL;
		}
	}
	g_sarray_count = 0;
}


void prog_set_line(int line, const char* text) {
	int i = find_prog_index_by_line(line);
	if (!text || !text[0])
	{
		if (i >= 0)
		{
			free(g_prog[i].text);
			for (; i < g_prog_count - 1; i++)
				g_prog[i] = g_prog[i + 1];

			g_prog_count--;
		}
		return;
	}
	if (i >= 0)
	{
		free(g_prog[i].text);
		g_prog[i].text = strdup_c(text);
	}
	else
	{
		if (g_prog_count >= MAX_PROG_LINES)
		{
			printf("PROGRAM FULL\n");
			return;
		}

		g_prog[g_prog_count].line = line;
		g_prog[g_prog_count].text = strdup_c(text);
		g_prog_count++;
		sort_program();
	}
}

void prog_clear(void)
{
	int i;
	for (i = 0; i < g_prog_count; i++)
	{
		free(g_prog[i].text);
		g_prog[i].text = NULL;
	}
	g_prog_count = 0;
}

void vars_clear(void) 
{ 
	int i; 
	
	for (i = 0; i < g_var_count; i++) 
	{ 
		if (g_vars[i].type == VT_STR && g_vars[i].str) 
			free(g_vars[i].str); 
		g_vars[i].str = NULL; 
	} 
	g_var_count = 0; 
}

void files_clear(void)
{
	int i; 
	for (i = 0; i < MAX_FILES; i++) 
	{ 
		if (g_files[i].used && g_files[i].fp) 
		{ 
			fclose(g_files[i].fp); 
		} 
		
		g_files[i].used = 0; 
		g_files[i].fp = NULL; 
	}
}

void cmd_list(int startGiven, int start, int endGiven, int end) {
	int i; sort_program();
	if (!startGiven) { for (i = 0; i < g_prog_count; i++) printf("%d %s\n", g_prog[i].line, g_prog[i].text); return; }
	if (startGiven && !endGiven) { for (i = 0; i < g_prog_count; i++) if (g_prog[i].line >= start) printf("%d %s\n", g_prog[i].line, g_prog[i].text); return; }
	for (i = 0; i < g_prog_count; i++) if (g_prog[i].line >= start && g_prog[i].line <= end) printf("%d %s\n", g_prog[i].line, g_prog[i].text);
}

/* Execute multiple statements on one physical line.
   Split on ':' or '\' unless inside a quoted string. */
static int exec_multi(const char* src, int duringRun, int currentLine, int* outJump)
{
	char seg[MAX_LINE_LEN];
	size_t n = strlen(src);
	size_t start = 0;
	int inq = 0;
	size_t i = 0;

	while (1) 
	{
		char c = (i < n) ? src[i] : '\0';
		if (c == '"') inq = !inq;
		if ((!inq && (c == ':' || c == '\\')) || c == '\0') {
			size_t len = (i > start) ? (i - start) : 0;
			if (len) {
				if (len >= sizeof(seg)) len = sizeof(seg) - 1;
				memcpy(seg, src + start, len);
				seg[len] = 0;
				trim(seg);
				if (seg[0]) {
					int rc = exec_statement(seg, duringRun, currentLine, outJump);
					/* Propagate any non-zero result: jump(1), run(2), end(9), or error(<0) */
					if (rc != 0) return rc;
				}
			}
			if (c == '\0') break;
			start = i + 1;
		}
		i++;
	}
	return 0;
}

/* --------- Runner --------- */
static void run_program(void) {
	int pcIndex = 0;

	if (g_prog_count <= 0)
	{
		printf("NO PROGRAM\n");
		return;
	}

	sort_program();
	g_for_top = 0;
	g_gosub_top = 0;

	while (pcIndex < g_prog_count) {
		int curLine = g_prog[pcIndex].line; 
		const char* src = g_prog[pcIndex].text; 
		int code, jump = 0;

		{
			if (g_trace) printf("[TRACE] %d %s\n", curLine, src);

			char tmp[MAX_LINE_LEN]; 
			strncpy(tmp, src, sizeof(tmp) - 1); 
			tmp[sizeof(tmp) - 1] = 0; trim(tmp);

			if (tmp[0] == 0) 
			{ 
				pcIndex++; 
				continue; 
			}
			
			{ 
				Lexer lx; 
				lx_init(&lx, tmp); 
				lx_next(&lx); 
				if (lx.cur.type == T_REM) 
				{ 
					pcIndex++; 
					continue; 
				} 
			}
		}
		
		code = exec_multi(src, 1, curLine, &jump);

		if (code == 9) 
			return;
		else if (code < 0) 
			return;
		else if (code == 1) 
		{
			int idx = find_prog_index_by_line(jump); 
			
			if (idx < 0) 
			{ 
				printf("ERROR: Undefined line %d\n", jump); 
				return; 
			} 
			pcIndex = idx;
		}
		else pcIndex++;
	}
}

/* --------- UI --------- */
static int starts_with_kw(const char* s, const char* kw) 
{ 
	size_t i, n = strlen(kw); 
	
	if (strlen(s) < n) 
		return 0; 
	
	for (i = 0; i < n; i++) 
		if (toupper((unsigned char)s[i]) != toupper((unsigned char)kw[i])) 
			return 0; 
	
	return 1; 
}

static void print_help(void) 
{
	printf("Commands: NEW, LIST [start [end]], RUN, SAVE/LOAD \"file\"\n");
	printf("Variables: SAVEVARS/LOADVARS \"file\"\n");
	printf("File I/O: OPEN \"name\" FOR INPUT|OUTPUT|APPEND AS #n; CLOSE [#n]; PRINT #n, ...; INPUT #n, var; LINE INPUT #n, var$\n");
	printf("Statements: PRINT, INPUT, LET, IF <rel> THEN <line>, GOTO, GOSUB/RETURN, END/STOP, REM; FOR..NEXT\n");
	printf("Relational ops: = <> < > <= >= (true=1, false=0)\n");
}

int main(void) 
{
	char line[MAX_LINE_LEN];
	memset(g_files, 0, sizeof(g_files));

	printf("Linter - The interpreter of BASIC Programming language\n");
	printf("           Yuri Starikov - 1986 - 2025.\n");
	printf("           This version was written on C++\n");
	printf("Version: %s at %s\n", __DATE__, __TIME__);
	printf("           Type HELP for help\nREADY.\n");

	for (;;) {
		printf("> ");
	
		if (!fgets(line, sizeof(line), stdin)) break;
		trim(line);
		
		if (line[0] == 0) continue;
		if (starts_with_kw(line, "HELP")) { print_help(); continue; }
		{
			int ln = 0; char* p = line; while (isdigit((unsigned char)*p)) { ln = ln * 10 + (*p - '0'); p++; }
			if (ln > 0) { while (*p == ' ') p++; if (*p == 0) prog_set_line(ln, NULL); else prog_set_line(ln, p); continue; }
		}
		{
			int code, jump = 0; if (starts_with_kw(line, "RUN")) { run_program(); continue; }
			code = exec_statement(line, 0, -1, &jump); if (code == 2) run_program();
		}
	}
	files_clear();
	return 0;
}
