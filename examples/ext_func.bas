dim ExtBuf(10)
let A = 4
let B$ = "test"
efunc1(A, B$)

let C = efunc2()
print "C =" C

let D$ = efunc3()
print "D$ =" D$
end

efunc4:
let a = param()
let b$ = param$()
print "a =" a, "b$ =" b$, "ExtBuf =" ExtBuf(0) ExtBuf(1) ExtBuf(2) ExtBuf(3)
return

