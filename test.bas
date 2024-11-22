rem Mein kleines Testprogramm
let x = 1
loop: print "X =", x
let x = x + 1;let y=x*10
if x < 10 then goto loop
let Bvar = x * 10 + 11
print "B =", Bvar
gosub foo
print "END", x
end


foo:
print "foo", y, y<12, not y>12
let A$ = "Welt"
print "Hallo", A$
return

