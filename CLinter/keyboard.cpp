/* Nonblocking console keyboard input; classic C, macOS/Linux/Windows. */
#include "keyboard.h"
#include "graphics.h"
#include <stdio.h>
#include <math.h>
#if defined(_WIN32)
#include <conio.h>
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <errno.h>
#include <time.h>
#endif
double clinter_clock(void) {
#if defined(_WIN32)
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    if (!frequency.QuadPart) QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#endif
}
int clinter_inkey(void) {
    int game_key=graphics_read_key();
    if(game_key)return game_key;
    /* Active SDL window owns the keyboard: never steal console keystrokes. */
    if(graphics_active())return 0;
#if defined(_WIN32)
    if (!_isatty(_fileno(stdin))) return 0;
    if (!_kbhit()) return 0;
    { int ch = _getch();
      /* Extended keys are two-byte sequences; do not leak scan codes as text. */
      if (ch == 0 || ch == 224) { if (_kbhit()) (void)_getch(); return 0; }
      return ch == 3 ? 0 : ch; }
#else
    struct termios original, raw;
    fd_set set;
    struct timeval timeout = {0, 0};
    unsigned char ch;
    int ready, result = 0;
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &original) != 0) return 0;
    raw = original;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_iflag &= (tcflag_t)~(IXON | ICRNL);
    raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) return 0;
    FD_ZERO(&set); FD_SET(STDIN_FILENO, &set);
    ready = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
    if (ready > 0 && read(STDIN_FILENO, &ch, 1) == 1) result = (int)ch;
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &original);
    /* Escape-prefixed special keys are not yet mapped. */
    if (result == 27 || result == 3) return 0;
    return result;
#endif
}
int clinter_pause(double seconds) {
    if (!isfinite(seconds) || seconds < 0.0 || seconds > 3600.0) {
        puts("ERROR: PAUSE duration out of range"); return -1;
    }
#if defined(_WIN32)
    Sleep((DWORD)(seconds * 1000.0));
#else
    struct timespec req, rem;
    req.tv_sec = (time_t)seconds;
    req.tv_nsec = (long)((seconds - (double)req.tv_sec) * 1000000000.0);
    while (nanosleep(&req, &rem) == -1 && errno == EINTR) req = rem;
#endif
    return 0;
}
