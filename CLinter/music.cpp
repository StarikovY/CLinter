/* MSX BASIC PLAY: up to three concurrent MML voices, SDL2 queued PCM audio. */
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
#define MUSIC_RATE 22050
#define MUSIC_MAX_MS 120000
#define MUSIC_MAX_EVENTS 2048
typedef struct { double hz; int start, end, gate_end, volume; } MusicEvent;
typedef struct { MusicEvent events[MUSIC_MAX_EVENTS]; int count, total; } MusicVoice;
static int music_error(const char *msg) { fprintf(stderr,"ERROR: PLAY %s\n",msg); return -1; }
static int digits(const char **s,int fallback) {
    int n=0,any=0;
    while(isdigit((unsigned char)**s)) {any=1;n=n*10+(*(*s)++-'0');if(n>10000)break;}
    return any?n:fallback;
}
static int parse_voice(const char *s, MusicVoice *voice) {
    int octave=4,length=4,tempo=120,volume=12,gate=8;
    static const int semitones[7]={9,11,0,2,4,5,7};
    memset(voice,0,sizeof(*voice));
    while(*s) {
        char c=(char)toupper((unsigned char)*s++);
        int n,acc=0,dots=0,duration;
        double freq=0,beats;
        if(isspace((unsigned char)c))continue;
        if(c=='O') {octave=digits(&s,-1);if(octave<1||octave>8)return music_error("octave out of range");continue;}
        if(c=='L') {length=digits(&s,-1);if(length<1||length>64)return music_error("length out of range");continue;}
        if(c=='T') {tempo=digits(&s,-1);if(tempo<32||tempo>255)return music_error("tempo out of range");continue;}
        if(c=='V') {volume=digits(&s,-1);if(volume<0||volume>15)return music_error("volume out of range");continue;}
        if(c=='Q') {gate=digits(&s,-1);if(gate<1||gate>8)return music_error("gate out of range");continue;}
        if(c=='>' || c=='<') {octave+=(c=='>')?1:-1;if(octave<1||octave>8)return music_error("octave out of range");continue;}
        if((c<'A'||c>'G')&&c!='R'&&c!='N')return music_error("unsupported MML command");
        if(c=='N') {
            int note=digits(&s,-1);
            if(note<0||note>96)return music_error("numeric note out of range");
            n=length;
            beats=4.0/n;duration=(int)(60000.0/tempo*beats);
            if(duration<1)duration=1;
            if(voice->total+duration>MUSIC_MAX_MS)return music_error("sequence too long");
            if(voice->count>=MUSIC_MAX_EVENTS)return music_error("too many notes");
            freq=note?440.0*pow(2.0,(note-46)/12.0):0;
            voice->events[voice->count].hz=freq;
            voice->events[voice->count].start=voice->total;
            voice->events[voice->count].end=voice->total+duration;
            voice->events[voice->count].gate_end=voice->total+duration*gate/8;
            voice->events[voice->count].volume=volume;
            voice->count++;voice->total+=duration;continue;
        }
        if(c!='R' && (*s=='#'||*s=='+')) {acc=1;s++;}
        else if(c!='R' && *s=='-'){acc=-1;s++;}
        n=digits(&s,length);if(n<1||n>64)return music_error("note length out of range");
        while(*s=='.') {dots++;s++;if(dots>3)return music_error("too many dots");}
        beats=4.0/n;{double add=beats/2;int i;for(i=0;i<dots;i++){beats+=add;add/=2;}}
        duration=(int)(60000.0/tempo*beats);
        if(duration<1)duration=1;
        if(voice->total+duration>MUSIC_MAX_MS)return music_error("sequence too long");
        if(voice->count>=MUSIC_MAX_EVENTS)return music_error("too many notes");
        if(c!='R') {
            int midi=(octave+1)*12+semitones[c-'A']+acc;
            freq=440.0*pow(2.0,(midi-69)/12.0);
        }
        voice->events[voice->count].hz=freq;
        voice->events[voice->count].start=voice->total;
        voice->events[voice->count].end=voice->total+duration;
        voice->events[voice->count].gate_end=voice->total+duration*gate/8;
        voice->events[voice->count].volume=volume;
        voice->count++;
        voice->total+=duration;
    }
    return 0;
}
#ifdef CLINTER_HAVE_SDL2
static SDL_AudioDeviceID device;
static unsigned int background_total_bytes;
static int background_started;
static int music_progress_percent(void) {
    unsigned int queued;
    if (!background_started || !background_total_bytes) return 100;
    queued=SDL_GetQueuedAudioSize(device);
    if (queued==0) {background_started=0;return 100;}
    if (queued>background_total_bytes) queued=background_total_bytes;
    return (int)(((unsigned long long)(background_total_bytes-queued)*100)/background_total_bytes);
}
static void music_stop_background(void) {
    if(device)SDL_ClearQueuedAudio(device);
    background_started=0; background_total_bytes=0;
}

