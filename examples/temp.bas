let count = 10
let sum = 0
dim A(10)
let C$ = string$(10, 62)
print C$
for i = 1 to count
let sum = sum + 1
let A(i) = sum
next i
print sum, A(1), A(2), A(3), A(4), A(5), A(6), A(7), A(8), A(9), A(10)
end
