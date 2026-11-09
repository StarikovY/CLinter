/* G10/M3: parse ON SPRITE/PLAY GOSUB without changing ON expr dispatch. */
#include <stdio.h>
#include "msx_events.h"
#include "parse.h"
#include "sprite.h"
#include "music.h"
int msx_event_definition(Lexer *lx) {
    Lexer check=*lx;
    int is_play,target;
    if(check.cur.type!=T_ONKW)return -2;
    lx_next(&check);
    if(check.cur.type!=T_IDENT ||
       (_stricmp(check.cur.text,"SPRITE")&&_stricmp(check.cur.text,"PLAY")))return -2;
    is_play=!_stricmp(check.cur.text,"PLAY");
    lx_next(&check);
    if(check.cur.type!=T_GOSUB) {puts("ERROR: ON event needs GOSUB");return -1;}
    lx_next(&check);
    if(check.cur.type!=T_NUMBER || check.cur.number!=(int)check.cur.number) {
        puts("ERROR: ON event line");return -1;
    }
    target=(int)check.cur.number;
    lx_next(&check);
    if(check.cur.type!=T_END) {puts("ERROR: ON event trailing tokens");return -1;}
    return is_play?music_set_play_target(target):sprite_set_collision_target(target);
}
