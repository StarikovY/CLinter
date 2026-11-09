#include <string.h>
#include "runtime.h"
#include "parse.h"
int read_filename_after(Lexer *lx, char *out, size_t cap) {
    size_t pos=lx->i, len;
    const char *raw=lx->s;
    while(raw[pos]==' '||raw[pos]=='\t')pos++;
    if(raw[pos] && raw[pos]!='"') {
        size_t end=strlen(raw);
        while(end>pos && (raw[end-1]==' '||raw[end-1]=='\t'||raw[end-1]=='\r'))end--;
        len=end-pos;
        if(!len||len>=cap)return 0;
        memcpy(out,raw+pos,len);out[len]=0;
        return 1;
    }
    lx_next(lx);
    if(lx->cur.type!=T_STRING)return 0;
    strncpy(out,lx->cur.text,cap-1);out[cap-1]=0;
    return 1;
}
