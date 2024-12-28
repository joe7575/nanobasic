dim Pld(3)

set1(Pld, 0, 32)
set1(Pld, 1, 1)
set1(Pld, 2, 255)
set1(Pld, 3, 250)
set4(Pld, 4, 4294967295)

send(1, 1014, Pld)

for i = 0 to 50
  print ".";
next
end

start:
  id = param()
  port = param()
  print "start", port, id, get1(Pld, 0), get1(Pld, 1), get1(Pld, 2), get1(Pld, 3), get4(Pld, 4)
  return
