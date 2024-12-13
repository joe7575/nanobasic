REM Test Basic V2 features
const MAX = 10
let i = 5
dim A(5)
let B$ = string$(10,0)
C$ = "Hello"
let A(1) = 1

while i > 0
    print i
    i = i - 1
end

for j = 1 to 5
    for k = 1 to j
      print j, k,
    next
    print
next j

if j > 5 then
    print "j is greater than 5"
    print "j is greater than 5"
end

if j > 5 then
    print "j is greater than 5"
    print "j is greater than 5"
else
    print "j is smaller or equal to 5"
    print "j is smaller or equal to 5"
end

if j < 5 then
    print "j is smaller than 5"
    print "j is smaller than 5"
else 
    if i < MAX then
        print "i is smaller than MAX"
    end

    print "j is greater or equal to 5"
    print "j is greater or equal to 5"
end

erase A
erase B$
erase C$
exit

