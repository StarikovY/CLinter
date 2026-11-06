/* MSX BASIC PLAY: single-voice Music Macro Language, SDL2 queued audio. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "music.h"
#include "parse.h"
#ifdef CLINTER_HAVE_SDL2
#include <SDL.h>
#endif

static int music_error(const char *msg) { fprintf(stderr,"ERROR: PLAY %s\n",msg); return -1; }
static int digits(const char **s,int fallback) {
    int n=0,any=0;
    while(isdigit((unsigned char)**s)) {any=1;n=n*10+(*(*s)++-'0');if(n>10000)break;}
    return any?n:fallback;
}
#ifdef CLINTER_HAVE_SDL2
static SDL_AudioDeviceID device;
static int audio_open(void) {
    SDL_AudioSpec want,got;
    if(device)return 1;
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)return 0;
    SDL_zero(want);want.freq=22050;want.format=AUDIO_S16SYS;want.channels=1;want.samples=1024;
    device=SDL_OpenAudioDevice(NULL,0,&want,&got,0);
    if(!device || got.freq!=22050 || got.format!=AUDIO_S16SYS || got.channels!=1)return 0;
    SDL_PauseAudioDevice(device,0);return 1;
}
static void play_tone(double hz,int duration_ms,int volume) {
    const int rate=22050;
    int count=(int)((double)rate*duration_ms/1000.0),i;
    short buffer[1024];double phase=0,step=hz/rate;
    if(getenv("CLINTER_GRAPHICS_HEADLESS"))return;
    if(!audio_open())return;
    while(count>0) {
        int n=count>1024?1024:count;
        for(i=0;i<n;i++) {
            double env=(double)(count-i)/(rate*0.015);
            double gain=env<1.0?env:1.0;
            buffer[i]=(short)(hz>0 && fmod(phase,1.0)<0.5?volume*1000*gain:- (hz>0?volume*1000*gain:0));
            phase+=step;
        }
        if(SDL_QueueAudio(device,buffer,(unsigned int)(n*sizeof(short)))!=0)break;
        count-=n;
    }
    while(SDL_GetQueuedAudioSize(device)>0) SDL_Delay(5);
}
#else
static void play_tone(double hz,int duration_ms,int volume) {(void)hz;(void)duration_ms;(void)volume;}
#endif
static int mml(const char *s) {
    int octave=4,length=4,tempo=120,volume=12;
    static const int semitones[7]={9,11,0,2,4,5,7}; /* A B C D E F G */
    while(*s) {
        char c=(char)toupper((unsigned char)*s++);
        int n,acc=0,dots=0,duration;
        double freq=0,beats;
        if(isspace((unsigned char)c) || c==',')continue;
        if(c=='O') {octave=digits(&s,-1);if(octave<1||octave>8)return music_error("octave out of range");continue;}
        if(c=='L') {length=digits(&s,-1);if(length<1||length>64)return music_error("length out of range");continue;}
        if(c=='T') {tempo=digits(&s,-1);if(tempo<32||tempo>255)return music_error("tempo out of range");continue;}
        if(c=='V') {volume=digits(&s,-1);if(volume<0||volume>15)return music_error("volume out of range");continue;}
        if(c=='>' || c=='<') {octave+=(c=='>')?1:-1;if(octave<1||octave>8)return music_error("octave out of range");continue;}
        if((c<'A'||c>'G')&&c!='R')return music_error("unsupported MML command");
        if(*s=='#'||*s=='+') {acc=1;s++;}else if(*s=='-'){acc=-1;s++;}
        n=digits(&s,length);if(n<1||n>64)return music_error("note length out of range");
        while(*s=='.') {dots++;s++;if(dots>3)return music_error("too many dots");}
        beats=4.0/n;{double add=beats/2;int i;for(i=0;i<dots;i++){beats+=add;add/=2;}}
        duration=(int)(60000.0/tempo*beats);
        if(c!='R') {
            int semitone=semitones[c-'A']+acc;
            int midi=(octave+1)*12+semitone;
            freq=440.0*pow(2.0,(midi-69)/12.0);
        }
        play_tone(freq,duration,volume);
    }
    return 0;
}
int music_statement(Lexer *lx) {
    char sequence[512];
    lx_next(lx);
    if(lx->cur.type==T_STRING) {
        strncpy(sequence,lx->cur.text,sizeof(sequence)-1);sequence[sizeof(sequence)-1]=0;
        lx_next(lx);
    } else if(lx->cur.type==T_IDENT && is_string_var_name(lx->cur.text)) {
        Variable *v=find_var(lx->cur.text);
        snprintf(sequence,sizeof(sequence),"%s",v && v->str?v->str:"");lx_next(lx);
    } else return music_error("expected quoted MML or string variable");
    if(lx->cur.type!=T_END)return music_error("unexpected trailing input");
    return mml(sequence);
}
