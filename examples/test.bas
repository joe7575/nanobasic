rem Mein kleines Testprogramm
tron
const kV = 100
let x = 1 + kV
loop: print "X = "; x,
let x = x + 1:let y=x*10
if x < 10 then goto loop
let Bvar = x * 10 + 11
print "B = "; Bvar
gosub myfunc
gosub myfunc2
gosub myfunc3
print
let A$ = "0123456789"
print left$(A$, 3), "= 012"
print right$(A$, 3), "= 789"
print mid$(A$, 4, 3), "= 456"
print len(A$), "= 10"
print val("123"), "= 123"
print str$(123), "= 123"
print chr$(65), "= 'A'"
print hex$(123), "= 7B"
print instr(1, A$, "345"), "= 4"
print instr(4, A$, "345"), "= 4"
print instr(10, A$, "9"), "= 10"
print
free()
print "END", x
end


myfunc:
print "myfunc", y, y<12, not y>12
let A$ = "Welt"
print "Hallo", A$
let B$ = A$ + " lasse dich grüßen!"
print B$
print left$(B$, 10)
DIM buf1(4)
set1(buf1, 0, 1)
set1(buf1, 1, 2)
set2(buf1, 2, 540)
set4(buf1, 4, 12345)
set4(buf1, 12, 987654321)

DIM buf2(4)
copy(buf2, 12, buf1, 12, 4)
print get4(buf2, 12), "= 987654321"

return


myfunc2:
for i = 0 to 10
  print i,
next i
print
return

on_can:
let p1 = stack()
rem print "on_can", p1
return

myfunc3:
print "RND ",
for i = 0 to 20
  print rnd(100),
next i
print
return