rem Mein kleines Testprogramm
const kV = 100
let x = 1 + kV
loop: print "X = "; x,
let x = x + 1:let y=x*10
if x < 110 then goto loop
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
let B$ = A$ + " lasse dich gruessen!"
print B$
print left$(B$, 10)
let b$ = string$(16, 0)
set1(b$, 0, 1)
set1(b$, 1, 2)
set2(b$, 2, 540)
set4(b$, 4, 12345)
set4(b$, 12, 987654321)

let b2$ = string$(16, 0)
copy(b2$, 12, b$, 12, 4)
print get4(b2$, 12), "= 987654321"

return


myfunc2:
for i = 0 to 10
  print i,
next i
print
return

on_can:
let p1 = param()
rem print "on_can", p1
return

myfunc3:
print "RND ",
for i = 0 to 20
  print i, rnd(100),
next i
print
return