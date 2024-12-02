10 dim A(10)

20 for i = 0 to 100
30   print i,
40 next i
50 set1(A, 0, 1)
60 set1(A, 1, 2)
70 set1(A, 2, 3)
80 set1(A, 3, 4)
90 cmd(67890)

100 print "get1(A, 0) =", get1(A, 0)
110 print "get1(A, 1) =", get1(A, 1)
120 print "get1(A, 2) =", get1(A, 2)
130 print "get1(A, 3) =", get1(A, 3)
140 goto 300

190 rem on_can
200 let pa1 = stack()
205 print
210 print "on_can", pa1
220 print get1(A, 0), get1(A, 1), get1(A, 2), get1(A, 3)
230 return

300 print "END"
310 end