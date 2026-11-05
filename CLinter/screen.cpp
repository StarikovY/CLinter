/* PYLinter/MSX-compatible text-screen statements. ANSI is the portable output backend. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "screen.h"
#include "parse.h"

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
        if (!screen_integer(lx,&a) || !screen_end(lx)) return screen_error("mode syntax");
        if (a != 0) return screen_error("unsupported mode (only SCREEN 0)");
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
        printf("\033[%d;%dH", b+1, a+1);
        fflush(stdout);
        return 0;
    }
    if (_stricmp(command,"COLOR") == 0) {
        if (!screen_integer(lx,&a) || lx->cur.type != T_COMMA) return screen_error("COLOR syntax");
        lx_next(lx);
        if (!screen_integer(lx,&b) || !screen_end(lx)) return screen_error("COLOR syntax");
        if (a < 0 || a > 15 || b < 0 || b > 15) return screen_error("COLOR index out of range");
        screen_foreground = a; screen_background = b;
        printf("\033[%d;%dm", screen_color_code(a,0), screen_color_code(b,1));
        fflush(stdout);
        return 0;
    }
    (void)screen_active;
    return screen_error("unknown statement");
}
