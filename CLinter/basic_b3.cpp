#include "basic_b3.h"
#include "parse.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
/* MSX default type declarations: 0=single, 1=integer, 2=double, 3=string. */
static unsigned char defaults[26];
static int handler=0, failing=0, active=0;
void b3_reset(void) { memset(defaults,0,sizeof(defaults));handler=failing=active=0; }
int b3_default_type(const char *name) {
    size_t n=strlen(name);
    if(n && name[n-1]=='$')return 3;
    if(n && name[n-1]=='%')return 1;
    if(n && name[n-1]=='#')return 2;
    if(n && name[n-1]=='!')return 0;
    if(n && isalpha((unsigned char)name[0]))return defaults[toupper((unsigned char)name[0])-'A'];
    return 0;
}
int b3_error_target(void){return handler;}
int b3_handle_error(int line){
    if(!handler || active)return 0;
    failing=line;active=1;return handler;
}
int b3_resume_target(int next){
    int target=failing;
    if(!active)return -1;
    active=0;failing=0;
    return next ? next_line_number_after(target) : target;
}
int b3_statement(Lexer *lx,int duringRun,int currentLine,int *jump){
    (void)currentLine;
    if(lx->cur.type==T_ONKW){
        lx_next(lx);
        if(lx->cur.type!=T_IDENT || _stricmp(lx->cur.text,"ERROR"))return -2;
        lx_next(lx);
        if(lx->cur.type!=T_GOTO){puts("ERROR: ON ERROR requires GOTO");return -1;}
        lx_next(lx);
        if(lx->cur.type!=T_NUMBER || lx->cur.number<0 || (int)lx->cur.number!=lx->cur.number){puts("ERROR: ON ERROR line");return -1;}
        handler=(int)lx->cur.number;active=0;failing=0;lx_next(lx);
        if(lx->cur.type!=T_END){puts("ERROR: ON ERROR syntax");return -1;}
        return 0;
    }
    if(lx->cur.type!=T_IDENT)return -2;
    if(!_stricmp(lx->cur.text,"RESUME")){
        int target;
        if(!duringRun){puts("ERROR: RESUME outside RUN");return -1;}
        lx_next(lx);
        if(lx->cur.type==T_IDENT && !_stricmp(lx->cur.text,"NEXT")){target=b3_resume_target(1);lx_next(lx);}
        else if(lx->cur.type==T_NUMBER){target=(int)lx->cur.number;if(!active){puts("ERROR: RESUME without error");return -1;}active=0;failing=0;lx_next(lx);}
        else target=b3_resume_target(0);
        if(target<0 || lx->cur.type!=T_END){puts("ERROR: RESUME syntax/state");return -1;}
        *jump=target;return 1;
    }
    int typ=-1;
    if(!_stricmp(lx->cur.text,"DEFINT"))typ=1;
    else if(!_stricmp(lx->cur.text,"DEFSNG"))typ=0;
    else if(!_stricmp(lx->cur.text,"DEFDBL"))typ=2;
    else if(!_stricmp(lx->cur.text,"DEFSTR"))typ=3;
    if(typ<0)return -2;
    lx_next(lx);
    do {
        if(lx->cur.type!=T_IDENT || !isalpha((unsigned char)lx->cur.text[0]) || lx->cur.text[1]){
            puts("ERROR: DEF type letter expected");return -1;
        }
        int first=toupper((unsigned char)lx->cur.text[0])-'A',last=first;
        lx_next(lx);
        if(lx->cur.type==T_MINUS){
            lx_next(lx);
            if(lx->cur.type!=T_IDENT || !isalpha((unsigned char)lx->cur.text[0]) || lx->cur.text[1]){
                puts("ERROR: DEF type range");return -1;
            }
            last=toupper((unsigned char)lx->cur.text[0])-'A';lx_next(lx);
        }
        if(last<first){puts("ERROR: DEF type reversed range");return -1;}
        for(int i=first;i<=last;i++)defaults[i]=(unsigned char)typ;
        if(lx->cur.type!=T_COMMA)break;
        lx_next(lx);
    }while(1);
    if(lx->cur.type!=T_END){puts("ERROR: DEF type syntax");return -1;}
    return 0;
}
