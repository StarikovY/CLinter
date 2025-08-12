
#ifndef RUNTIME_H
#define RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define MAX_LINE_LEN    512
#define MAX_PROG_LINES  2000
#define MAX_VARS        1024
#define MAX_STACK       256
#define MAX_FILES       16

typedef enum { VT_NUM=0, VT_STR=1 } VarType;

typedef struct { char name[32]; VarType type; double num; char *str; } Variable;
typedef struct { int line; char *text; } ProgLine;

/* Tokenizer/Parser shared types */
typedef enum {
    T_END=0, T_NUMBER, T_STRING, T_IDENT,
    T_LET, T_PRINT, T_INPUT, T_IF, T_THEN, T_GOTO, T_ENDKW, T_STOP, T_REM, T_RUN, T_NEW, T_LIST, T_SAVE, T_LOAD,
    T_SAVEVARS, T_LOADVARS,
    T_FOR, T_TO, T_STEP, T_NEXT, T_GOSUB, T_RETURN,
    T_OPEN, T_CLOSE, T_AS, T_APPEND, T_OUTPUTKW,
    T_HASH, T_LINE, T_DIM,
    T_EQ, T_NE, T_LT, T_GT, T_LE, T_GE,
    T_PLUS, T_MINUS, T_STAR, T_SLASH, T_LPAREN, T_RPAREN, T_COMMA, T_SEMI, T_POWOP,
    T_DATA, T_READ, T_RESTORE, T_TRACE, T_ON, T_OFF, T_DUMP, T_VARS, T_ARRAYS, T_STACK, T_QUIT, T_RENUM, T_BYE
} TokType;

/* +++ ARRAYS +++ */
#define MAX_ARRAYS 128
#define MAX_DIMS   10

typedef struct {
    char  name[32];
    int   ndims;
    int   dims[MAX_DIMS];
    double* data;            /* row-major, zero-based */
} Array;

extern Array g_arrays[MAX_ARRAYS];
extern int   g_array_count;

/* array helpers (implemented in main.cpp) */
Array* array_find(const char* name);
Array* array_dim(const char* name, int ndims, int* dims);
int    array_index(Array* a, int* subs, int nsubs);     /* -1 on OOB */
double array_get(Array* a, int* subs, int nsubs);
void   array_set(Array* a, int* subs, int nsubs, double val);
void   arrays_clear(void);

/* +++ STRING ARRAYS +++ */
#define MAX_SARRAYS 128

typedef struct {
    char  name[32];
    int   ndims;
    int   dims[MAX_DIMS];
    char** data;             /* row-major array of char* (may be NULL) */
} SArray;

extern SArray g_sarrays[MAX_SARRAYS];
extern int    g_sarray_count;

extern int g_trace;


/* string array helpers (implemented in main.cpp) */
SArray* sarray_find(const char* name);
SArray* sarray_dim(const char* name, int ndims, int* dims);
int     sarray_index(SArray* a, int* subs, int nsubs);    /* -1 on OOB */
const char* sarray_get(SArray* a, int* subs, int nsubs);  /* never NULL, returns "" if unset */
void    sarray_set(SArray* a, int* subs, int nsubs, const char* val);
void    sarrays_clear(void);


typedef struct { TokType type; char text[128]; double number; } Token;
typedef struct { const char *s; size_t i; Token cur; } Lexer;

typedef struct { char var[32]; double end; double step; int afterForLine; int forLine; } ForFrame;
typedef struct { int used; FILE* fp; } FileSlot;

/* Globals (defined in main.c) */
extern ProgLine g_prog[MAX_PROG_LINES];
extern int g_prog_count;

extern Variable g_vars[MAX_VARS];
extern int g_var_count;

extern ForFrame g_for_stack[MAX_STACK];
extern int g_for_top;

extern int g_gosub_stack[MAX_STACK];
extern int g_gosub_top;

extern FileSlot g_files[MAX_FILES];

/* Utilities (main.c) */
char *strdup_c(const char *s);
char *trim(char *s);
void sort_program(void);
int  find_prog_index_by_line(int line);
int  next_line_number_after(int current);
Variable* find_var(const char *name);
Variable* ensure_var(const char *name, int isStr);
int  is_string_var_name(const char *name);
void prog_set_line(int line, const char *text);
void prog_clear(void);
void vars_clear(void);
void files_clear(void);
void cmd_list(int startGiven,int start,int endGiven,int end);


#ifdef __cplusplus
}
#endif
#endif
