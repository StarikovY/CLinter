/* M3: MSX PSG-compatible SOUND register writes and BEEP. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "music_psg.h"
#include "parse.h"
#ifdef CLINTER_HAVE_SDL2
#include <SDL.h>
#endif
static unsigned char regs[16];
static int psg_fail(const char *message) {printf("ERROR: PSG %s\n",message);return -1;}
#ifdef CLINTER_HAVE_SDL2
static SDL_AudioDeviceID psg_device;
static void emit_tone(double frequency,int volume,int milliseconds) {
    SDL_AudioSpec want,got;
    short buffer[512];int position=0,samples=milliseconds*22050/1000;
    if(!frequency||!volume||getenv("CLINTER_GRAPHICS_HEADLESS"))return;
    if(!psg_device) {
        if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)return;
        SDL_zero(want);want.freq=22050;want.format=AUDIO_S16SYS;want.channels=1;want.samples=512;
        psg_device=SDL_OpenAudioDevice(NULL,0,&want,&got,0);
        if(!psg_device)return;
        SDL_PauseAudioDevice(psg_device,0);
    }
    SDL_ClearQueuedAudio(psg_device);
    while(position<samples) {
        int i,n=samples-position;if(n>512)n=512;
        for(i=0;i<n;i++) {
            double phase=fmod((position+i)*frequency/22050.0,1.0);
            buffer[i]=(short)((phase<0.5?1:-1)*volume*600);
        }
        SDL_QueueAudio(psg_device,buffer,n*sizeof(short));position+=n;
    }
    while(SDL_GetQueuedAudioSize(psg_device)>0)SDL_Delay(5);
}
#else
static void emit_tone(double frequency,int volume,int milliseconds) {(void)frequency;(void)volume;(void)milliseconds;}
#endif
int music_psg_statement(Lexer *lx) {
    int beep= !_stricmp(lx->cur.text,"BEEP");
    lx_next(lx);
    if(beep) {
        if(lx->cur.type!=T_END)return psg_fail("BEEP syntax");
        emit_tone(880,12,110);return 0;
    }
    {
        double reg=parse_rel(lx),val;
        int r,v;
        if(!isfinite(reg)||reg!=trunc(reg)||reg<0||reg>15)return psg_fail("register range");
        if(lx->cur.type!=T_COMMA)return psg_fail("SOUND syntax");
        lx_next(lx);val=parse_rel(lx);
        if(!isfinite(val)||val!=trunc(val)||val<0||val>255||lx->cur.type!=T_END)return psg_fail("value range");
        r=(int)reg;v=(int)val;
        if((r==1||r==3||r==5)&&v>15)return psg_fail("tone high byte");
        if((r==6)&&v>31)return psg_fail("noise period");
        if((r==8||r==9||r==10)&&v>31)return psg_fail("channel volume");
        if(r==13&&v>15)return psg_fail("envelope shape");
        regs[r]=(unsigned char)v;
        /* Emit short tone for audible PSG register programming. */
        if(r>=8&&r<=10 && (v&15)) {
            int ch=r-8,period=regs[ch*2]+((regs[ch*2+1]&15)<<8);
            if(period>0 && !(regs[7]&(1<<ch)))emit_tone(1789772.5/(16.0*period),v&15,100);
        }
        return 0;
    }
}
