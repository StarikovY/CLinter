/* MSX2 primitive graphics: software-indexed framebuffer, optional SDL2 presentation.
   Coordinates and colors remain deterministic with CLINTER_GRAPHICS_HEADLESS=1. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "graphics.h"
#include "sprite.h"
#include "parse.h"
#ifdef CLINTER_HAVE_SDL2
#include <SDL.h>
#endif
static const unsigned char input_font[95][12] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x40,0x00,0x00,0x00,0x00},
    {0x60,0x60,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x28,0x30,0x30,0x78,0x50,0x78,0x50,0x60,0x00,0x00,0x00,0x00},
    {0x38,0x54,0x54,0x70,0x18,0x14,0x54,0x38,0x10,0x00,0x00,0x00},
    {0x72,0x54,0x54,0x78,0x0f,0x15,0x25,0x27,0x00,0x00,0x00,0x00},
    {0x38,0x40,0x44,0x3e,0x44,0x44,0x44,0x3c,0x00,0x00,0x00,0x00},
    {0x40,0x40,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x20,0x40,0x40,0x40,0x40,0x40,0x40,0x20,0x20,0x00,0x00,0x00},
    {0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x10,0x54,0x38,0x28,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x10,0x10,0x7c,0x10,0x10,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00},
    {0x40,0x40,0x80,0x80,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x38,0x6c,0x44,0x44,0x44,0x44,0x6c,0x38,0x00,0x00,0x00,0x00},
    {0x30,0x50,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x00,0x00},
    {0x30,0x48,0x08,0x08,0x10,0x20,0x40,0xf8,0x00,0x00,0x00,0x00},
    {0x70,0x88,0x08,0x30,0x08,0x88,0x88,0x70,0x00,0x00,0x00,0x00},
    {0x08,0x18,0x28,0x28,0x48,0xfc,0x08,0x08,0x00,0x00,0x00,0x00},
    {0x78,0x80,0x80,0xb0,0xc8,0x08,0x88,0x70,0x00,0x00,0x00,0x00},
    {0x38,0x24,0x44,0x78,0x44,0x44,0x44,0x38,0x00,0x00,0x00,0x00},
    {0xf8,0x08,0x10,0x10,0x20,0x20,0x40,0x40,0x00,0x00,0x00,0x00},
    {0x38,0x44,0x44,0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00,0x00},
    {0x38,0x44,0x44,0x44,0x3c,0x44,0x48,0x38,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x40,0x80,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x18,0x60,0x40,0x30,0x08,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x78,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x60,0x18,0x08,0x30,0x40,0x00,0x00,0x00,0x00},
    {0x30,0x48,0x48,0x08,0x10,0x10,0x00,0x10,0x00,0x00,0x00,0x00},
    {0x0e,0x31,0x2e,0x52,0x52,0x54,0x5b,0x20,0x1e,0x00,0x00,0x00},
    {0x10,0x30,0x28,0x48,0x78,0x44,0x44,0x84,0x00,0x00,0x00,0x00},
    {0x78,0x44,0x44,0x48,0x7c,0x44,0x44,0x78,0x00,0x00,0x00,0x00},
    {0x1c,0x22,0x42,0x40,0x40,0x42,0x22,0x3c,0x00,0x00,0x00,0x00},
    {0x78,0x44,0x42,0x42,0x42,0x42,0x44,0x78,0x00,0x00,0x00,0x00},
    {0x7c,0x40,0x40,0x40,0x78,0x40,0x40,0x7c,0x00,0x00,0x00,0x00},
    {0x7c,0x40,0x40,0x40,0x78,0x40,0x40,0x40,0x00,0x00,0x00,0x00},
    {0x1c,0x26,0x42,0x40,0x4e,0x42,0x26,0x3a,0x00,0x00,0x00,0x00},
    {0x42,0x42,0x42,0x42,0x7e,0x42,0x42,0x42,0x00,0x00,0x00,0x00},
    {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00},
    {0x08,0x08,0x08,0x08,0x08,0x48,0x48,0x30,0x00,0x00,0x00,0x00},
    {0x44,0x48,0x50,0x50,0x70,0x50,0x48,0x44,0x00,0x00,0x00,0x00},
    {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x7c,0x00,0x00,0x00,0x00},
    {0x63,0x63,0x63,0x55,0x55,0x55,0x59,0x49,0x00,0x00,0x00,0x00},
    {0x64,0x64,0x64,0x54,0x54,0x54,0x4c,0x4c,0x00,0x00,0x00,0x00},
    {0x1c,0x22,0x41,0x41,0x41,0x41,0x22,0x1c,0x00,0x00,0x00,0x00},
    {0x78,0x44,0x44,0x44,0x78,0x40,0x40,0x40,0x00,0x00,0x00,0x00},
    {0x1c,0x22,0x41,0x41,0x41,0x41,0x22,0x1f,0x00,0x00,0x00,0x00},
    {0x78,0x44,0x44,0x44,0x78,0x4c,0x44,0x44,0x00,0x00,0x00,0x00},
    {0x38,0x44,0x40,0x20,0x1c,0x04,0x44,0x38,0x00,0x00,0x00,0x00},
    {0x7c,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x00,0x00},
    {0x44,0x44,0x44,0x44,0x44,0x44,0x44,0x38,0x00,0x00,0x00,0x00},
    {0x84,0x44,0x44,0x48,0x28,0x28,0x30,0x10,0x00,0x00,0x00,0x00},
    {0x8c,0x4c,0x4c,0x54,0x55,0x53,0x33,0x23,0x00,0x00,0x00,0x00},
    {0x44,0x48,0x28,0x30,0x30,0x28,0x48,0x44,0x00,0x00,0x00,0x00},
    {0x44,0x44,0x28,0x28,0x10,0x10,0x10,0x10,0x00,0x00,0x00,0x00},
    {0x7c,0x04,0x08,0x10,0x10,0x20,0x40,0x7c,0x00,0x00,0x00,0x00},
    {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x60,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x40,0x40,0x00,0x00,0x00},
    {0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x00},
    {0x00,0x00,0x30,0x50,0x50,0x48,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf8,0x00,0x00,0x00},
    {0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x38,0x48,0x18,0x68,0x48,0x78,0x00,0x00,0x00,0x00},
    {0x40,0x40,0x78,0x44,0x44,0x44,0x44,0x78,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x38,0x44,0x40,0x40,0x44,0x38,0x00,0x00,0x00,0x00},
    {0x04,0x04,0x3c,0x44,0x44,0x44,0x44,0x3c,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x38,0x44,0x7c,0x40,0x44,0x38,0x00,0x00,0x00,0x00},
    {0x40,0x40,0xe0,0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x3c,0x44,0x44,0x44,0x44,0x3c,0x44,0x38,0x00,0x00},
    {0x40,0x40,0x78,0x64,0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00},
    {0x40,0x00,0x40,0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00},
    {0x40,0x00,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0xc0,0x00,0x00},
    {0x40,0x40,0x48,0x50,0x60,0x50,0x50,0x48,0x00,0x00,0x00,0x00},
    {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x60,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x77,0x49,0x49,0x49,0x49,0x49,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x78,0x64,0x44,0x44,0x44,0x44,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x38,0x44,0x44,0x44,0x44,0x38,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x78,0x44,0x44,0x44,0x44,0x78,0x40,0x40,0x00,0x00},
    {0x00,0x00,0x3c,0x44,0x44,0x44,0x44,0x3c,0x04,0x04,0x00,0x00},
    {0x00,0x00,0x70,0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x70,0x48,0x60,0x18,0x48,0x78,0x00,0x00,0x00,0x00},
    {0x40,0x40,0xe0,0x40,0x40,0x40,0x40,0x60,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x44,0x44,0x44,0x44,0x4c,0x3c,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x88,0x88,0x50,0x50,0x50,0x20,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x99,0x9a,0x5a,0x6a,0x66,0x24,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x20,0xa0,0x40,0xc0,0xa0,0x20,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x88,0x88,0x50,0x50,0x50,0x20,0x20,0x40,0x00,0x00},
    {0x00,0x00,0x78,0x08,0x10,0x20,0x40,0x78,0x00,0x00,0x00,0x00},
    {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x60,0x00,0x00,0x00},
    {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x00,0x00},
    {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0xc0,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x68,0x58,0x00,0x00,0x00,0x00,0x00,0x00}
};
#define GW 512
#define GH 212
/* CLinter page model: SCREEN 5/6 four pages, SCREEN 7 two pages, SCREEN 8 one page. */
#define MAX_PAGES 4
static unsigned char pages[MAX_PAGES][GW*GH];
static int active_page=0, display_page=0, page_count=0;
#define pixels (pages[active_page])
static int mode, width, ink=15, paper=0, border_color=0;
static int color_limit(void) { return mode==6?3:(mode==8?255:15); }
static int text_col=0, text_row=0;
static unsigned int rgb_palette[16];
#ifdef CLINTER_HAVE_SDL2
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static int sdl_started;
static int input_waiting;
static char input_overlay[1024];
static unsigned int palette[256]={
  0xff000000,0xff000000,0xff21c842,0xff5edc78,
  0xff5455ed,0xff7d76fc,0xffd4524d,0xff42ebf5,
  0xfffc5554,0xffff7978,0xffd4c154,0xffe6ce80,
  0xff21b03b,0xffc95bba,0xffcccccc,0xffffffff};
