rem Mein kleines Testprogramm
const kV = 100
let x = 1 + kV
loop: print "X =" x,
let x = x + 1:let y=x*10
if x < 110 then goto loop
let Bvar = x * 10 + 11
print "B = "; Bvar
gosub foo
gosub foo2
print
let A$ = "0123456789"
print
print "END", x
erase buf1
if kV < 100 then print "kV < 100" else print "kV >= 100"
end


foo:
print "foo", y, y<12, not y>12
let A$ = "Welt"
print "Hallo", A$
DIM buf1(4)
let buf1(0) = 987654321
print buf1(0), "= 987654321"
cmd(12345)

return


foo2:
for i = 0 to 10
  print i,
next
return

on_can:
let p1 = stack()
print "on_can", p1
return