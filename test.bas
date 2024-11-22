rem Mein kleines Testprogramm
let x = 1
loop: print "X =", x
let x = x + 1
if x < 10 then goto loop
let Bvar = x * 10 + 11
print "B =", Bvar
gosub foo
print "END", x
end


foo:
print "foo"
return