static void present(void);
static void reset_palette(void) {
    static const unsigned int defaults[16]={
      0xff000000,0xff000000,0xff21c842,0xff5edc78,
      0xff5455ed,0xff7d76fc,0xffd4524d,0xff42ebf5,
      0xfffc5554,0xffff7978,0xffd4c154,0xffe6ce80,
      0xff21b03b,0xffc95bba,0xffcccccc,0xffffffff};
    int i;
    for(i=0;i<16;i++)palette[i]=defaults[i];
    /* MSX2 SCREEN 8: GRB 3:3:2 direct-color encoding. */
    if(mode==8)for(i=0;i<256;i++){
        unsigned int gr=(unsigned int)((i>>5)&7),red=(unsigned int)((i>>2)&7),blue=(unsigned int)(i&3);
        palette[i]=0xff000000u|((red*255/7)<<16)|((gr*255/7)<<8)|(blue*255/3);
    }
}
#endif /* CLINTER_HAVE_SDL2 palette initialization */
int graphics_set_palette(int index,int red,int green,int blue) {
    if(index<0 || index>15 || red<0 || red>7 || green<0 || green>7 || blue<0 || blue>7 || mode==8)return -1;
    rgb_palette[index]=(unsigned int)((red<<6)|(green<<3)|blue);
#ifdef CLINTER_HAVE_SDL2
    palette[index]=0xff000000u|((unsigned int)(red*255/7)<<16)|((unsigned int)(green*255/7)<<8)|(unsigned int)(blue*255/7);
    present();
#endif
    return 0;
}
int graphics_get_palette(int index,int *red,int *green,int *blue) {
    unsigned int value;
    if(index<0 || index>15 || !red || !green || !blue)return -1;
    value=rgb_palette[index];*red=(value>>6)&7;*green=(value>>3)&7;*blue=value&7;return 0;
}
#ifdef CLINTER_HAVE_SDL2
/* Embedded 8x12 bitmap font for SDL INPUT overlay (ASCII 32..126). */

