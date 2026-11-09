/* PYLinter/MSX-compatible text-screen statements. ANSI is the portable output backend. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "screen.h"
#include "parse.h"
#include "graphics.h"
#include "sprite.h"

static int screen_active = 0;
static int screen_x = 0, screen_y = 0;
static int screen_foreground = 7, screen_background = 0;

static int screen_error(const char *msg) {
    printf("ERROR: SCREEN %s\n", msg);
    return -1;
}
static int screen_integer(Lexer *lx, int *out) {
    double v = parse_rel(lx);
    if (!isfinite(v) || v < -100000.0 || v > 100000.0 || trunc(v) != v)
        return 0;
    *out = (int)v;
    return 1;
}
static int screen_end(Lexer *lx) { return lx->cur.type == T_END; }
static int screen_color_code(int index, int background) {
    static const int ansi_normal[8] = {0,4,2,6,1,5,3,7};
    int bright = index >= 8;
    return (background ? (bright ? 100 : 40) : (bright ? 90 : 30)) + ansi_normal[index & 7];
}
int screen_statement(Lexer *lx) {
    char command[16];
    int a, b;
    strncpy(command, lx->cur.text, sizeof(command)-1);
    command[sizeof(command)-1] = '\0';
    lx_next(lx);
    if (_stricmp(command,"SCREEN") == 0) {
        int active=0,display=0,has_pages=0,field,spritesize=0;
        if (!screen_integer(lx,&a)) return screen_error("mode syntax");
        /* MSX: SCREEN mode[,sprite-size[,key-click[,active-page[,display-page]]]] */
        if(lx->cur.type==T_COMMA) {
            int slot;
            for(slot=1;slot<=4 && lx->cur.type==T_COMMA;slot++) {
                lx_next(lx);
                if(lx->cur.type==T_COMMA || lx->cur.type==T_END)continue;
                if(!screen_integer(lx,&field))return screen_error("SCREEN parameter");
                if(slot==1)spritesize=field;
                if(slot==3){active=field;has_pages=1;}
                if(slot==4){display=field;has_pages=1;}
            }
        }
        if(!screen_end(lx))return screen_error("SCREEN syntax");
        if(a!=0 && a!=2 && a!=3 && a!=4 && a!=5 && a!=6 && a!=7 && a!=8)return screen_error("unsupported mode (SCREEN 0, 2, 3, 4, 5, 6, 7, 8)");
        if(a==0 && has_pages)return screen_error("SCREEN 0 pages");
        if(graphics_current_mode()!=a && graphics_mode(a)!=0)return screen_error("mode");
        if(a!=0){if(sprite_size(spritesize)!=0)return screen_error("sprite size");if(graphics_pages(active,display)!=0)return screen_error("page out of range");return 0;}
        screen_active = 1;
        screen_x = screen_y = 0;
        screen_foreground = 7; screen_background = 0;
        /* SCREEN 0: 80x24 logical text screen. Do not force terminal resize. */
        fputs("\033[0m\033[H", stdout);
        fflush(stdout);
        return 0;
    }
    if (_stricmp(command,"CLS") == 0) {
        if (!screen_end(lx)) return screen_error("CLS syntax");
        screen_x = screen_y = 0;
        if (graphics_active()) { graphics_clear(); return 0; }
        fputs("\033[2J\033[H", stdout);
        fflush(stdout);
        return 0;
    }
    if (_stricmp(command,"LOCATE") == 0) {
        if (!screen_integer(lx,&a) || lx->cur.type != T_COMMA) return screen_error("LOCATE syntax");
        lx_next(lx);
        if (!screen_integer(lx,&b) || !screen_end(lx)) return screen_error("LOCATE syntax");
        /* MSX coordinates: X=column, Y=row, both zero-based.
           Permit Y=24 as a compatibility status line: ColorScreen1.bas uses it. */
        if (a < 0 || a >= 80 || b < 0 || b > 24) return screen_error("LOCATE coordinates out of range");
        screen_x = a; screen_y = b;
        if (graphics_active()) { graphics_locate(a,b); return 0; }
        printf("\033[%d;%dH", b+1, a+1);
        fflush(stdout);
        return 0;
    }
    /* MSX2 programmable palette: COLOR=(index,red,green,blue), RGB 0..7. */
    if (_stricmp(command,"COLOR") == 0 && lx->cur.type==T_EQ) {
        int red,green,blue;
        lx_next(lx);
        if(lx->cur.type!=T_LPAREN)return screen_error("palette syntax");
        lx_next(lx);
        if(!screen_integer(lx,&a) || lx->cur.type!=T_COMMA)return screen_error("palette index");
        lx_next(lx);
        if(!screen_integer(lx,&red) || lx->cur.type!=T_COMMA)return screen_error("palette red");
        lx_next(lx);
        if(!screen_integer(lx,&green) || lx->cur.type!=T_COMMA)return screen_error("palette green");
        lx_next(lx);
        if(!screen_integer(lx,&blue) || lx->cur.type!=T_RPAREN)return screen_error("palette blue");
        lx_next(lx);
        if(!screen_end(lx) || graphics_set_palette(a,red,green,blue))return screen_error("palette range/syntax");
        return 0;
    }
    if (_stricmp(command,"COLOR") == 0) {
        if (!screen_integer(lx,&a) || lx->cur.type != T_COMMA) return screen_error("COLOR syntax");
        lx_next(lx);
        if (!screen_integer(lx,&b)) return screen_error("COLOR syntax");
        if (lx->cur.type == T_COMMA) {
            int border;
            lx_next(lx);
            if (!screen_integer(lx,&border) || border<0 || border>15) return screen_error("COLOR border");
            graphics_border(border);
        }
        if (!screen_end(lx)) return screen_error("COLOR syntax");
        if (a < 0 || a > (graphics_current_mode()==8?255:(graphics_current_mode()==6?3:15)) || b < 0 || b > (graphics_current_mode()==8?255:(graphics_current_mode()==6?3:15))) return screen_error("COLOR index out of range");
        screen_foreground = a; screen_background = b;
        graphics_color(a,b);
        if (graphics_active()) return 0;
        printf("\033[%d;%dm", screen_color_code(a,0), screen_color_code(b,1));
        fflush(stdout);
        return 0;
    }
    (void)screen_active;
    return screen_error("unknown statement");
}
