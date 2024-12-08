5 tron
10 let i = 0: gosub 100
20 let i = 1: gosub 100
30 let i = 2: gosub 100
40 let i = 4: gosub 100
50 end

100 on i goto 120, 140, 160
110 print "next"
115 return

120 print 120
130 goto 115
140 print 140
150 goto 115
160 print 160
170 goto 115


