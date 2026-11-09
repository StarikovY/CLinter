/* G6: MSX BASIC 1-bit sprites, composited separately from background pages. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sprite.h"
#include "graphics.h"
#include "parse.h"
#define SPRITES 32
#define PATTERNS 256
static unsigned char patterns[PATTERNS][32];
static unsigned char defined[PATTERNS];
static struct { int x,y,color,pattern,visible; } slots[SPRITES];
static int side=8, scale=1;
static int collision_enabled=0, collision_latched=0;
static int collision_target=0, collision_pending=0, collision_previous=0;
static int fail(const char *msg) { printf("ERROR: SPRITE %s\n",msg); return -1; }
static int integer(Lexer *lx,int *n) {
    double v=parse_rel(lx);
    if(!isfinite(v)||v < -32768||v>32767||trunc(v)!=v)return 0;
    *n=(int)v;return 1;
}
/* Hex sprite patterns permit exact binary pixels, including zero bytes. */
static int hex_digit(int c) {
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
}
static int comma(Lexer *lx) { if(lx->cur.type!=T_COMMA)return 0;lx_next(lx);return 1; }
void sprite_reset(void) { memset(slots,0,sizeof(slots));memset(patterns,0,sizeof(patterns));memset(defined,0,sizeof(defined));side=8;scale=1;collision_enabled=0;collision_latched=0;collision_target=0;collision_pending=0;collision_previous=0; }
int sprite_size(int size) { if(size<0||size>3)return -1;side=(size&1)?16:8;scale=(size&2)?2:1;return 0; }
int sprite_visible_count(void) { int i,n=0;for(i=0;i<SPRITES;i++)n+=slots[i].visible;return n; }
int sprite_statement(Lexer *lx) {
    int id,x,y,color=15,pattern=-1,i,bytes;
    if(lx->cur.type!=T_IDENT)return fail("syntax");
    if(!_stricmp(lx->cur.text,"SPRITE$")) {
        /* SPRITE$(n) = "binary pattern"; 8 bytes for 8x8, 32 for 16x16. */
        lx_next(lx);
        if(lx->cur.type!=T_LPAREN)return fail("pattern index");lx_next(lx);
        if(!integer(lx,&id)||id<0||id>=PATTERNS||lx->cur.type!=T_RPAREN)return fail("pattern index");
        lx_next(lx);if(lx->cur.type!=T_EQ)return fail("pattern assignment");lx_next(lx);
        {
            const char *value;
            int hex_mode=0;
            if(lx->cur.type==T_IDENT && !_stricmp(lx->cur.text,"HEX$")) {
                hex_mode=1;lx_next(lx);
                if(lx->cur.type!=T_LPAREN)return fail("HEX$ opening parenthesis");
                lx_next(lx);
            }
            if(lx->cur.type==T_STRING)value=lx->cur.text;
            else if(!hex_mode && lx->cur.type==T_IDENT && is_string_var_name(lx->cur.text)) {
                Variable *v=find_var(lx->cur.text);
                value=(v && v->str)?v->str:"";
            } else return fail("pattern must be a string literal, variable or HEX$(string)");
            bytes=side==8?8:32;
            if(hex_mode) {
                if((int)strlen(value)!=bytes*2)return fail("hex pattern length");
                for(i=0;i<bytes;i++) {
                    int hi=hex_digit((unsigned char)value[2*i]);
                    int lo=hex_digit((unsigned char)value[2*i+1]);
                    if(hi<0||lo<0)return fail("hex pattern digit");
                    patterns[id][i]=(unsigned char)((hi<<4)|lo);
                }
            } else {
                if((int)strlen(value)!=bytes)return fail("pattern length");
                for(i=0;i<bytes;i++)patterns[id][i]=(unsigned char)value[i];
            }
            defined[id]=1;lx_next(lx);
            if(hex_mode) {
                if(lx->cur.type!=T_RPAREN)return fail("HEX$ closing parenthesis");
                lx_next(lx);
            }
        }
        if(lx->cur.type!=T_END)return fail("pattern syntax");
        return 0;
    }
    if(_stricmp(lx->cur.text,"PUT"))return fail("statement");
    lx_next(lx);
    if(lx->cur.type!=T_IDENT||_stricmp(lx->cur.text,"SPRITE"))return fail("expected SPRITE");
    lx_next(lx);
    if(!integer(lx,&id)||id<0||id>=SPRITES)return fail("slot range");
    if(lx->cur.type==T_END){slots[id].visible=0;graphics_refresh();return 0;}
    if(!comma(lx))return fail("position syntax");
    if(lx->cur.type!=T_LPAREN)return fail("position syntax");lx_next(lx);
    if(!integer(lx,&x)||!comma(lx)||!integer(lx,&y)||lx->cur.type!=T_RPAREN)return fail("coordinates");
    lx_next(lx);
    if(lx->cur.type==T_COMMA){
        lx_next(lx);if(!integer(lx,&color)||color<0||color>15)return fail("color range");
        if(lx->cur.type==T_COMMA){lx_next(lx);if(!integer(lx,&pattern)||pattern<0||pattern>=PATTERNS)return fail("pattern range");}
    }
    if(lx->cur.type!=T_END)return fail("trailing tokens");
    if(pattern<0)pattern=slots[id].pattern;
    if(!defined[pattern])return fail("undefined pattern");
    slots[id].x=x;slots[id].y=y;slots[id].color=color;slots[id].pattern=pattern;slots[id].visible=1;graphics_refresh();
    return 0;
}
/* Sprite collision uses opaque pattern pixels, not bounding boxes. */
static int opaque(int id,int xx,int yy) {
    int x,y,offset;
    if(!slots[id].visible || !slots[id].color || !defined[slots[id].pattern])return 0;
    x=xx-slots[id].x;y=yy-slots[id].y;
    if(x<0||y<0||x>=side*scale||y>=side*scale)return 0;
    x/=scale;y/=scale;
    offset=side==8?y:(x>=8?16+y:y);
    return (patterns[slots[id].pattern][offset]>>(7-(x&7)))&1;
}
static int collide_now(void) {
    int i,j,x,y,x0,y0,x1,y1;
    for(i=0;i<SPRITES;i++)for(j=i+1;j<SPRITES;j++){
        if(!slots[i].visible||!slots[j].visible||!slots[i].color||!slots[j].color)continue;
        x0=slots[i].x>slots[j].x?slots[i].x:slots[j].x;
        y0=slots[i].y>slots[j].y?slots[i].y:slots[j].y;
        x1=(slots[i].x<slots[j].x?slots[i].x:slots[j].x)+side*scale;
        y1=(slots[i].y<slots[j].y?slots[i].y:slots[j].y)+side*scale;
        for(y=y0;y<y1;y++)for(x=x0;x<x1;x++)if(opaque(i,x,y)&&opaque(j,x,y))return 1;
    }
    return 0;
}
int sprite_collision(void) {return collide_now();}
int sprite_collision_latched(void) {return collision_latched;}
int sprite_take_event(void) {int target=collision_pending;collision_pending=0;return target;}
void sprite_poll_collision(void) {
    int now=collide_now();
    if(collision_enabled && now && !collision_previous) {
        collision_latched=1;
        if(collision_target)collision_pending=collision_target;
    }
    collision_previous=now;
}
int sprite_event_statement(Lexer *lx) {
    int on;
    if(lx->cur.type!=T_IDENT || _stricmp(lx->cur.text,"SPRITE"))return fail("event syntax");
    lx_next(lx);
    if(lx->cur.type==T_ON || lx->cur.type==T_ONKW)on=1;
    else if(lx->cur.type==T_OFF)on=0;
    else if(lx->cur.type==T_STOP)on=2;
    else return fail("event syntax");
    lx_next(lx);if(lx->cur.type!=T_END)return fail("event syntax");
    if(on==0){collision_enabled=0;collision_latched=0;collision_pending=0;}
    else if(on==1){collision_enabled=1;collision_previous=0;sprite_poll_collision();}
    else collision_enabled=0; /* STOP pauses detection, retaining the latch */
    return 0;
}
void sprite_overlay(unsigned int *argb,int width,int height,const unsigned int *palette) {
    int i,x,y,xx,yy,offset,bit,dx,dy;
    sprite_poll_collision();
    for(i=SPRITES-1;i>=0;i--) {
        if(!slots[i].visible||!defined[slots[i].pattern]||!slots[i].color)continue;
        for(y=0;y<side;y++)for(x=0;x<side;x++){
            offset=side==8?y:(x>=8?16+y:y);
            bit=7-(x&7);
            if(!(patterns[slots[i].pattern][offset]&(1u<<bit)))continue;
            for(dy=0;dy<scale;dy++)for(dx=0;dx<scale;dx++){
                xx=slots[i].x+x*scale+dx;yy=slots[i].y+y*scale+dy;
                if(xx<0||yy<0||xx>=width||yy>=height)continue;
                argb[yy*width+xx]=palette[slots[i].color];
            }
        }
    }
}

int sprite_set_collision_target(int line) {
    if(line<1||line>65535)return fail("event line range");
    collision_target=line;collision_pending=0;
    return 0;
}
