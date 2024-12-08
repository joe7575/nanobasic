let v = 4000
let s = 100

REM Wuerzelberechnung nach Heron
for i = 1 to 10
    let t = ((v / s) + s) / 2
    if t = s goto exit
    let s = t
next i

exit:
print i, s
end
