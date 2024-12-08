10 DIM A(10)

20 FOR i = 0 to 100
30   PRINT i,
40 next i
50 set1(A, 0, 1)
60 set1(A, 1, 2)
70 set1(A, 2, 3)
80 set1(A, 3, 4)
90 cmd(67890)

100 PRINT "get1(A, 0) =", get1(A, 0)
110 PRINT "get1(A, 1) =", get1(A, 1)
120 PRINT "get1(A, 2) =", get1(A, 2)
130 PRINT "get1(A, 3) =", get1(A, 3)
140 goto 300

190 rem on_can
200 let pa1 = stack()
205 PRINT
210 PRINT "on_can", pa1
220 PRINT get1(A, 0), get1(A, 1), get1(A, 2), get1(A, 3)
230 return

300 PRINT "END"
310 end