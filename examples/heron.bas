let v = 400000 ' Value to calculate the square root
let s = 100    ' Initial guess
dim Res(5)     ' Array to store the intermediate results

REM Root calculation according to Heron
for i = 1 to 6
    let t = ((v / s) + s) / 2
    Res(i) = t
    if t = s goto exit
    let s = t
next i

exit:
print "The square root from" v;"is" t

print "Intermediate results:"
for i = 1 to 6
    print Res(i),
next i
print
free
end
