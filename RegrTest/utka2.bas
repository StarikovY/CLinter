10 REM UTKA (DUCK) - old Basic game for LINTER 
20 REM written by Yuri STarikov and 
30 REM students of Moscow school #167 
40 REM First release: 1987 
50 SCREEN 0 
60 CLS 
120 RANDOMIZE 
130 SHUTY = 0 
140 LOCATE 40 , 24 
150 PRINT "I" 
160 Y = INT ( RND ( 23 ) * 22 + 1 ) 
170 X = 75 
180 SPEED = RND ( 1 ) * 0.5 
190 LOCATE X , Y : PRINT STR$(X) + "DUCK "
220 rem INP$ = INKEY$ 
230 rem IF INP$ = "x" OR INP$ = "X" THEN GOTO 350 
240 rem IF INP$ = "i" OR INP$ = "I" THEN GOTO 320 
250 PAUSE ( SPEED ) 
260 X = X - 1 
280 IF X >= 1 THEN GOTO 190
290 GOTO 60
320 REM End
350 REM End
