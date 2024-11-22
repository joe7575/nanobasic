LET x = 1
loop: PRINT "X =", x
LET x = x + 1
IF x < 10 THEN GOTO loop
PRINT "END", x
END