static void draw_input_text(const char *message) {
    int x=6, y=GH-19, k, j, b;
    SDL_Rect background={0,GH-23,width,23};
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer,0,0,0,235);
    SDL_RenderFillRect(renderer,&background);
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    for(k=0;message[k] && x+8<width;k++,x+=8) {
        unsigned char ch=(unsigned char)message[k];
        if(ch<32 || ch>126)ch='?';
        for(j=0;j<12;j++) for(b=0;b<8;b++)
            if(input_font[ch-32][j] & (0x80>>b)) SDL_RenderDrawPoint(renderer,x+b,y+j);
    }
}
static int queued_game_key = 0;
static void game_event(const SDL_Event *e) {
    if(e->type==SDL_QUIT || (e->type==SDL_WINDOWEVENT && e->window.event==SDL_WINDOWEVENT_CLOSE)) {g_ctrlc_pressed=1;return;}
    if(e->type==SDL_KEYDOWN) {
        SDL_Keycode key=e->key.keysym.sym;
        if(key==SDLK_c && (e->key.keysym.mod & KMOD_CTRL)) {g_ctrlc_pressed=1;return;}
        if(key>=SDLK_a && key<=SDLK_z) queued_game_key=(int)(key-SDLK_a+'A');
        else if(key>=SDLK_0 && key<=SDLK_9) queued_game_key=(int)key;
        else if(key==SDLK_SPACE) queued_game_key=' ';
        else if(key==SDLK_RETURN || key==SDLK_KP_ENTER) queued_game_key=13;
        else if(key==SDLK_ESCAPE) queued_game_key=27;
    }
}
int graphics_read_key(void) {
    int key;
    if(!window || input_waiting || getenv("CLINTER_GRAPHICS_HEADLESS"))return 0;
    graphics_poll_events();
    key=queued_game_key;queued_game_key=0;return key;
}
static void present(void) {
    unsigned int argb[GW*GH]; int x,y;
    SDL_Event event;
    if (!renderer || !texture) return;
    if (!input_waiting) while (SDL_PollEvent(&event)) game_event(&event);
    for(y=0;y<GH;y++) for(x=0;x<width;x++) argb[y*width+x]=palette[pages[display_page][y*GW+x]];
    sprite_overlay(argb,width,GH,palette);
    SDL_UpdateTexture(texture,NULL,argb,width*(int)sizeof(unsigned int));
    SDL_RenderClear(renderer); SDL_RenderCopy(renderer,texture,NULL,NULL);
    if (input_waiting) draw_input_text(input_overlay);
    SDL_RenderPresent(renderer);
}
#else
int graphics_read_key(void) { return 0; }
static void present(void) {}
#endif
void graphics_refresh(void) { present(); }
int graphics_active(void) { return mode != 0; }
int graphics_current_mode(void) { return mode; }
void graphics_poll_events(void) {
#ifdef CLINTER_HAVE_SDL2
    SDL_Event event;
    if(window && !input_waiting)while(SDL_PollEvent(&event)) {
        game_event(&event);
    }
#endif
}
int graphics_input(const char *prompt,char *out,size_t capacity) {
#ifdef CLINTER_HAVE_SDL2
    char display[1024]; size_t len=0;
    SDL_Event event;
    if(mode && window && renderer && texture && !getenv("CLINTER_GRAPHICS_HEADLESS")) {
        if(!capacity)return -1;
        out[0]=0;
        SDL_RaiseWindow(window);
        SDL_StartTextInput();
        input_waiting=1;
        for(;;) {
            snprintf(display,sizeof(display),"%s%s_",prompt?prompt:"? ",out);
            snprintf(input_overlay,sizeof(input_overlay),"%s",display);
            present();
            if(g_ctrlc_pressed) {input_waiting=0;SDL_StopTextInput();return -1;}
            if(!SDL_WaitEventTimeout(&event,100))continue;
            if(event.type==SDL_QUIT) { g_ctrlc_pressed=1; input_waiting=0; SDL_StopTextInput(); return -1; }
            if(event.type==SDL_KEYDOWN) {
                if(event.key.keysym.sym==SDLK_c && (event.key.keysym.mod & KMOD_CTRL)) {g_ctrlc_pressed=1;input_waiting=0;SDL_StopTextInput();return -1;}
                if(event.key.keysym.sym==SDLK_RETURN || event.key.keysym.sym==SDLK_KP_ENTER)break;
                if(event.key.keysym.sym==SDLK_BACKSPACE && len)out[--len]=0;
            } else if(event.type==SDL_TEXTINPUT) {
                size_t n=strlen(event.text.text);
                if(n && n<capacity-len) { memcpy(out+len,event.text.text,n+1);len+=n; }
            }
        }
        input_waiting=0;SDL_StopTextInput();present();return 0;
    }
#endif
    (void)prompt;(void)out;(void)capacity;
    return 1; /* No native window: use console INPUT. */
}

