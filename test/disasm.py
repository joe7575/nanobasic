import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))

is_active = False
index = 0
Opcodes = []
for line in open("nb_int.h").readlines():
    if "Opcode definitions" in line:
        is_active = True
        continue
    elif "};" in line:
        is_active = False
        continue
    if is_active:
        words = line.split()
        if words[0] == "k_END,":
            opcode = words[0][2:-1]
            bytes = 1
        elif words[0] == "k_PUSH_STR_Nx,":
            opcode = words[0][2:-4]
            bytes = 0
        elif words[0][0] == "k":
            opcode = words[0][2:-4]
            bytes = words[0][-2]
        else:
            continue
        Opcodes.append((opcode, int(bytes)))
        #print(opcode, bytes)

first_line = True        
code = ""
for line in open("output.txt").readlines():
    code = code + line + " "
    
words = code.split()
assert words[0] == "BC"
assert words[1] == "01"
idx = 2
while idx < len(words):
    byte = int(words[idx], 16)
    if byte < len(Opcodes):
        opcode, bytes = Opcodes[byte]
        print("%04X: %-14s  " % (index, opcode), end="")
        if bytes == 0:
            bytes = int(words[idx+1], 16) + 1
            idx += 1
            print('"', end="")
            for i in range(1, bytes):
                print("%c" % int(words[idx+i], 16), end="")
            print('"')
        else:
            for i in range(1, bytes):
                print("%02X " % int(words[idx+i], 16), end="")
            print()
        index += bytes
        idx += bytes
            
