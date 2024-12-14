1 REM NanoBasic example for a configuration with line numbers
2 REM and only core functionality
10 ' This is a comment
20 REM this is a comment too
30 ' Variable declaration
40 a = 10
50 let B$ = string$(10, 0)
60 dim Arr(10)
100 ' For loops
110 FOR i = 0 to a
120   PRINT i,
130 next i
140 print  ' newline
150 FOR j = 0 to 10 step 2
160   PRINT j,
170 next j
180 print
200 ' If statement
210 IF a = 10 THEN PRINT "a is 10" ELSE PRINT "a is not 10"
220 ' Gosub statement
230 gosub 300
240 ' For loop with data statement
250 FOR i = 1 to 8
260   read var1, var2
270   PRINT var1 "+" var2 "=" var1 + var2
280 next
290 end
300 ' Subroutine
310 PRINT "subroutine at line 300"
320 return
400 ' Data statement
410 DATA 1, 2, 3, 4, 5, 6, 7, 8
420 DATA 9, 10, 11, 12, 13, 14, 15, 16
