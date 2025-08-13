#ifndef PRINTFUNC_H
#define PRINTFUNC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "runtime.h"
#include "parse.h"

/* Execute a PRINT statement.
   Precondition: lx->cur.type == T_PRINT
   Postcondition: tokens for this statement are consumed.
   Returns 0 on success, -1 on error. */
int exec_print(Lexer* lx);


#ifdef __cplusplus
}
#endif

#endif /* PRINTFUNC_H */
