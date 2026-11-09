#include "basic_b1b2.h"
#include "parse.h"
#include "strcon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {char name[32],arg[32],body[256];} Fn;
static Fn functions[64];
static int fn_count=0, fn_depth=0;
void b1b2_reset(void){fn_count=0;}
static int keyword(const char *a,const char *b){return !_stricmp(a,b);}
static int fail(const char *s){printf("ERROR: %s\n",s);return -1;}
static int find_matching(int line,int forward){
 int idx=find_prog_index_by_line(line),depth=0,step=forward?1:-1;
 if(idx<0)return -1;
 for(idx+=step;idx>=0&&idx<g_prog_count;idx+=step){
  Lexer l;lx_init(&l,g_prog[idx].text);lx_next(&l);
  if(l.cur.type!=T_IDENT)continue;
  if(keyword(l.cur.text,forward?"WHILE":"WEND"))depth++;
  if(keyword(l.cur.text,forward?"WEND":"WHILE")){
   if(depth==0)return g_prog[idx].number;
   depth--;
  }
 }
 return -1;
}
int b1b2_function(const char *name,Lexer *lx,double *value){
 int i;
 for(i=0;i<fn_count;i++)if(keyword(name,functions[i].name))break;
 if(i==fn_count)return 0;
 if(fn_depth>=16){puts("ERROR: DEF FN recursion limit");*value=0;return 1;}
 lx_next(lx);
 if(lx->cur.type!=T_LPAREN){puts("ERROR: DEF FN argument");*value=0;return 1;}
 lx_next(lx);
 double arg=parse_rel(lx);
 if(lx->cur.type!=T_RPAREN){puts("ERROR: DEF FN closing parenthesis");*value=0;return 1;}
 lx_next(lx);
 Variable *v=find_var(functions[i].arg);
 int existed=(v!=NULL);double previous=v?v->num:0;
 v=ensure_var(functions[i].arg,0);
 if(!v){*value=0;return 1;}
 v->num=arg;
 Lexer expr;lx_init(&expr,functions[i].body);lx_next(&expr);
 fn_depth++;*value=parse_rel(&expr);fn_depth--;
 if(existed)v->num=previous;
 else { /* temporary function parameter is cleared after evaluation */
  int j;for(j=0;j<g_var_count;j++)if(&g_vars[j]==v){
   for(;j+1<g_var_count;j++)g_vars[j]=g_vars[j+1];g_var_count--;break;
  }
 }
 return 1;
}
int b1b2_statement(Lexer *lx,int duringRun,int currentLine,int *jump){
 char op[32];
 if(lx->cur.type!=T_IDENT)return -2;
 strncpy(op,lx->cur.text,sizeof(op)-1);op[sizeof(op)-1]=0;
 if(keyword(op,"WHILE")){
  lx_next(lx);double v=parse_rel(lx);
  if(lx->cur.type!=T_END)return fail("WHILE expression");
  if(v!=0)return 0;
  if(!duringRun)return fail("WHILE requires RUN");
  int end=find_matching(currentLine,1);
  if(end<0)return fail("WHILE without WEND");
  *jump=next_line_number_after(end);
  if(*jump<0)return 9;
  return 1;
 }
 if(keyword(op,"WEND")){
  lx_next(lx);if(lx->cur.type!=T_END)return fail("WEND syntax");
  if(!duringRun)return fail("WEND requires RUN");
  int start=find_matching(currentLine,0);
  if(start<0)return fail("WEND without WHILE");
  *jump=start;return 1;
 }
 if(keyword(op,"DEF")){
  lx_next(lx);
  if(lx->cur.type!=T_IDENT || !(toupper((unsigned char)lx->cur.text[0])=='F'&&toupper((unsigned char)lx->cur.text[1])=='N'))return fail("DEF FN name");
  char name[32],arg[32];strncpy(name,lx->cur.text,31);name[31]=0;
  lx_next(lx);if(lx->cur.type!=T_LPAREN)return fail("DEF FN (");
  lx_next(lx);if(lx->cur.type!=T_IDENT)return fail("DEF FN parameter");
  strncpy(arg,lx->cur.text,31);arg[31]=0;
  lx_next(lx);if(lx->cur.type!=T_RPAREN)return fail("DEF FN )");
  lx_next(lx);if(lx->cur.type!=T_EQ)return fail("DEF FN =");
  const char *body=lx->s+lx->i;
  while(*body==' '||*body=='\t')body++;
  if(!*body||strlen(body)>=sizeof(functions[0].body))return fail("DEF FN expression");
  int i;for(i=0;i<fn_count;i++)if(keyword(name,functions[i].name))break;
  if(i==fn_count){if(fn_count>=64)return fail("too many DEF FN");fn_count++;}
  strcpy(functions[i].name,name);strcpy(functions[i].arg,arg);strcpy(functions[i].body,body);
  return 0;
 }
 if(keyword(op,"CLEAR")){
  lx_next(lx);if(lx->cur.type!=T_END)return fail("CLEAR syntax");
  vars_clear();arrays_clear();sarrays_clear();g_for_top=0;return 0;
 }
 if(keyword(op,"ERASE")){
  lx_next(lx);int count=0;
  for(;;){
   if(lx->cur.type!=T_IDENT)return fail("ERASE array name");
   char name[32];strncpy(name,lx->cur.text,31);name[31]=0;
   int i,found=0;
   for(i=0;i<g_array_count;i++)if(keyword(name,g_arrays[i].name)){
    free(g_arrays[i].data);for(int j=i;j+1<g_array_count;j++)g_arrays[j]=g_arrays[j+1];g_array_count--;found=1;break;
   }
   for(i=0;i<g_sarray_count;i++)if(keyword(name,g_sarrays[i].name)){
    int n=1;for(int k=0;k<g_sarrays[i].ndims;k++)n*=g_sarrays[i].dims[k];
    for(int k=0;k<n;k++)free(g_sarrays[i].data[k]);free(g_sarrays[i].data);
    for(int j=i;j+1<g_sarray_count;j++)g_sarrays[j]=g_sarrays[j+1];g_sarray_count--;found=1;break;
   }
   if(!found)return fail("ERASE unknown array");count++;lx_next(lx);
   if(lx->cur.type!=T_COMMA)break;
   lx_next(lx);
  }
  return lx->cur.type==T_END&&count?0:fail("ERASE syntax");
 }
 if(keyword(op,"SWAP")){
  lx_next(lx);if(lx->cur.type!=T_IDENT)return fail("SWAP first variable");
  char a[32],b[32];strncpy(a,lx->cur.text,31);a[31]=0;
  lx_next(lx);if(lx->cur.type!=T_COMMA)return fail("SWAP comma");
  lx_next(lx);if(lx->cur.type!=T_IDENT)return fail("SWAP second variable");
  strncpy(b,lx->cur.text,31);b[31]=0;lx_next(lx);
  if(lx->cur.type!=T_END)return fail("SWAP syntax");
  int sa=is_string_var_name(a),sb=is_string_var_name(b);
  if(sa!=sb)return fail("SWAP type mismatch");
  Variable *va=ensure_var(a,sa),*vb=ensure_var(b,sb);
  if(!va||!vb)return fail("SWAP variable limit");
  if(sa){char *tmp=va->str;va->str=vb->str;vb->str=tmp;}
  else {double tmp=va->num;va->num=vb->num;vb->num=tmp;}
  return 0;
 }
 if(keyword(op,"MID$")){
  lx_next(lx);if(lx->cur.type!=T_LPAREN)return fail("MID$ (");
  lx_next(lx);if(lx->cur.type!=T_IDENT||!is_string_var_name(lx->cur.text))return fail("MID$ string variable");
  char name[32];strncpy(name,lx->cur.text,31);name[31]=0;
  lx_next(lx);if(lx->cur.type!=T_COMMA)return fail("MID$ comma");
  lx_next(lx);int start=(int)parse_rel(lx),length=-1;
  if(lx->cur.type==T_COMMA){lx_next(lx);length=(int)parse_rel(lx);}
  if(lx->cur.type!=T_RPAREN)return fail("MID$ )");
  lx_next(lx);if(lx->cur.type!=T_EQ)return fail("MID$ =");
  lx_next(lx);char buf[4096];SBuf sb;sbuf_init(&sb,buf,sizeof(buf));parse_string_expr_to_buf(lx,&sb);
  if(lx->cur.type!=T_END||start<1||length< -1)return fail("MID$ syntax");
  Variable *v=ensure_var(name,1);if(!v)return fail("MID$ variable limit");
  const char *old=v->str?v->str:"";size_t n=strlen(old),m=strlen(buf);
  if((size_t)start>n)return 0;
  size_t pos=(size_t)start-1,copy=n-pos;
  if(length>=0&&(size_t)length<copy)copy=(size_t)length;
  if(m<copy)copy=m;
  char *out=(char*)malloc(n+1);if(!out)return fail("MID$ allocation");
  memcpy(out,old,n+1);memcpy(out+pos,buf,copy);free(v->str);v->str=out;
  return 0;
 }
 return -2;
}