static int audio_open(void) {
    SDL_AudioSpec want,got;
    if(device)return 1;
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)return 0;
    SDL_zero(want);want.freq=MUSIC_RATE;want.format=AUDIO_S16SYS;want.channels=1;want.samples=1024;
    device=SDL_OpenAudioDevice(NULL,0,&want,&got,0);
    if(!device || got.freq!=MUSIC_RATE || got.format!=AUDIO_S16SYS || got.channels!=1)return 0;
    SDL_PauseAudioDevice(device,0);return 1;
}
static void render_voices(MusicVoice *voices,int count,int background) {
    int total=0,i,position=0,indices[3]={0,0,0};
    double phase[3]={0,0,0};
    short buffer[1024];
    if(getenv("CLINTER_GRAPHICS_HEADLESS"))return;
    for(i=0;i<count;i++)if(voices[i].total>total)total=voices[i].total;
    if(!total||!audio_open())return;
    music_stop_background();
    while(position<(int)((long long)total*MUSIC_RATE/1000)) {
        int n=1024,j;
        int remaining=(int)((long long)total*MUSIC_RATE/1000)-position;
        if(n>remaining)n=remaining;
        for(j=0;j<n;j++) {
            int v,ms=(int)((long long)(position+j)*1000/MUSIC_RATE);
            double mix=0;
            for(v=0;v<count;v++) {
                MusicVoice *voice=&voices[v];MusicEvent *e;double envelope=1;
                while(indices[v]<voice->count && ms>=voice->events[indices[v]].end)indices[v]++;
                if(indices[v]>=voice->count)continue;
                e=&voice->events[indices[v]];
                if(ms<e->start||ms>=e->gate_end||e->hz<=0||e->volume==0)continue;
                if(e->gate_end-ms<15)envelope=(e->gate_end-ms)/15.0;
                mix+=(fmod(phase[v],1.0)<0.5?1.0:-1.0)*(e->volume/15.0)*envelope;
                phase[v]+=e->hz/MUSIC_RATE;
            }
            mix=mix/count;
            if(mix>1)mix=1;if(mix< -1)mix=-1;
            buffer[j]=(short)(mix*12000);
        }
        if(SDL_QueueAudio(device,buffer,(unsigned int)(n*sizeof(short)))!=0)break;
        position+=n;
    }
    if(background) {
        background_total_bytes=SDL_GetQueuedAudioSize(device);
        background_started=background_total_bytes>0;
    } else while(SDL_GetQueuedAudioSize(device)>0)SDL_Delay(5);
}
#else
static int music_progress_percent(void) {return 100;}
static void music_stop_background(void) {}
static void render_voices(MusicVoice *voices,int count,int background) {(void)voices;(void)count;(void)background;}
#endif
static int play_event_enabled,play_event_target,play_event_pending;
int music_statement(Lexer *lx) {
    if(lx->cur.type==T_IDENT || lx->cur.type==T_STOP) {
        Lexer look=*lx;
        lx_next(&look);
        if(look.cur.type==T_ON || look.cur.type==T_ONKW || look.cur.type==T_OFF || look.cur.type==T_STOP)
            return music_play_control(lx);
    }
    char sequences[3][512];
    MusicVoice *voices;
    int count=0,i,result=0,background=0;
    lx_next(lx);
    /* PLAY PROGRESS variable: 0..100, based on queued audio actually consumed.
       PLAY STOP: cancel background playback immediately. */
    if(lx->cur.type==T_IDENT || lx->cur.type==T_STOP) {
        Lexer look=*lx;
        if(look.cur.type==T_STOP || !_stricmp(look.cur.text,"STOP")) {
            lx_next(&look);
            if(look.cur.type!=T_END)return music_error("STOP syntax");
            music_stop_background();return 0;
        }
        /* M4: report background playback state without blocking graphics. */
        if(!_stricmp(look.cur.text,"ACTIVE")) {
            Variable *v;
            lx_next(&look);
            if(look.cur.type!=T_IDENT || is_string_var_name(look.cur.text))return music_error("ACTIVE expects numeric variable");
            v=ensure_var(look.cur.text,0);
            if(!v)return music_error("variable limit");
            lx_next(&look);
            if(look.cur.type!=T_END)return music_error("ACTIVE syntax");
            v->num=music_progress_percent()<100?1.0:0.0;
            return 0;
        }
        if(!_stricmp(look.cur.text,"PROGRESS")) {
            Variable *v;
            lx_next(&look);
            if(look.cur.type!=T_IDENT || strchr(look.cur.text,'$'))return music_error("PROGRESS expects numeric variable");
            v=ensure_var(look.cur.text,0);
            if(!v)return music_error("variable limit");
            lx_next(&look);
            if(look.cur.type!=T_END)return music_error("PROGRESS syntax");
            v->num=(double)music_progress_percent();return 0;
        }
    }
    /* PLAY BACKGROUND <voices>: queue music without blocking graphics. */
    if(lx->cur.type==T_IDENT && !_stricmp(lx->cur.text,"BACKGROUND")) {background=1;lx_next(lx);}
    for(;;) {
        if(count>=3)return music_error("maximum three voices");
        if(lx->cur.type==T_STRING) {
            snprintf(sequences[count],sizeof(sequences[count]),"%s",lx->cur.text);
            lx_next(lx);
        } else if(lx->cur.type==T_IDENT && is_string_var_name(lx->cur.text)) {
            Variable *v=find_var(lx->cur.text);
            snprintf(sequences[count],sizeof(sequences[count]),"%s",v && v->str?v->str:"");
            lx_next(lx);
        } else return music_error("expected quoted MML or string variable");
        count++;
        if(lx->cur.type!=T_COMMA)break;
        lx_next(lx);
    }
    if(lx->cur.type!=T_END)return music_error("unexpected trailing input");
    voices=(MusicVoice*)calloc((size_t)count,sizeof(*voices));
    if(!voices)return music_error("out of memory");
    for(i=0;i<count;i++)if(parse_voice(sequences[i],&voices[i])!=0){result=-1;break;}
    if(!result) {render_voices(voices,count,background);if(!background && play_event_enabled && play_event_target)play_event_pending=play_event_target;}
    free(voices);
    return result;
}
/* M3 completion notifications: PLAY currently renders synchronously. */
int music_take_play_event(void) {int n=play_event_pending;play_event_pending=0;return n;}
int music_set_play_target(int n) {if(n<1||n>65535)return music_error("event line range");play_event_target=n;play_event_pending=0;return 0;}
int music_play_control(Lexer *lx) {
    int mode;
    lx_next(lx);
    if(lx->cur.type==T_ON || lx->cur.type==T_ONKW)mode=1;
    else if(lx->cur.type==T_OFF)mode=0;
    else if(lx->cur.type==T_STOP)mode=2;
    else return music_error("event control");
    lx_next(lx);
    if(lx->cur.type!=T_END)return music_error("event control syntax");
    if(mode==0) {play_event_enabled=0;play_event_pending=0;}
    else if(mode==1)play_event_enabled=1;
    else play_event_enabled=0;
    return 0;
}
