10 REM PRINT USING regression - fixed width numeric pictures
20 PRINT "START PrUsing"
30 PRINT USING "###"; 42
40 PRINT USING "###.##"; 12.5
50 PRINT USING "0000"; 42
60 PRINT USING "####"; -42
70 PRINT USING "##"; 1234
80 A=1.2
90 PRINT USING "###.##"; A, 3.4
100 PRINT "END PrUsing"
