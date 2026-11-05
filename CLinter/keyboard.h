#ifndef CLINTER_KEYBOARD_H
#define CLINTER_KEYBOARD_H
#ifdef __cplusplus
extern "C" {
#endif
/* Nonblocking, one-byte key poll. Returns 0 when no key is ready. */
int clinter_inkey(void);
/* PAUSE expression in seconds, bounded and interruptible. */
int clinter_pause(double seconds);
#ifdef __cplusplus
}
#endif
#endif