void graphics_shutdown(void) {
#ifdef CLINTER_HAVE_SDL2
    if(texture) SDL_DestroyTexture(texture); texture=NULL;
    if(renderer) SDL_DestroyRenderer(renderer); renderer=NULL;
    if(window) SDL_DestroyWindow(window); window=NULL;
    if(sdl_started) SDL_QuitSubSystem(SDL_INIT_VIDEO); sdl_started=0;
#endif
    mode=0;active_page=display_page=page_count=0;
}
static int draw_x=0,draw_y=0;
int graphics_mode(int newmode) {
    if(newmode!=0 && newmode!=2 && newmode!=3 && newmode!=4 && newmode!=5 && newmode!=6 && newmode!=7 && newmode!=8) return -1;
    if(newmode==0) { graphics_shutdown(); return 0; }
    sprite_reset();
    mode=newmode; memset(rgb_palette,0,sizeof(rgb_palette)); width=(mode==5 || mode==2 || mode==3 || mode==4 || mode==8)?256:512; ink=(mode==6)?3:15; paper=0;
    memset(pages,0,sizeof(pages));/* Keep SCREEN 8 single-page until its page layout is implemented; G3 relies on this. */
    page_count=(mode==2 || mode==3 || mode==4)?1:((mode==5 || mode==6)?4:((mode==7)?2:1));active_page=display_page=0;draw_x=draw_y=0;
#ifdef CLINTER_HAVE_SDL2
    reset_palette();
    if(!getenv("CLINTER_GRAPHICS_HEADLESS")) {
        if(!sdl_started) { if(SDL_InitSubSystem(SDL_INIT_VIDEO)==0) sdl_started=1; }
        if(sdl_started) {
            if(texture) { SDL_DestroyTexture(texture); texture=NULL; }
            if(renderer) { SDL_DestroyRenderer(renderer); renderer=NULL; }
            if(window) { SDL_DestroyWindow(window); window=NULL; }
            window=SDL_CreateWindow("CLinter MSX2 Graphics",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,width*2,GH*2,SDL_WINDOW_SHOWN);
            if(window) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
            if(!renderer && window) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
            if(renderer) { SDL_RenderSetLogicalSize(renderer,width,GH); texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,width,GH); }
            if(!texture) { fprintf(stderr,"WARNING: SDL2 window unavailable; graphics framebuffer remains active\n"); }
        } else fprintf(stderr,"WARNING: SDL2 video unavailable; graphics framebuffer remains active\n");
    }
