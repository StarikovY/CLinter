
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "runtime.h"
#include "parse.h"
#include "wxecut.h"

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

#ifdef OLD
int exec_statement(const char*src,int duringRun,int currentLine,int*outJump){
    Lexer lx; lx_init(&lx,src); lx_next(&lx);

    if(lx.cur.type==T_REM) return 0;
    if (lx.cur.type == T_NEW) 
    { 
        prog_clear(); 
        vars_clear(); 
        arrays_clear();
        sarrays_clear();
        g_for_top = 0; 
        g_gosub_top = 0; 
        files_clear(); 
        printf("NEW PROGRAM\n"); 
        return 0; 
    }


    if(lx.cur.type==T_LIST){
        Token a; lx_next(&lx); a=lx.cur;
        if(a.type==T_END){ cmd_list(0,0,0,0); return 0; }
        if(a.type==T_NUMBER){
            int start=(int)a.number; Token b; lx_next(&lx); b=lx.cur;
            if(b.type==T_NUMBER){ int end=(int)b.number; cmd_list(1,start,1,end); return 0; }
            cmd_list(1,start,0,0); return 0;
        }
        printf("ERROR: LIST syntax\n"); return -1;
    }

    /* program SAVE/LOAD */
    if(lx.cur.type==T_SAVE || lx.cur.type==T_LOAD){
        int isLoad=(lx.cur.type==T_LOAD);
        char fname[260]; fname[0]=0;
        if(!read_filename_after(&lx,fname,sizeof(fname))){ printf("ERROR: filename\n"); return -1; }
        if(isLoad){
            FILE*f=fopen(fname,"rb"); char linebuf[1024]; int ln;
            if(!f){ printf("ERROR: cannot open file\n"); return -1; }
            prog_clear();
            while(fscanf(f,"%d",&ln)==1){
                if(fgets(linebuf,sizeof(linebuf),f)){ char *p=linebuf; if(*p==' ') p++; trim(p); prog_set_line(ln,p); }
            }
            fclose(f); printf("Loaded %s (%d lines)\n", fname, g_prog_count); return 0;
        } else {
            FILE*f=fopen(fname,"wb"); int i; if(!f){ printf("ERROR: cannot write file\n"); return -1; }
            sort_program(); for(i=0;i<g_prog_count;i++) fprintf(f,"%d %s\n", g_prog[i].line, g_prog[i].text);
            fclose(f); printf("Saved to %s\n", fname); return 0;
        }
    }

    if (lx.cur.type == T_DIM) 
    {
        for (;;) {
            char aname[32]; int dims[MAX_DIMS]; int nd = 0;
            lx_next(&lx);
            if (lx.cur.type != T_IDENT) { printf("ERROR: DIM needs name\n"); return -1; }
            // if (is_string_var_name(lx.cur.text)) { printf("ERROR: STRING ARRAYS NOT SUPPORTED\n"); return -1; }

            strncpy(aname, lx.cur.text, sizeof(aname) - 1); aname[sizeof(aname) - 1] = 0;
            lx_next(&lx);
            if (lx.cur.type != T_LPAREN) { printf("ERROR: DIM needs '('\n"); return -1; }
            lx_next(&lx);
            while (lx.cur.type != T_RPAREN && lx.cur.type != T_END) {
                if (nd >= MAX_DIMS) { printf("ERROR: > %d DIMENSIONS\n", MAX_DIMS); return -1; }
                dims[nd++] = (int)parse_rel(&lx);  /* size per dim (>=1), zero-based indexing */
                if (lx.cur.type == T_COMMA) { lx_next(&lx); continue; }
                else break;
            }
            if (lx.cur.type != T_RPAREN) { printf("ERROR: DIM missing ')'\n"); return -1; }
            lx_next(&lx);
            
            if (is_string_var_name(aname)) 
            {
                if (!sarray_dim(aname, nd, dims)) return -1;              
            }
            else 
            {
                if (!array_dim(aname, nd, dims)) return -1;                
            }

            if (lx.cur.type == T_COMMA) { /* multiple arrays in one DIM */ continue; }
            break;
        }
        return 0;
    }


    /* SAVEVARS / LOADVARS */
    if(lx.cur.type==T_SAVEVARS || lx.cur.type==T_LOADVARS){
        int isLoad=(lx.cur.type==T_LOADVARS); char fname[260]; fname[0]=0;
        if(!read_filename_after(&lx,fname,sizeof(fname))){ printf("ERROR: filename\n"); return -1; }
        if(isLoad){
            FILE*f=fopen(fname,"rb"); char line[512];
            if(!f){ printf("ERROR: cannot open file\n"); return -1; }
            vars_clear();
            while(fgets(line,sizeof(line),f)){
                char *name=strtok(line,"\t\r\n");
                char *type=strtok(NULL,"\t\r\n");
                char *val =strtok(NULL,"\r\n");
                if(name&&type&&val){
                    Variable *v = ensure_var(name, (type[0]=='S'));
                    if(v->type==VT_STR){ if(v->str) free(v->str); v->str=strdup_c(val); }
                    else v->num=atof(val);
                }
            }
            fclose(f); printf("Variables loaded from %s (%d)\n", fname, g_var_count); return 0;
        } else {
            FILE*f=fopen(fname,"wb"); int i; if(!f){ printf("ERROR: cannot write file\n"); return -1; }
            for(i=0;i<g_var_count;i++){
                if(g_vars[i].type==VT_STR) fprintf(f,"%s\tS\t%s\n", g_vars[i].name, g_vars[i].str?g_vars[i].str:"");
                else fprintf(f,"%s\tN\t%.15g\n", g_vars[i].name, g_vars[i].num);
            }
            fclose(f); printf("Variables saved to %s\n", fname); return 0;
        }
    }

    /* FILE I/O */
    if(lx.cur.type==T_OPEN){
        char fname[260]; fname[0]=0; int mode=0; int handle=-1; FILE*fp;
        if(!read_filename_after(&lx,fname,sizeof(fname))){ printf("ERROR: OPEN needs filename\n"); return -1; }
        if(lx.cur.type!=T_FOR){ printf("ERROR: OPEN needs FOR\n"); return -1; }
        lx_next(&lx);
        if(lx.cur.type==T_INPUT) mode=0;
        else if(lx.cur.type==T_OUTPUTKW) mode=1;
        else if(lx.cur.type==T_APPEND) mode=2;
        else { printf("ERROR: OPEN mode\n"); return -1; }
        lx_next(&lx);
        if(lx.cur.type!=T_AS){ printf("ERROR: OPEN needs AS\n"); return -1; }
        lx_next(&lx);
        if(lx.cur.type==T_HASH) lx_next(&lx);
        if(lx.cur.type!=T_NUMBER){ printf("ERROR: OPEN needs handle number\n"); return -1; }
        handle=(int)lx.cur.number;
        if(handle<0 || handle>=MAX_FILES){ printf("ERROR: handle out of range\n"); return -1; }
        if(g_files[handle].used){ printf("ERROR: handle already open\n"); return -1; }
        if(mode==0) fp=fopen(fname,"rb"); else if(mode==1) fp=fopen(fname,"wb"); else fp=fopen(fname,"ab");
        if(!fp){ printf("ERROR: cannot open file\n"); return -1; }
        g_files[handle].used=1; g_files[handle].fp=fp; return 0;
    }
    if(lx.cur.type==T_CLOSE){
        lx_next(&lx);
        if(lx.cur.type==T_HASH || lx.cur.type==T_NUMBER){
            int handle; if(lx.cur.type==T_HASH) lx_next(&lx);
            if(lx.cur.type!=T_NUMBER){ printf("ERROR: CLOSE needs number\n"); return -1; }
            handle=(int)lx.cur.number;
            if(handle>=0 && handle<MAX_FILES && g_files[handle].used){ fclose(g_files[handle].fp); g_files[handle].used=0; g_files[handle].fp=NULL; }
        } else { files_clear(); }
        return 0;
    }

    /* RUN */
    if(lx.cur.type==T_RUN){ return 2; }

    /* END/STOP */
    if(lx.cur.type==T_ENDKW || lx.cur.type==T_STOP){ if(duringRun) return 9; printf("OK\n"); return 0; }

    /* PRINT */
    if(lx.cur.type==T_PRINT){
        FILE*out = stdout; lx_next(&lx);
        if(lx.cur.type==T_HASH){ lx_next(&lx);
            if(lx.cur.type!=T_NUMBER){ printf("ERROR: PRINT # needs handle\n"); return -1; }
            out = file_from_handle((int)lx.cur.number); if(!out){ printf("ERROR: bad handle\n"); return -1; }
            lx_next(&lx); if(lx.cur.type==T_COMMA || lx.cur.type==T_SEMI) lx_next(&lx);
        }
        { int first=1; while(lx.cur.type!=T_END){
            if(!first){ if(lx.cur.type==T_COMMA || lx.cur.type==T_SEMI){ lx_next(&lx);} else fprintf(out," "); }
            first=0;
            if (lx.cur.type == T_STRING)
            {
                fprintf(out, "%s", lx.cur.text); lx_next(&lx);
            }
            else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) 
            {
                char nbuf[32]; strncpy(nbuf, lx.cur.text, sizeof(nbuf) - 1); nbuf[sizeof(nbuf) - 1] = 0; lx_next(&lx);
                if (lx.cur.type == T_LPAREN) 
                {
                    int subs[MAX_DIMS], nsubs = 0; SArray* sa = sarray_find(nbuf);
                    lx_next(&lx);
                    while (lx.cur.type != T_RPAREN && lx.cur.type != T_END) 
                    {
                        if (nsubs >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); break; }
                        subs[nsubs++] = (int)parse_rel(&lx);
                        if (lx.cur.type == T_COMMA) { lx_next(&lx); continue; }
                        else break;                      
                    }
                    
                    if (lx.cur.type == T_RPAREN) lx_next(&lx);
                    fprintf(out, "%s", sa ? sarray_get(sa, subs, nsubs) : "");
                   
                }
                else 
                {
                    Variable * v = find_var(nbuf); fprintf(out, "%s", (v && v->type == VT_STR && v->str) ? v->str : "");                    
                }
            }
            else { double val=parse_rel(&lx); fprintf(out,"%.15g",val); }

        } fprintf(out,"\n"); }
        return 0;
    }

    /* INPUT / LINE INPUT */
    if(lx.cur.type==T_INPUT){
        lx_next(&lx);
        if(lx.cur.type==T_HASH){
            int handle; FILE*fp; char vname[32]; int isStr;
            lx_next(&lx);
            if(lx.cur.type!=T_NUMBER){ printf("ERROR: INPUT # needs handle\n"); return -1; }
            handle=(int)lx.cur.number; lx_next(&lx);
            fp = file_from_handle(handle); if(!fp){ printf("ERROR: bad handle\n"); return -1; }
            if(lx.cur.type==T_COMMA) lx_next(&lx);
            if(lx.cur.type!=T_IDENT){ printf("ERROR: INPUT # needs variable\n"); return -1; }
            strncpy(vname,lx.cur.text,sizeof(vname)-1); vname[sizeof(vname)-1]=0; isStr = is_string_var_name(vname);
            if(isStr){ char buf[4096]; if(!read_token_from_file(fp,buf,sizeof(buf))) strcpy(buf,""); { Variable *v=ensure_var(vname,1); if(v->str) free(v->str); v->str=strdup_c(buf); } }
            else { char tok[256]; if(!read_token_from_file(fp,tok,sizeof(tok))) strcpy(tok,"0"); ensure_var(vname,0)->num = atof(tok); }
            return 0;
        } else if(lx.cur.type==T_LINE){
            lx_next(&lx);
            if(lx.cur.type!=T_HASH){ printf("ERROR: LINE INPUT needs #\n"); return -1; }
            lx_next(&lx); if(lx.cur.type!=T_NUMBER){ printf("ERROR: LINE INPUT # needs handle\n"); return -1; }
            { int handle=(int)lx.cur.number; FILE*fp; char vname[32];
              lx_next(&lx); fp=file_from_handle(handle); if(!fp){ printf("ERROR: bad handle\n"); return -1; }
              if(lx.cur.type==T_COMMA) lx_next(&lx);
              if(lx.cur.type!=T_IDENT || !is_string_var_name(lx.cur.text)){ printf("ERROR: LINE INPUT needs string var\n"); return -1; }
              strncpy(vname,lx.cur.text,sizeof(vname)-1); vname[sizeof(vname)-1]=0;
              { char buf[8192]; if(!read_line_from_file(fp,buf,sizeof(buf))) buf[0]=0; { Variable*vs=ensure_var(vname,1); if(vs->str) free(vs->str); vs->str=strdup_c(buf); } }
            }
            return 0;
        } else {
            char vname[32]; int isStr;
            if(lx.cur.type!=T_IDENT){ printf("ERROR: INPUT needs var\n"); return -1; }
            strncpy(vname,lx.cur.text,sizeof(vname)-1); vname[sizeof(vname)-1]=0; isStr=is_string_var_name(vname);
            { Variable *v=ensure_var(vname,isStr); char buf[512];
              printf("%s? ", vname); if(!fgets(buf,sizeof(buf),stdin)) strcpy(buf,"0\n"); trim(buf);
              if(isStr){ if(v->str) free(v->str); v->str=strdup_c(buf); } else { v->num=atof(buf); } }
            return 0;
        }
    }

    /* IF <rel> THEN <line> */
    if(lx.cur.type==T_IF){
        int dest; double cond; lx_next(&lx); cond=parse_rel(&lx);
        if(lx.cur.type!=T_THEN){ printf("ERROR: THEN expected\n"); return -1; }
        lx_next(&lx); if(lx.cur.type!=T_NUMBER){ printf("ERROR: line number expected\n"); return -1; }
        dest=(int)lx.cur.number; if(cond!=0.0){ *outJump=dest; return 1; } return 0;
    }

    if(lx.cur.type==T_GOTO){ lx_next(&lx); if(lx.cur.type!=T_NUMBER){ printf("ERROR: GOTO needs line\n"); return -1; } *outJump=(int)lx.cur.number; return 1; }
    if(lx.cur.type==T_GOSUB){ int nextLine=next_line_number_after(currentLine); lx_next(&lx);
        if(lx.cur.type!=T_NUMBER){ printf("ERROR: GOSUB needs line\n"); return -1; }
        if(nextLine<0){ printf("ERROR: GOSUB at last line\n"); return -1; }
        if(g_gosub_top>=MAX_STACK){ printf("ERROR: GOSUB stack overflow\n"); return -1; }
        g_gosub_stack[g_gosub_top++]=nextLine; *outJump=(int)lx.cur.number; return 1; }
    if(lx.cur.type==T_RETURN){ if(g_gosub_top<=0){ printf("ERROR: RETURN without GOSUB\n"); return -1; } *outJump=g_gosub_stack[--g_gosub_top]; return 1; }

    if(lx.cur.type==T_FOR){
        char vname[32]; double start,toVal,step=1.0; int afterFor;
        lx_next(&lx); if(lx.cur.type!=T_IDENT){ printf("ERROR: FOR needs var\n"); return -1; }
        strncpy(vname,lx.cur.text,sizeof(vname)-1); vname[sizeof(vname)-1]=0;
        lx_next(&lx); if(lx.cur.type!=T_EQ){ printf("ERROR: FOR needs '='\n"); return -1; }
        lx_next(&lx); start=parse_rel(&lx);
        if(lx.cur.type!=T_TO){ printf("ERROR: FOR needs TO\n"); return -1; }
        lx_next(&lx); toVal=parse_rel(&lx);
        if(lx.cur.type==T_STEP){ lx_next(&lx); step=parse_rel(&lx); }
        ensure_var(vname,0)->num = start;
        afterFor = next_line_number_after(currentLine);
        if(afterFor<0){ printf("ERROR: FOR cannot be last line\n"); return -1; }
        if(g_for_top>=MAX_STACK){ printf("ERROR: FOR stack overflow\n"); return -1; }
        strncpy(g_for_stack[g_for_top].var, vname, sizeof(g_for_stack[g_for_top].var)-1);
        g_for_stack[g_for_top].var[sizeof(g_for_stack[g_for_top].var)-1]=0;
        g_for_stack[g_for_top].end = toVal; g_for_stack[g_for_top].step = step;
        g_for_stack[g_for_top].afterForLine = afterFor; g_for_stack[g_for_top].forLine = currentLine;
        g_for_top++; return 0;
    }
    if(lx.cur.type==T_NEXT){
        int idx=g_for_top-1; lx_next(&lx);
        if(lx.cur.type==T_IDENT){
            int k; for(k=g_for_top-1;k>=0;k--){ if(_stricmp(g_for_stack[k].var,lx.cur.text)==0){ idx=k; break; } }
            if(k<0){ printf("ERROR: NEXT for unknown FOR var\n"); return -1; }
        }
        if(idx<0){ printf("ERROR: NEXT without FOR\n"); return -1; }
        { ForFrame fr=g_for_stack[idx]; Variable*v=ensure_var(fr.var,0);
          double cur=v->num + fr.step; int cont=(fr.step>=0)?(cur<=fr.end):(cur>=fr.end);
          v->num=cur; if(cont){ *outJump=fr.afterForLine; return 1; }
          else { int m; for(m=idx;m<g_for_top-1;m++) g_for_stack[m]=g_for_stack[m+1]; g_for_top--; return 0; } }
    }

    if (lx.cur.type == T_LET) lx_next(&lx);

    if (lx.cur.type == T_IDENT) {
        int isStr; char name[32]; strncpy(name, lx.cur.text, sizeof(name) - 1); name[sizeof(name) - 1] = 0; isStr = is_string_var_name(name);
        lx_next(&lx);

        /* Array element? IDENT '(' ... ')' '=' expr */
        if (lx.cur.type == T_LPAREN) {
            int subs[MAX_DIMS], nsubs = 0;
            Array* a;

            if (isStr) 
            {
                // lx_next(&lx);
                /* RHS must be a string literal or a string variable/array element */
                if (lx.cur.type == T_STRING) {
                    SArray* sa = sarray_find(name);
                    if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
                    sarray_set(sa, subs, nsubs, lx.cur.text); lx_next(&lx);
                }
                else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
                    char srcname[32]; strncpy(srcname, lx.cur.text, sizeof(srcname) - 1); srcname[sizeof(srcname) - 1] = 0;
                    lx_next(&lx);
                    if (lx.cur.type == T_LPAREN) {
                        /* source is string array element */
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
                        /* source is scalar string variable */
                        Variable* sv = find_var(srcname);
                        const char* sval = (sv && sv->type == VT_STR && sv->str) ? sv->str : "";
                        {
                            SArray* sa = sarray_find(name); if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
                            sarray_set(sa, subs, nsubs, sval);
                        }
                    }
                }
                else {
                    printf("ERROR: string array assignment needs a string\n"); return -1;
                }
                return 0;
            }

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
            a = array_find(name);
            if (!a) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
            { double vnum = parse_rel(&lx); array_set(a, subs, nsubs, vnum); }
            return 0;
        }

        /* scalar assignment */
        if (lx.cur.type != T_EQ) { printf("ERROR: '=' expected\n"); return -1; } lx_next(&lx);
        if (isStr) {
            if (lx.cur.type == T_STRING) { Variable* v = ensure_var(name, 1); if (!v) return -1; if (v->str) free(v->str); v->str = strdup_c(lx.cur.text); lx_next(&lx); }
            else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
                Variable* src = find_var(lx.cur.text); Variable* dst = ensure_var(name, 1);
                if (dst->str) free(dst->str); dst->str = strdup_c((src && src->type == VT_STR && src->str) ? src->str : ""); lx_next(&lx);
            }
            else { printf("ERROR: string assignment needs a string\n"); return -1; }
        }
        else { double vnum = parse_rel(&lx); ensure_var(name, 0)->num = vnum; }
        return 0;
    }

    if(!duringRun){ printf("ERROR: syntax\n"); return -1; }
    printf("ERROR: syntax at line %d\n", currentLine); return -1;
}
#endif // OLD
int exec_statement(const char* src, int duringRun, int currentLine, int* outJump) {
    Lexer lx;
    lx_init(&lx, src);
    lx_next(&lx);

    /* Example: handle NEW command */
    if (lx.cur.type == T_NEW) {
        prog_clear();
        vars_clear();
        arrays_clear();
        sarrays_clear();
        g_for_top = 0;
        g_gosub_top = 0;
        files_clear();
        printf("NEW PROGRAM\n");
        return 0;
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
            FILE* f = fopen(fname, "wb"); int i; if (!f) { printf("ERROR: cannot write file\n"); return -1; }
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
            if (!f) { printf("ERROR: cannot open file\n"); return -1; }
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

    /* PRINT (console or file) — supports string arrays */
    if (lx.cur.type == T_PRINT) 
    {
        FILE* out = stdout; lx_next(&lx);
        if (lx.cur.type == T_HASH) {
            lx_next(&lx);
            if (lx.cur.type != T_NUMBER) { printf("ERROR: PRINT # needs handle\n"); return -1; }
            out = file_from_handle((int)lx.cur.number); if (!out) { printf("ERROR: bad handle\n"); return -1; }
            lx_next(&lx); if (lx.cur.type == T_COMMA || lx.cur.type == T_SEMI) lx_next(&lx);
        }
        {
            int first = 1; while (lx.cur.type != T_END) {
                if (!first) { if (lx.cur.type == T_COMMA || lx.cur.type == T_SEMI) { lx_next(&lx); } else fprintf(out, " "); }
                first = 0;
                if (lx.cur.type == T_STRING) { fprintf(out, "%s", lx.cur.text); lx_next(&lx); }
                else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
                    char nbuf[32]; strncpy(nbuf, lx.cur.text, sizeof(nbuf) - 1); nbuf[sizeof(nbuf) - 1] = 0; lx_next(&lx);
                    if (lx.cur.type == T_LPAREN) {
                        int subs[MAX_DIMS], nsubs = 0; SArray* sa = sarray_find(nbuf);
                        lx_next(&lx);
                        while (lx.cur.type != T_RPAREN && lx.cur.type != T_END) {
                            if (nsubs >= MAX_DIMS) { printf("ERROR: TOO MANY SUBSCRIPTS\n"); break; }
                            subs[nsubs++] = (int)parse_rel(&lx);
                            if (lx.cur.type == T_COMMA) { lx_next(&lx); continue; }
                            else break;
                        }
                        if (lx.cur.type == T_RPAREN) lx_next(&lx);
                        fprintf(out, "%s", sa ? sarray_get(sa, subs, nsubs) : "");
                    }
                    else {
                        Variable* v = find_var(nbuf); fprintf(out, "%s", (v && v->type == VT_STR && v->str) ? v->str : "");
                    }
                }
                else {
                    double val = parse_rel(&lx); fprintf(out, "%.15g", val);
                }
            } fprintf(out, "\n");
        }
        return 0;
    }

    /* INPUT / LINE INPUT (unchanged behavior) */
    if (lx.cur.type == T_INPUT) 
    {
        lx_next(&lx);
        if (lx.cur.type == T_HASH) {
            int handle; FILE* fp; char vname[32]; int isStr;
            lx_next(&lx);
            if (lx.cur.type != T_NUMBER) { printf("ERROR: INPUT # needs handle\n"); return -1; }
            handle = (int)lx.cur.number; lx_next(&lx);
            fp = file_from_handle(handle); if (!fp) { printf("ERROR: bad handle\n"); return -1; }
            if (lx.cur.type == T_COMMA) lx_next(&lx);
            if (lx.cur.type != T_IDENT) { printf("ERROR: INPUT # needs variable\n"); return -1; }
            strncpy(vname, lx.cur.text, sizeof(vname) - 1); vname[sizeof(vname) - 1] = 0; isStr = is_string_var_name(vname);
            if (isStr) { char buf[4096]; if (!read_token_from_file(fp, buf, sizeof(buf))) strcpy(buf, ""); { Variable* v = ensure_var(vname, 1); if (v->str) free(v->str); v->str = strdup_c(buf); } }
            else { char tok[256]; if (!read_token_from_file(fp, tok, sizeof(tok))) strcpy(tok, "0"); ensure_var(vname, 0)->num = atof(tok); }
            return 0;
        }
        else if (lx.cur.type == T_LINE) {
            lx_next(&lx);
            if (lx.cur.type != T_HASH) { printf("ERROR: LINE INPUT needs #\n"); return -1; }
            lx_next(&lx); if (lx.cur.type != T_NUMBER) { printf("ERROR: LINE INPUT # needs handle\n"); return -1; }
            {
                int handle = (int)lx.cur.number; FILE* fp; char vname[32];
                lx_next(&lx); fp = file_from_handle(handle); if (!fp) { printf("ERROR: bad handle\n"); return -1; }
                if (lx.cur.type == T_COMMA) lx_next(&lx);
                if (lx.cur.type != T_IDENT || !is_string_var_name(lx.cur.text)) { printf("ERROR: LINE INPUT needs string var\n"); return -1; }
                strncpy(vname, lx.cur.text, sizeof(vname) - 1); vname[sizeof(vname) - 1] = 0;
                { char buf[8192]; if (!read_line_from_file(fp, buf, sizeof(buf))) buf[0] = 0; { Variable* vs = ensure_var(vname, 1); if (vs->str) free(vs->str); vs->str = strdup_c(buf); } }
            }
            return 0;
        }
        else {
            char vname[32]; int isStr;
            if (lx.cur.type != T_IDENT) { printf("ERROR: INPUT needs var\n"); return -1; }
            strncpy(vname, lx.cur.text, sizeof(vname) - 1); vname[sizeof(vname) - 1] = 0; isStr = is_string_var_name(vname);
            {
                Variable* v = ensure_var(vname, isStr); char buf[512];
                printf("%s? ", vname); if (!fgets(buf, sizeof(buf), stdin)) strcpy(buf, "0\n"); trim(buf);
                if (isStr) { if (v->str) free(v->str); v->str = strdup_c(buf); }
                else { v->num = atof(buf); }
            }
            return 0;
        }
    }

    /* IF <rel> THEN <line> */
    if (lx.cur.type == T_IF) 
    {
        int dest; double cond; lx_next(&lx); cond = parse_rel(&lx);
        if (lx.cur.type != T_THEN) { printf("ERROR: THEN expected\n"); return -1; }
        lx_next(&lx); if (lx.cur.type != T_NUMBER) { printf("ERROR: line number expected\n"); return -1; }
        dest = (int)lx.cur.number; if (cond != 0.0) { *outJump = dest; return 1; } return 0;
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
                    char srcname[32]; strncpy(srcname, lx.cur.text, sizeof(srcname) - 1); srcname[sizeof(srcname) - 1] = 0;
                    lx_next(&lx);
                    if (lx.cur.type == T_LPAREN) {
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
                        Variable* sv = find_var(srcname);
                        const char* sval = (sv && sv->type == VT_STR && sv->str) ? sv->str : "";
                        {
                            SArray* sa = sarray_find(name); if (!sa) { printf("ERROR: UNDIM'D ARRAY %s\n", name); return -1; }
                            sarray_set(sa, subs, nsubs, sval);
                        }
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
            if (lx.cur.type == T_STRING) { Variable* v = ensure_var(name, 1); if (!v) return -1; if (v->str) free(v->str); v->str = strdup_c(lx.cur.text); lx_next(&lx); }
            else if (lx.cur.type == T_IDENT && is_string_var_name(lx.cur.text)) {
                Variable* src = find_var(lx.cur.text); Variable* dst = ensure_var(name, 1);
                if (dst->str) free(dst->str); dst->str = strdup_c((src && src->type == VT_STR && src->str) ? src->str : ""); lx_next(&lx);
            }
            else { printf("ERROR: string assignment needs a string at line %d\n", currentLine); return -1; }
        }
        else {
            double vnum = parse_rel(&lx); ensure_var(name, 0)->num = vnum;
        }
        return 0;
    }
    if (!duringRun) 
    { 
        printf("ERROR: syntax\n"); return -1; 
    }

    printf("ERROR: syntax at line %d\n", currentLine); return -1;
}
