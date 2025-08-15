10 REM ==== MATH REGRESSION TESTS ====
20 EPS = 1E-6
30 PRINT "MATH TESTS (EPS=";EPS;")"
40 GOTO 200
200 REM ---- Operators (+ - * / ^) ----
210 N$="ADD":      E=5:       G=2+3:         GOSUB 9000
220 N$="SUB":      E=-1:      G=2-3:         GOSUB 9000
230 N$="MUL":      E=6:       G=2*3:         GOSUB 9000
240 N$="DIV":      E=2:       G=6/3:         GOSUB 9000
250 N$="POW(^)":   E=8:       G=2^3:         GOSUB 9000
260 N$="POW()":    E=8:       G=POW(2,3):    GOSUB 9000
300 REM ---- Integer ops ----
310 N$="IDIV":     E=2:       G=IDIV(7,3):   GOSUB 9000
320 N$="MOD":      E=1:       G=MOD(7,3):    GOSUB 9000
330 N$="INT+":     E=3:       G=INT(3.7):    GOSUB 9000
340 N$="INT-":     E=-4:      G=INT(-3.2):   GOSUB 9000
350 N$="SGN-":     E=-1:      G=SGN(-2):     GOSUB 9000
360 N$="SGN0":     E=0:       G=SGN(0):      GOSUB 9000
370 N$="SGN+":     E=1:       G=SGN(5):      GOSUB 9000
400 REM ---- Trig (radians) ----
410 N$="SIN0":     E=0:       G=SIN(0):            GOSUB 9000
420 N$="COS0":     E=1:       G=COS(0):            GOSUB 9000
430 N$="SIN(PI/2)":E=1:       G=SIN(PI/2):         GOSUB 9000
440 N$="COS(PI)":  E=-1:      G=COS(PI):           GOSUB 9000
450 N$="TAN(PI/4)":E=1:       G=TAN(PI/4):         GOSUB 9000
460 N$="ATN(1)":   E=PI/4:    G=ATN(1):            GOSUB 9000
500 REM ---- Exponentials & logs ----
510 N$="EXP0":     E=1:       G=EXP(0):            GOSUB 9000
520 N$="LOGE":     E=1:       G=LOG(EXP(1)):       GOSUB 9000
530 N$="LOG1":     E=0:       G=LOG(1):            GOSUB 9000
540 N$="LOG10(1000)":E=3:     G=LOG10(1000):       GOSUB 9000
600 REM ---- Roots & abs ----
610 N$="SQR9":     E=3:       G=SQR(9):            GOSUB 9000
620 N$="ABS-5":    E=5:       G=ABS(-5):           GOSUB 9000
700 REM ---- Cross-checks ----
710 N$="POWvs^":   E=2^5:     G=POW(2,5):          GOSUB 9000
720 N$="EXPLOG":   E=5:       G=EXP(LOG(5)):       GOSUB 9000
730 N$="SINSQ+COSQ":E=1:      G=SIN(0.7)^2 + COS(0.7)^2: GOSUB 9000
800 PRINT : PRINT "DONE."
810 END
9000 REM ---- CHECK(name$, expected, got) ----
9010 D = ABS(G - E)
9020 PRINT "TEST:"; N$; " EXP="; E; " GOT="; G; " DIFF="; D;
9030 IF D <= EPS THEN PRINT "  PASS" ELSE PRINT "  FAIL"
9040 RETURN