#endif
    present(); return 0;
}
int graphics_pages(int active,int display) {
    if(!mode || active<0 || display<0 || active>=page_count || display>=page_count)return -1;
    active_page=active;display_page=display;present();return 0;
}
void graphics_clear(void) { if(mode) { memset(pixels,paper,sizeof(pixels)); text_col=text_row=0; present(); } }
void graphics_color(int foreground,int background) { ink=foreground; paper=background; }
void graphics_border(int color) { border_color=color; (void)border_color; }
void graphics_locate(int col,int row) { text_col=col; text_row=row; }
void graphics_text(const char *text,size_t count) {
    size_t i; int x,y,b,j;
    if(!mode || !text)return;
    for(i=0;i<count;i++) {
        unsigned char ch=(unsigned char)text[i];
        if(ch=='\r') {text_col=0;continue;}
        if(ch=='\n') {text_col=0;text_row++;continue;}
        if(text_col>=width/8) {text_col=0;text_row++;}
        x=text_col*8;y=text_row*12;
        if(ch<32 || ch>126)ch='?';
        for(j=0;j<12;j++)for(b=0;b<8;b++)
            if(y+j>=0 && y+j<GH && x+b<width && (input_font[ch-32][j] & (0x80>>b)))
                pixels[(y+j)*GW+x+b]=(unsigned char)ink;
        text_col++;
    }
    present();
}
int graphics_point(int x,int y) { return (mode && x>=0 && x<width && y>=0 && y<GH)?pixels[y*GW+x]:-1; }
static void pixel(int x,int y,int c) { if(x>=0 && x<width && y>=0 && y<GH) pixels[y*GW+x]=(unsigned char)c; }
static void line(int x0,int y0,int x1,int y1,int c) {
    int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
    for(;;) { pixel(x0,y0,c); if(x0==x1 && y0==y1) break;
        { int e=2*err; if(e>=dy) {err+=dy;x0+=sx;} if(e<=dx) {err+=dx;y0+=sy;} }
    }
}
static void rectangle(int x0,int y0,int x1,int y1,int c,int fill) {
    int y,t;
    if(x0>x1){t=x0;x0=x1;x1=t;} if(y0>y1){t=y0;y0=y1;y1=t;}
    if(fill) { for(y=y0;y<=y1;y++) line(x0,y,x1,y,c); }
    else {line(x0,y0,x1,y0,c);line(x0,y1,x1,y1,c);line(x0,y0,x0,y1,c);line(x1,y0,x1,y1,c);}
}
static void circle(int cx,int cy,int r,int c) {
    int x=r,y=0,d=1-r;
    while(x>=y) {
        pixel(cx+x,cy+y,c);pixel(cx+y,cy+x,c);pixel(cx-y,cy+x,c);pixel(cx-x,cy+y,c);
        pixel(cx-x,cy-y,c);pixel(cx-y,cy-x,c);pixel(cx+y,cy-x,c);pixel(cx+x,cy-y,c);
        y++;if(d<0)d+=2*y+1;else{x--;d+=2*(y-x)+1;}
    }
}
/* MSX BASIC CIRCLE: optional radians (start/end) and vertical aspect ratio. */
static void circle_arc(int cx,int cy,int r,int c,double start,double end,double aspect,int full,int radial_start,int radial_end) {
    int i,steps=(r>0?r*16:16),px=0,py=0;
    double a,theta;
    if(steps<128)steps=128;
    if(steps>32768)steps=32768;
    if(full) { start=0;end=6.283185307179586; }
    if(start<0)start=-start;
    if(end<0)end=-end;
    if(end<start)end+=6.283185307179586;
    for(i=0;i<=steps;i++) {
        a=start+(end-start)*(double)i/steps;
        theta=a;
        { int x=cx+(int)round(r*cos(theta)),y=cy-(int)round(r*aspect*sin(theta));
          if(i)line(px,py,x,y,c);else pixel(x,y,c);
          px=x;py=y;
        }
    }
    if(radial_start)line(cx,cy,cx+(int)round(r*cos(start)),cy-(int)round(r*aspect*sin(start)),c);
    if(radial_end)line(cx,cy,cx+(int)round(r*cos(end)),cy-(int)round(r*aspect*sin(end)),c);
}
/* Bounded scanline-free flood fill: each pixel is pushed at most once. */
static int flood_border(int x,int y,int c,int border,int use_border) {
    int *stack,top=0,limit=width*GH,old=graphics_point(x,y);
    if(old<0 || old==c || (use_border && old==border))return 0;
    stack=(int*)malloc((size_t)limit*sizeof(int));if(!stack)return -1;
    pixel(x,y,c);stack[top++]=y*width+x;
    while(top) {
        int v=stack[--top],px=v%width,py=v/width,nx[4]={px-1,px+1,px,px},ny[4]={py,py,py-1,py+1},k;
        for(k=0;k<4;k++) if(nx[k]>=0 && nx[k]<width && ny[k]>=0 && ny[k]<GH) {
            int existing=graphics_point(nx[k],ny[k]);
            if((use_border?(existing!=border && existing!=c):(existing==old))) {
                pixel(nx[k],ny[k],c);stack[top++]=ny[k]*width+nx[k];
            }
        }
    }
    free(stack);return 0;
}
static int fail(const char *why) { printf("ERROR: GRAPHICS %s\n",why);return -1; }
static int number(Lexer *lx,int *v) {
    double d=parse_rel(lx);
    if(!isfinite(d)||d<-100000||d>100000||trunc(d)!=d)return 0;
    *v=(int)d;return 1;
}
static int expect(Lexer *lx,TokType type) { if(lx->cur.type!=type)return 0;lx_next(lx);return 1; }
static int coord(Lexer *lx,int *x,int *y) {
    return expect(lx,T_LPAREN)&&number(lx,x)&&expect(lx,T_COMMA)&&number(lx,y)&&expect(lx,T_RPAREN);
}
static int draw_string(const char *str) {
    int x=draw_x,y=draw_y,c=ink,scale=1,penup=0,noupdate=0;
    const char *p=str;
    while(*p) {
        char cmd;int n=0,sign=1,has=0,dx=0,dy=0,nx,ny,xrelative=0;
        while(*p==' ' || *p==';')p++;
        if(!*p)break;
        cmd=(char)toupper((unsigned char)*p++);
        if(cmd=='B') {penup=1;continue;}
        if(cmd=='N') {noupdate=1;continue;}
        if(*p=='+' || *p=='-'){xrelative=1;if(*p=='-')sign=-1;p++;}
        while(*p>='0' && *p<='9') {has=1;if(n>10000)return -1;n=n*10+(*p++-'0');}
        n*=sign;
        if(cmd=='C') {if(!has || n<0 || n>color_limit())return -1;c=n;continue;}
        if(cmd=='S') {if(!has || n<1 || n>256)return -1;scale=n;continue;}
        if(cmd=='M') {
            int yy=0,ysign=1,yhas=0,relative=xrelative;
            if(!has)return -1;
            if(*p!=',')return -1;p++;
            if(*p=='+' || *p=='-'){relative=1;if(*p=='-')ysign=-1;p++;}
            while(*p>='0' && *p<='9'){yhas=1;if(yy>10000)return -1;yy=yy*10+(*p++-'0');}
            if(!yhas)return -1;
            nx=relative?x+n:n;ny=relative?y+ysign*yy:ysign*yy;
        } else {
            if(!has)n=1;
            n*=scale;
            switch(cmd) {
            case 'U':dy=-n;break;case 'D':dy=n;break;
            case 'L':dx=-n;break;case 'R':dx=n;break;
            case 'E':dx=n;dy=-n;break;case 'F':dx=n;dy=n;break;
            case 'G':dx=-n;dy=n;break;case 'H':dx=-n;dy=-n;break;
            default:return -1;
            }
            nx=x+dx;ny=y+dy;
        }
        if(nx<-100000 || nx>100000 || ny<-100000 || ny>100000)return -1;
        if(!penup)line(x,y,nx,ny,c);
        if(!noupdate){x=nx;y=ny;}
        penup=noupdate=0;
    }
    draw_x=x;draw_y=y;return 0;
}
int graphics_statement(Lexer *lx) {
    char op[16];int x,y,x2,y2,c=ink,r,shape=0,relative=0;
    strncpy(op,lx->cur.text,sizeof(op)-1);op[sizeof(op)-1]=0;lx_next(lx);
    if(!mode)return fail("SCREEN 5 or SCREEN 7 required");
    if(!_stricmp(op,"PSET") || !_stricmp(op,"PRESET")) {
        if(!coord(lx,&x,&y))return fail("pixel syntax");
        if(expect(lx,T_COMMA)) {if(!number(lx,&c))return fail("color syntax");}
        else if(!_stricmp(op,"PRESET"))c=paper;
        if(lx->cur.type!=T_END || c<0 || c>color_limit())return fail("pixel syntax/color");
        pixel(x,y,c);
    } else if(!_stricmp(op,"LINE")) {
        if(lx->cur.type==T_MINUS) { x=draw_x;y=draw_y; }
        else if(!coord(lx,&x,&y))return fail("LINE coordinates");
        if(!expect(lx,T_MINUS))return fail("LINE coordinates");
        if(lx->cur.type==T_STEP){relative=1;lx_next(lx);}
        if(!coord(lx,&x2,&y2))return fail("LINE coordinates");
        if(relative){x2+=x;y2+=y;}
        if(expect(lx,T_COMMA)) {
            if(lx->cur.type!=T_COMMA && lx->cur.type!=T_END && !number(lx,&c))return fail("LINE color");
        }
        if(expect(lx,T_COMMA)) {
            if(lx->cur.type!=T_IDENT)return fail("LINE shape");
            if(!_stricmp(lx->cur.text,"B"))shape=1;
            else if(!_stricmp(lx->cur.text,"BF"))shape=2;
            else return fail("LINE shape");
            lx_next(lx);
        }
        if(lx->cur.type!=T_END || c<0 || c>color_limit())return fail("LINE syntax/color");
        if(shape)rectangle(x,y,x2,y2,c,shape==2);else line(x,y,x2,y2,c);draw_x=x2;draw_y=y2;
    } else if(!_stricmp(op,"CIRCLE")) {
        if(!coord(lx,&x,&y)||!expect(lx,T_COMMA)||!number(lx,&r))return fail("CIRCLE syntax");
        {double start=0,end=0,aspect=1.0;int full=1,rs=0,re=0;
         if(expect(lx,T_COMMA)) {
             if(lx->cur.type!=T_COMMA && lx->cur.type!=T_END && !number(lx,&c))return fail("CIRCLE color");
             if(expect(lx,T_COMMA)) {
                 if(lx->cur.type!=T_COMMA && lx->cur.type!=T_END) {start=parse_rel(lx);rs=start<0;full=0;}
                 if(expect(lx,T_COMMA)) {
                     if(lx->cur.type!=T_COMMA && lx->cur.type!=T_END){end=parse_rel(lx);re=end<0;full=0;}
                     if(expect(lx,T_COMMA))aspect=parse_rel(lx);
                 }
             }
         }
         if(lx->cur.type!=T_END || r<0 || r>2048 || c<0 || c>color_limit() || !isfinite(start) || !isfinite(end) || !isfinite(aspect) || aspect<=0 || aspect>10 || fabs(start)>6.283185307179586 || fabs(end)>6.283185307179586)return fail("CIRCLE radius/color/arc/aspect");
         circle_arc(x,y,r,c,start,end,aspect,full,rs,re);
        }
    } else if(!_stricmp(op,"PAINT")) {
        if(!coord(lx,&x,&y))return fail("PAINT coordinates");
        {int border=0,use_border=0;
         if(expect(lx,T_COMMA)) {
             if(lx->cur.type!=T_COMMA && lx->cur.type!=T_END && !number(lx,&c))return fail("PAINT color");
             if(expect(lx,T_COMMA)){use_border=1;if(!number(lx,&border))return fail("PAINT border");}
         }
         if(lx->cur.type!=T_END || c<0 || c>color_limit() || (use_border && (border<0 || border>color_limit())))return fail("PAINT syntax/color");
         if(flood_border(x,y,c,border,use_border))return fail("out of memory");
        }
    } else if(!_stricmp(op,"COPY")) {
        int source_page=active_page,dest_page=active_page,dx,dy,sw,sh,xx,yy;
        unsigned char *snapshot;size_t count;
        if(!coord(lx,&x,&y)||!expect(lx,T_MINUS)||!coord(lx,&x2,&y2))return fail("COPY source rectangle");
        if(expect(lx,T_COMMA) && !number(lx,&source_page))return fail("COPY source page");
        if(!expect(lx,T_TO)||!coord(lx,&dx,&dy))return fail("COPY TO coordinates");
        if(expect(lx,T_COMMA) && !number(lx,&dest_page))return fail("COPY destination page");
        if(lx->cur.type!=T_END || source_page<0 || source_page>=page_count || dest_page<0 || dest_page>=page_count || x<0 || y<0 || x2<x || y2<y || x2>=width || y2>=GH)return fail("COPY range/page/syntax");
        sw=x2-x+1;sh=y2-y+1;count=(size_t)sw*(size_t)sh;
        snapshot=(unsigned char*)malloc(count);if(!snapshot)return fail("COPY out of memory");
        for(yy=0;yy<sh;yy++)memcpy(snapshot+(size_t)yy*sw,pages[source_page]+(size_t)(y+yy)*GW+x,(size_t)sw);
        for(yy=0;yy<sh;yy++)for(xx=0;xx<sw;xx++) {
            int tx=dx+xx,ty=dy+yy;
            if(tx>=0 && tx<width && ty>=0 && ty<GH)pages[dest_page][(size_t)ty*GW+tx]=snapshot[(size_t)yy*sw+xx];
        }
        free(snapshot);
    } else if(!_stricmp(op,"DRAW")) {
        if(lx->cur.type!=T_STRING)return fail("DRAW string required");
        if(draw_string(lx->cur.text))return fail("DRAW command");
        lx_next(lx);if(lx->cur.type!=T_END)return fail("DRAW syntax");
    } else return fail("unknown statement");
    present();return 0;
}
