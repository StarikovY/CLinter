/* CLinter sequential file operations. Compiled as C by CMake. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "runtime.h"
#include "parse.h"
#include "basic_fileio.h"

static int channel(Lexer *lx, int *out) {
    if (lx->cur.type == T_HASH) lx_next(lx);
    if (lx->cur.type != T_NUMBER || lx->cur.number != (int)lx->cur.number) return 0;
    *out = (int)lx->cur.number;
    if (*out < 0 || *out >= MAX_FILES) return 0;
    lx_next(lx);
    return 1;
}
static int read_field(FILE *f, char *buf, size_t size, int line_mode) {
    int c; size_t i=0;
    if (line_mode) {
        if (!fgets(buf, (int)size, f)) return 0;
        buf[strcspn(buf, "\r\n")] = 0;
        return 1;
    }
    do { c=fgetc(f); if (c==EOF) return 0; }
    while (c==' ' || c=='\t' || c=='\r' || c=='\n' || c==',');
    if (c=='"') {
        while ((c=fgetc(f))!=EOF && c!='"') if(i+1<size) buf[i++]=(char)c;
        c=fgetc(f); if (c!=EOF && c!=',' && c!='\r' && c!='\n') ungetc(c,f);
    } else {
        do { if(i+1<size) buf[i++]=(char)c; c=fgetc(f); }
        while (c!=EOF && c!=',' && c!='\r' && c!='\n');
        while (i && isspace((unsigned char)buf[i-1])) --i;
    }
    buf[i]=0;
    return 1;
}
int basic_fileio_statement(Lexer *lx, int *outJump) {
    if (lx->cur.type == T_FSEEK) {
        int n; double offset;
        lx_next(lx);
        if (!channel(lx, &n) || lx->cur.type != T_COMMA) { puts("ERROR: FSEEK needs #channel,offset"); return -1; }
        lx_next(lx);
        offset = parse_rel(lx);
        if (offset < 0 || offset > 2147483647.0 || offset != (long)offset) { puts("ERROR: invalid FSEEK offset"); return -1; }
        if (!g_files[n].used || !g_files[n].fp || fseek(g_files[n].fp, (long)offset, SEEK_SET) != 0) { puts("ERROR: FSEEK failed"); return -1; }
        return 0;
    }
    if (lx->cur.type == T_OPEN) {
        char filename[260]; int mode, n; FILE *fp;
        lx_next(lx);
        if (lx->cur.type != T_STRING && lx->cur.type != T_IDENT) { puts("ERROR: OPEN needs filename"); return -1; }
        strncpy(filename,lx->cur.text,sizeof(filename)-1); filename[sizeof(filename)-1]=0;
        lx_next(lx);
        /* FOR is also the loop keyword; it must be consumed after filename. */
        if (lx->cur.type != T_FOR) { puts("ERROR: OPEN needs FOR"); return -1; }
        lx_next(lx);
        if (lx->cur.type == T_INPUT) mode=0;
        else if (lx->cur.type == T_OUTPUTKW) mode=1;
        else if (lx->cur.type == T_APPEND) mode=2;
        else { puts("ERROR: OPEN mode"); return -1; }
        lx_next(lx);
        if (lx->cur.type != T_AS) { puts("ERROR: OPEN needs AS"); return -1; }
        lx_next(lx);
        if (!channel(lx,&n)) { puts("ERROR: OPEN needs valid channel"); return -1; }
        int else_line = -1;
        if (lx->cur.type == T_ELSE) {
            lx_next(lx);
            if (lx->cur.type != T_NUMBER || lx->cur.number < 0 || lx->cur.number != (int)lx->cur.number) {
                puts("ERROR: OPEN ELSE needs line number"); return -1;
            }
            else_line = (int)lx->cur.number;
            lx_next(lx);
        }
        if (g_files[n].used) {
            if (else_line >= 0) { *outJump = else_line; return 1; }
            puts("ERROR: handle already open"); return -1;
        }
        fp=fopen(filename, mode==0?"rb":mode==1?"wb":"ab");
        if (!fp) {
            if (else_line >= 0) { *outJump = else_line; return 1; }
            puts("ERROR: cannot open file"); return -1;
        }
        g_files[n].fp=fp; g_files[n].used=1;
        return 0;
    }
    if (lx->cur.type == T_CLOSE) {
        int n;
        lx_next(lx);
        if (lx->cur.type == T_END) { files_clear(); return 0; }
        if (!channel(lx,&n)) { puts("ERROR: CLOSE needs valid channel"); return -1; }
        if (g_files[n].used) { fclose(g_files[n].fp); g_files[n].fp=NULL; g_files[n].used=0; }
        return 0;
    }
    if (lx->cur.type == T_INPUT || lx->cur.type == T_LINE) {
        int line_mode=(lx->cur.type==T_LINE), n;
        FILE *fp;
        lx_next(lx);
        if (line_mode) {
            if (lx->cur.type != T_INPUT) { puts("ERROR: LINE needs INPUT"); return -1; }
            lx_next(lx);
        }
        if (lx->cur.type != T_HASH) { puts("ERROR: file INPUT needs #"); return -1; }
        if (!channel(lx,&n) || !g_files[n].used || !g_files[n].fp) { puts("ERROR: invalid INPUT channel"); return -1; }
        fp=g_files[n].fp;
        if (lx->cur.type==T_COMMA || lx->cur.type==T_SEMI) lx_next(lx);
        for (;;) {
            char name[128], buf[1024], *end; Variable *v;
            int str;
            if (lx->cur.type!=T_IDENT) { puts("ERROR: INPUT # needs variable"); return -1; }
            strncpy(name,lx->cur.text,sizeof(name)-1); name[sizeof(name)-1]=0;
            str=is_string_var_name(name);
            if (line_mode && !str) { puts("ERROR: LINE INPUT # needs string variable"); return -1; }
            lx_next(lx);
            if (!read_field(fp,buf,sizeof(buf),line_mode)) { puts("ERROR: INPUT # end of file"); return -1; }
            v=ensure_var(name,str);
            if (str) {
                char *copy=strdup_c(buf);
                if (!copy) { puts("ERROR: out of memory"); return -1; }
                free(v->str); v->str=copy;
            } else {
                double value=strtod(buf,&end);
                while (*end==' ' || *end=='\t') ++end;
                if (end==buf || *end) { puts("ERROR: INPUT # expects number"); return -1; }
                v->num=value;
            }
            if (line_mode || lx->cur.type!=T_COMMA) break;
            lx_next(lx);
        }
        return 0;
    }
    return -1;
}
