/* CLinter RT-11-style MAT statements. Classic C, zero-based DIM extents. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "runtime.h"
#include "parse.h"

static int mat_error(const char *s) { printf("ERROR: MAT %s\n",s); return -1; }
static int mat_count(const Array *a) { int i,n=1; for(i=0;i<a->ndims;i++) n*=a->dims[i]; return n; }
static int mat_same(const Array *a,const Array *b) {
 int i; if(a->ndims!=b->ndims)return 0;
 for(i=0;i<a->ndims;i++)if(a->dims[i]!=b->dims[i])return 0;
 return 1;
}
static int mat_name(Lexer *lx, Array **out) {
 if(lx->cur.type!=T_IDENT)return mat_error("array name expected");
 *out=array_find(lx->cur.text);
 if(!*out)return mat_error("array not DIMensioned");
 if((*out)->ndims>2)return mat_error("only vectors and 2-D matrices supported");
 lx_next(lx);return 0;
}
static int mat_end(Lexer *lx) {return lx->cur.type==T_END?0:mat_error("unexpected trailing tokens");}
static int mat_inverse(const Array *b,double *result) {
 int n,i,j,k,p;double *w,largest,t,factor;
 if(b->ndims!=2||b->dims[0]!=b->dims[1])return mat_error("INV requires square matrix");
 n=b->dims[0];w=(double*)calloc((size_t)n*(size_t)(2*n),sizeof(double));
 if(!w)return mat_error("out of memory");
 for(i=0;i<n;i++)for(j=0;j<n;j++){w[i*(2*n)+j]=b->data[i*n+j];w[i*(2*n)+n+j]=(i==j)?1:0;}
 for(k=0;k<n;k++){
  p=k;largest=fabs(w[k*(2*n)+k]);
  for(i=k+1;i<n;i++)if(fabs(w[i*(2*n)+k])>largest){p=i;largest=fabs(w[i*(2*n)+k]);}
  if(largest<1e-12){free(w);return mat_error("singular matrix");}
  if(p!=k)for(j=0;j<2*n;j++){t=w[k*(2*n)+j];w[k*(2*n)+j]=w[p*(2*n)+j];w[p*(2*n)+j]=t;}
  t=w[k*(2*n)+k];for(j=0;j<2*n;j++)w[k*(2*n)+j]/=t;
  for(i=0;i<n;i++)if(i!=k){factor=w[i*(2*n)+k];for(j=0;j<2*n;j++)w[i*(2*n)+j]-=factor*w[k*(2*n)+j];}
 }
 for(i=0;i<n;i++)for(j=0;j<n;j++)result[i*n+j]=w[i*(2*n)+n+j];
 free(w);return 0;
}
/* Returns 0 on success, -1 on failure. The DATA callback shares READ's cursor. */
int matrix_statement(Lexer *lx,double (*read_number)(void)) {
 Array *a=NULL,*b=NULL,*c=NULL;char op[32];int i,j,k,n,rows,cols;double *tmp=NULL,scalar;
 lx_next(lx);
 if(lx->cur.type==T_PRINT||lx->cur.type==T_INPUT||lx->cur.type==T_READ){
  TokType command=lx->cur.type;lx_next(lx);
  for(;;){
   if(mat_name(lx,&a))return -1;
   n=mat_count(a);rows=a->dims[0];cols=a->ndims==2?a->dims[1]:1;
   if(command==T_READ){if(!read_number)return mat_error("DATA reader unavailable");for(i=0;i<n;i++)a->data[i]=read_number();}
   else if(command==T_INPUT){for(i=0;i<n;i++)if(scanf("%lf",&a->data[i])!=1)return mat_error("numeric INPUT expected");}
   else {for(i=0;i<rows;i++){for(j=0;j<cols;j++)printf("%s%.15g",j?" ":"",a->data[i*cols+j]);printf("\n");}}
   if(lx->cur.type!=T_COMMA)break;lx_next(lx);
  }
  return mat_end(lx);
 }
 if(mat_name(lx,&a))return -1;
 if(lx->cur.type!=T_EQ)return mat_error("expected =");lx_next(lx);
 if(lx->cur.type!=T_IDENT)return mat_error("source or operation expected");
 strncpy(op,lx->cur.text,sizeof(op)-1);op[sizeof(op)-1]=0;
 /* The lexer preserves identifier case: normalize operation spelling. */
 for(i=0;op[i];i++)if(op[i]>='a'&&op[i]<='z')op[i]=(char)(op[i]-'a'+'A');
 if(!strcmp(op,"ZER")||!strcmp(op,"CON")||!strcmp(op,"IDN")){
  lx_next(lx);if(lx->cur.type==T_LPAREN){lx_next(lx);if(lx->cur.type!=T_RPAREN)return mat_error("expected )");lx_next(lx);}
  if(mat_end(lx))return -1;
  if(!strcmp(op,"IDN")&&(a->ndims!=2||a->dims[0]!=a->dims[1]))return mat_error("IDN requires square matrix");
  n=mat_count(a);for(i=0;i<n;i++)a->data[i]=!strcmp(op,"CON")?1:0;
  if(!strcmp(op,"IDN"))for(i=0;i<a->dims[0];i++)a->data[i*a->dims[1]+i]=1;
  return 0;
 }
 if(!strcmp(op,"TRN")||!strcmp(op,"INV")){
  int transpose=!strcmp(op,"TRN");lx_next(lx);
  if(lx->cur.type!=T_LPAREN)return mat_error("expected (");lx_next(lx);
  if(mat_name(lx,&b))return -1;
  if(lx->cur.type!=T_RPAREN)return mat_error("expected )");lx_next(lx);
  if(mat_end(lx))return -1;
  if(a->ndims!=2||b->ndims!=2)return mat_error("TRN/INV require 2-D matrices");
  if(transpose){if(a->dims[0]!=b->dims[1]||a->dims[1]!=b->dims[0])return mat_error("transpose dimension mismatch");}
  else if(!mat_same(a,b))return mat_error("inverse dimension mismatch");
  n=mat_count(a);tmp=(double*)calloc((size_t)n,sizeof(double));if(!tmp)return mat_error("out of memory");
  if(transpose){for(i=0;i<b->dims[0];i++)for(j=0;j<b->dims[1];j++)tmp[j*a->dims[1]+i]=b->data[i*b->dims[1]+j];}
  else if(mat_inverse(b,tmp)){free(tmp);return -1;}
  memcpy(a->data,tmp,(size_t)n*sizeof(double));free(tmp);return 0;
 }
 b=array_find(op);if(!b)return mat_error("source array not DIMensioned");
 if(b->ndims>2)return mat_error("source must be vector or matrix");lx_next(lx);
 if(lx->cur.type==T_END){if(!mat_same(a,b))return mat_error("assignment dimension mismatch");if(a!=b)memcpy(a->data,b->data,(size_t)mat_count(a)*sizeof(double));return 0;}
 if(lx->cur.type!=T_PLUS&&lx->cur.type!=T_MINUS&&lx->cur.type!=T_STAR)return mat_error("expected +, - or *");
 {TokType operation=lx->cur.type;lx_next(lx);
  if(lx->cur.type==T_IDENT){if(mat_name(lx,&c))return -1;}
  else if(operation==T_STAR){scalar=parse_rel(lx);}
  else return mat_error("second matrix expected");
  if(mat_end(lx))return -1;
  n=mat_count(a);tmp=(double*)calloc((size_t)n,sizeof(double));if(!tmp)return mat_error("out of memory");
  if(operation==T_PLUS||operation==T_MINUS){
   if(!c||!mat_same(a,b)||!mat_same(a,c)){free(tmp);return mat_error("addition/subtraction dimension mismatch");}
   for(i=0;i<n;i++)tmp[i]=b->data[i]+(operation==T_PLUS?c->data[i]:-c->data[i]);
  }else if(!c){if(!mat_same(a,b)){free(tmp);return mat_error("scalar multiplication dimension mismatch");}for(i=0;i<n;i++)tmp[i]=b->data[i]*scalar;}
  else {
   if(a->ndims!=2||b->ndims!=2||c->ndims!=2||b->dims[1]!=c->dims[0]||a->dims[0]!=b->dims[0]||a->dims[1]!=c->dims[1]){free(tmp);return mat_error("multiplication dimension mismatch");}
   for(i=0;i<a->dims[0];i++)for(j=0;j<a->dims[1];j++)for(k=0;k<b->dims[1];k++)tmp[i*a->dims[1]+j]+=b->data[i*b->dims[1]+k]*c->data[k*c->dims[1]+j];
  }
 }
 memcpy(a->data,tmp,(size_t)n*sizeof(double));free(tmp);return 0;
}
