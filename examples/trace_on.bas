10 tron
20 enabled = 1
30 if enabled=1 then
40   gosub 60000
50   enabled=0
60 elseif enabled=0 then
70   gosub 60000
80   enabled=1
90 endif
100 print cnt
110 for i=1 to 5
120   cnt = cnt + 1
130 next i
140 print cnt
150 while cnt > 0
160   cnt = cnt - 1
170 loop
180 print cnt
190 sleep(1)
200 goto 30
210 :
60000 print enabled
60010 return
