#include "basic_repl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if defined(_WIN32)
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <termios.h>
#endif
#define HIST_CAP 100
static char *entries[HIST_CAP];
static unsigned long long serials[HIST_CAP];
static int count=0, first=0;
static unsigned long long next_serial=1;
static char *copy_string(const char *s) { size_t n=strlen(s)+1; char *p=(char*)malloc(n); if(p) memcpy(p,s,n); return p; }
static const char *entry_at(int i) { return entries[(first+i)%HIST_CAP]; }
void basic_history_clear(void) { for(int i=0;i<count;i++) free(entries[(first+i)%HIST_CAP]); count=first=0; /* serials remain monotonic */ }
void basic_history_add(const char *line) {
    if(!line || !*line) return;
    char *p=copy_string(line); if(!p) return;
    if(count==HIST_CAP) { free(entries[first]); first=(first+1)%HIST_CAP; count--; }
    int slot=(first+count)%HIST_CAP; entries[slot]=p; serials[slot]=next_serial++; count++;
}
void basic_history_print(int limit) {
    int start=0; if(limit>=0 && limit<count) start=count-limit;
    for(int i=start;i<count;i++) { int j=(first+i)%HIST_CAP; printf("%5llu  %s\n",serials[j],entries[j]); }
}
int basic_history_expand(const char *input,char *out,size_t capacity) {
    if(!input || input[0]!='!' || !isdigit((unsigned char)input[1])) return 0;
    char *end; unsigned long long id=strtoull(input+1,&end,10);
    while(*end==' ' || *end=='\t') end++;
    if(*end) return 0;
    for(int i=0;i<count;i++) { int j=(first+i)%HIST_CAP; if(serials[j]==id) {
        size_t n=strlen(entries[j]); if(n>=capacity) return 0;
        memcpy(out,entries[j],n+1); return 1;
    }} return 0;
}
static void redraw(const char *prompt,const char *buf,size_t len,size_t cursor) {
    printf("\r\033[2K%s%s",prompt,buf);
    if(len>cursor) printf("\033[%zuD",len-cursor);
    fflush(stdout);
}
#if defined(_WIN32)
static int tty_input(void) { return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout)); }
#else
static int tty_input(void) { return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO); }
#endif
int basic_readline(const char *prompt,char *out,size_t capacity) {
    if(capacity<2) return 0;
    if(!tty_input()) { printf("%s",prompt); fflush(stdout); if(!fgets(out,(int)capacity,stdin)) return 0; return 1; }
#if defined(_WIN32)
    HANDLE h=GetStdHandle(STD_INPUT_HANDLE); DWORD oldmode;
    if(!GetConsoleMode(h,&oldmode)) { printf("%s",prompt); fflush(stdout); return fgets(out,(int)capacity,stdin)!=NULL; }
    SetConsoleMode(h,oldmode & ~(ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT));
#else
    struct termios oldmode,raw;
    if(tcgetattr(STDIN_FILENO,&oldmode)!=0) return 0;
    raw=oldmode; raw.c_lflag &= ~(ICANON|ECHO|ISIG); raw.c_iflag &= ~(IXON|ICRNL); raw.c_cc[VMIN]=1; raw.c_cc[VTIME]=0;
    if(tcsetattr(STDIN_FILENO,TCSANOW,&raw)!=0) return 0;
#endif
    size_t len=0,pos=0; int nav=count; char draft[4096]; draft[0]=0; out[0]=0;
    printf("%s",prompt); fflush(stdout);
    int result=1;
    for(;;) {
        int key=0;
#if defined(_WIN32)
        INPUT_RECORD rec; DWORD nr=0;
        if(!ReadConsoleInputA(h,&rec,1,&nr)) {result=0;break;}
        if(rec.EventType!=KEY_EVENT || !rec.Event.KeyEvent.bKeyDown) continue;
        WORD vk=rec.Event.KeyEvent.wVirtualKeyCode;
        if(vk==VK_LEFT) key=1001; else if(vk==VK_RIGHT) key=1002;
        else if(vk==VK_UP) key=1003; else if(vk==VK_DOWN) key=1004;
        else if(vk==VK_HOME) key=1005; else if(vk==VK_END) key=1006;
        else if(vk==VK_DELETE) key=1007; else key=(unsigned char)rec.Event.KeyEvent.uChar.AsciiChar;
#else
        unsigned char c; if(read(STDIN_FILENO,&c,1)!=1) {result=0;break;} key=c;
        if(c==27) {
            unsigned char seq[3];
            /* Escape sequence recognition, including arrows and Home/End/Delete. */
            if(read(STDIN_FILENO,&seq[0],1)!=1) continue;
            if(seq[0]!='[' && seq[0]!='O') continue;
            if(read(STDIN_FILENO,&seq[1],1)!=1) continue;
            switch(seq[1]) { case 'A':key=1003;break;case 'B':key=1004;break;case 'C':key=1002;break;case 'D':key=1001;break;case 'H':key=1005;break;case 'F':key=1006;break;
                case '1':case '3':case '4':case '7':case '8': if(read(STDIN_FILENO,&seq[2],1)==1 && seq[2]=='~') {key=(seq[1]=='3')?1007:((seq[1]=='1'||seq[1]=='7')?1005:1006);} break;
            }
        }
#endif
        if(key=='\r'||key=='\n') {printf("\r\n");break;}
        if(key==3) {printf("^C\r\n");out[0]=0;result=-1;break;}
        if(key==4 && len==0) {printf("\r\n");result=0;break;}
        if(key==1||key==1005) pos=0;
        else if(key==5||key==1006) pos=len;
        else if(key==1001 && pos) pos--;
        else if(key==1002 && pos<len) pos++;
        else if(key==21) {len=pos=0;out[0]=0;}
        else if((key==8||key==127) && pos) {memmove(out+pos-1,out+pos,len-pos+1);len--;pos--;}
        else if(key==1007 && pos<len) {memmove(out+pos,out+pos+1,len-pos);len--;}
        else if(key==1003 && count) {
            if(nav==count) {strncpy(draft,out,sizeof(draft)-1);draft[sizeof(draft)-1]=0;}
            if(nav>0) nav--;
            strncpy(out,entry_at(nav),capacity-1);out[capacity-1]=0;len=pos=strlen(out);
        } else if(key==1004 && nav<count) {
            nav++; const char *s=(nav==count)?draft:entry_at(nav);
            strncpy(out,s,capacity-1);out[capacity-1]=0;len=pos=strlen(out);
        } else if(key>=32 && key<=255 && len+1<capacity) {
            memmove(out+pos+1,out+pos,len-pos+1);out[pos]=(char)key;pos++;len++;
        }
        redraw(prompt,out,len,pos);
    }
#if defined(_WIN32)
    SetConsoleMode(h,oldmode);
#else
    tcsetattr(STDIN_FILENO,TCSANOW,&oldmode);
#endif
    return result;
}
