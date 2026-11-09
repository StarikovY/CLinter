#ifndef CLINTER_MUSIC_H
#define CLINTER_MUSIC_H
#include "runtime.h"
#ifdef __cplusplus
extern "C" {
#endif
int music_statement(Lexer *lx);
int music_play_control(Lexer *lx);
int music_set_play_target(int line);
int music_take_play_event(void);
#ifdef __cplusplus
}
#endif
#endif
