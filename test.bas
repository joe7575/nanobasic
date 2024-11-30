rem Mein kleines Testprogramm
let x = 1
loop: print "X = "; x,
let x = x + 1;let y=x*10
if x < 10 then goto loop
let Bvar = x * 10 + 11
print "B = "; Bvar
gosub foo
gosub foo2
print
let A$ = "0123456789"
print left$(A$, 3), "= 012"
print right$(A$, 3), "= 789"
print mid$(A$, 4, 3), "= 456"
print len(A$), "= 10"
print val("123"), "= 123"
print str$(123), "= 123"
print
print "END", x
end


foo:
print "foo", y, y<12, not y>12
let A$ = "Welt"
print "Hallo", A$
let B$ = A$ + " lasse dich grüßen!"
print B$
print left$(B$, 10)
let buf1[0] = 1
let buf1[1,1] = 2
let buf2[0] = 17
let buf2[1,1] = 18
let buf1[2,2] = 3 + 4 * 256
let buf2[2,2] = 19 + 20 * 256
let buf1[4,4] = 5 + 6 * 256 + 7 * 65536 + 8 * 16777216
let buf2[4,4] = 21 + 22 * 256 + 23 * 65536 + 24 * 16777216
cmd(12345)
event(666777, 0)

print buf1[0], buf1[1], buf2[0], buf2[1]
print buf1[2,2], buf2[2,2]
print buf1[4,4], buf2[4,4]
return


foo2:
for i = 0 to 10
  print i,
next i
return