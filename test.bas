rem Mein kleines Testprogramm
let x = 1
loop: print "X = "; x,
let x = x + 1;let y=x*10
if x < 10 then goto loop
let Bvar = x * 10 + 11
print "B = "; Bvar
gosub foo
print
print "END", x
end


foo:
print "foo", y, y<12, not y>12
let A$ = "Welt"
print "Hallo", A$
print "peek(1234) = "; peek(1234)
let rx[0] = 1
let rx[1,1] = 2
let tx[0] = 17
let tx[1,1] = 18
let rx[2,2] = 3 + 4 * 256
let tx[2,2] = 19 + 20 * 256
let rx[4,4] = 5 + 6 * 256 + 7 * 65536 + 8 * 16777216
let tx[4,4] = 21 + 22 * 256 + 23 * 65536 + 24 * 16777216

print rx[0], rx[1], tx[0], tx[1]
print rx[2,2], tx[2,2]
print rx[4,4], tx[4,4]
return

