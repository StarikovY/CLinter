10 REM ==== STRING REGRESSION TESTS ====
20 EPS = 1E-6
30 PRINT "STRING TESTS"
40 GOTO 200
200 REM ---- Basic concatenation ----
210 N$="CONCAT":  E$="HELLO"+"WORLD": G$="HELLO"+"WORLD": GOSUB 9000
220 N$="CHR$":    E$=CHR$(65):        G$=CHR$(65):        GOSUB 9000
230 N$="ASC":     EN=65:              GN=ASC("A"):        GOSUB 9100
300 REM ---- LEN ----
310 N$="LEN5":    EN=5:               GN=LEN("HELLO"):    GOSUB 9100
320 N$="LEN0":    EN=0:               GN=LEN(""):         GOSUB 9100
400 REM ---- LEFT$, RIGHT$, MID$ ----
410 N$="LEFT$":   E$="HEL":           G$=LEFT$("HELLO",3): GOSUB 9000
420 N$="RIGHT$":  E$="LLO":           G$=RIGHT$("HELLO",3): GOSUB 9000
430 N$="MID$":    E$="ELL":           G$=MID$("HELLO",2,3): GOSUB 9000
440 N$="MID$2":   E$="LO":            G$=MID$("HELLO",4):   GOSUB 9000
500 REM ---- INSTR ----
510 N$="INSTR1":  EN=3:               GN=INSTR("ABCDE","CD"): GOSUB 9100
520 N$="INSTR0":  EN=0:               GN=INSTR("ABCDE","ZZ"): GOSUB 9100
600 REM ---- SEG$, TRM$ ----
610 N$="SEG$":    E$="CD":            G$=SEG$("ABCDE",3,2): GOSUB 9000
620 N$="TRM$":    E$="ABC":           G$=TRM$("  ABC  "):   GOSUB 9000
700 REM ---- POS(), TAB() output ----
710 PRINT "POS test start";: X=POS(): PRINT "(POS=";X;")"
720 PRINT "TAB test A"; TAB(20); "Привет, Мир!"
800 PRINT: PRINT "DONE."
810 END
9000 REM ---- CHECK(name$, expected$, got$) ----
9010 PRINT "TEST:"; N$; " EXP=["; E$; "] GOT=["; G$; "]";
9020 IF E$ = G$ THEN PRINT "PASS" ELSE PRINT "FAIL"
9030 RETURN
9100 REM ---- CHECKN(name$, expected#, got#) ----
9110 D = ABS(GN - EN)
9120 PRINT "TEST:"; N$; " EXP="; EN; " GOT="; GN; " DIFF="; D;
9130 IF D <= EPS THEN PRINT "PASS" ELSE PRINT "FAIL"
9140 RETURN
