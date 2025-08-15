10 REM ==== ARRAY REGRESSION TESTS (0-BASED) ====
20 EPS = 1E-6
30 PRINT "ARRAY TESTS (EPS=";EPS;")"
40 GOTO 200

9000 REM ---- CHECK$(name$, expected$, got$) ----
9010 PRINT "TEST:"; N$; " EXP=["; E$; "] GOT=["; G$; "]";
9020 IF E$ = G$ THEN PRINT "PASS" ELSE PRINT "FAIL"
9030 RETURN

9100 REM ---- CHECKN(name$, expected#, got#) ----
9110 D = ABS(GN - EN)
9120 PRINT "TEST:"; N$; " EXP="; EN; " GOT="; GN; " DIFF="; D;
9130 IF D <= EPS THEN PRINT "PASS" ELSE PRINT "FAIL"
9140 RETURN


200 REM ===== NUMERIC ARRAYS =====
210 DIM A(3)
220 FOR I=0 TO 2: A(I) = (I+1)*2: NEXT I
230 N$="A(1)":      EN=4:      GN=A(1):                         GOSUB 9100
240 N$="A SUM":     EN=12:     GN=A(0)+A(1)+A(2):               GOSUB 9100

250 DIM B(2,3)
260 FOR I=0 TO 1: FOR J=0 TO 2: B(I,J)=(I+1)*10+(J+1): NEXT J: NEXT I
270 N$="B(1,2)":    EN=23:     GN=B(1,2):                       GOSUB 9100
280 GN=0: FOR I=0 TO 1: FOR J=0 TO 2: GN=GN+B(I,J): NEXT J: NEXT I
290 N$="B SUM":     EN=102:                                      GOSUB 9100

300 DIM C(2,2,2)
310 FOR I=0 TO 1: FOR J=0 TO 1: FOR K=0 TO 1: C(I,J,K)=100*(I+1)+10*(J+1)+(K+1): NEXT K: NEXT J: NEXT I
320 N$="C(1,0,1)":  EN=212:    GN=C(1,0,1):                     GOSUB 9100
330 GN=0: FOR I=0 TO 1: FOR J=0 TO 1: FOR K=0 TO 1: GN=GN+C(I,J,K): NEXT K: NEXT J: NEXT I
340 N$="C SUM":     EN=(111+112+121+122+211+212+221+222):       GOSUB 9100

350 REM arithmetic using array elements (0-based)
360 N$="A(2)*B(0,1)": EN=(3*2)*(10*1+2):  GN=A(2)*B(0,1):       GOSUB 9100
370 N$="C expr":      EN=122 - 2 + 21:    GN=C(0,1,1)-A(0)+B(1,0): GOSUB 9100


400 REM ===== STRING ARRAYS =====
410 DIM S$(3)
420 S$(0)="HEL": S$(1)="LO": S$(2)="!"
430 N$="S$ CONCAT": E$="HELLO!": G$=S$(0)+S$(1)+S$(2):          GOSUB 9000
440 N$="INSTR S$":  EN=4:         GN=INSTR(S$(0)+S$(1),"LO"):   GOSUB 9100

450 DIM T$(2,2)
460 FOR I=0 TO 1: FOR J=0 TO 1: T$(I,J)="I"+STR$(I+1)+"J"+STR$(J+1): NEXT J: NEXT I
470 N$="T$(1,0)":   E$="I2J1":    G$=T$(1,0):                   GOSUB 9000
480 N$="SEG$ T$":   E$="J2":      G$=SEG$(T$(0,1),3,2):         GOSUB 9000
490 N$="TRM$":      E$="ABC":     G$=TRM$("  ABC  "):           GOSUB 9000

500 REM mix arrays with math/string funcs
510 N$="STR$+A(1)": E$="4":       G$=STR$(A(1)):                GOSUB 9000
520 N$="CHR$":      E$="B":       G$=CHR$(66):                  GOSUB 9000

800 PRINT: PRINT "DONE."
810 END
