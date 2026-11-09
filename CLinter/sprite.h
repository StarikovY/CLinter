#ifndef CLINTER_SPRITE_H
#define CLINTER_SPRITE_H
#include "runtime.h"
#ifdef __cplusplus
extern "C" {
#endif
int sprite_statement(Lexer *lx);
void sprite_reset(void);
int sprite_size(int size);
void sprite_overlay(unsigned int *argb,int width,int height,const unsigned int *palette);
int sprite_visible_count(void);
int sprite_collision(void);
int sprite_collision_latched(void);
int sprite_take_event(void);
void sprite_poll_collision(void);
int sprite_set_collision_target(int line);
int sprite_event_statement(Lexer *lx);
#ifdef __cplusplus
}
#endif
#endif
