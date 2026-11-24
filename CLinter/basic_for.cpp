#include "runtime.h"
#include "parse.h"
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
int exec_for_next(Lexer lx, int currentLine, int *outJump) {
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
		if (!isfinite(start)||!isfinite(toVal)||!isfinite(step)||step==0 || start+step==start) { printf("ERROR: FOR step or range\n"); return -1; }
		set_numeric_var(ensure_var(vname,0),start);
        if (parse_numeric_failed()) return -1;
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
			double cur = v->num + fr.step;
            double tol = fmin(fabs(fr.step)*0.25, 8*DBL_EPSILON*fmax(fabs(v->num),fmax(fabs(fr.end),fabs(fr.step))));
            if (!isfinite(cur)||cur==v->num) { printf("ERROR: NEXT range or step\n"); return -1; }
            if (fabs(cur-fr.end)<=tol) cur=fr.end;
            if (!assign_numeric_value(v->name,cur,&cur) || cur==v->num) { parse_numeric_error(); return -1; }
            int cont = (fr.step >= 0) ? (cur <= fr.end) : (cur >= fr.end);
			v->num = cur; if (cont) { *outJump = fr.afterForLine; return 1; }
			else { int m; for (m = idx; m < g_for_top - 1; m++) g_for_stack[m] = g_for_stack[m + 1]; g_for_top--; return 0; }
		}
	}

return -1;
}
