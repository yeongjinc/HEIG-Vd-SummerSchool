from random import randint
import os

#call(["./L2", "ZE^9Zo^9Z?^9"])
for i in range(0, 1024):
    p = []
    for j in range(0, 8):
        p.append(randint(10, 127))
    p.append(p[0] ^ p[4] ^ 0x5a)
    p.append(p[1] ^ p[5] ^ 0x15)
    p.append(p[2] ^ p[6] ^ 0x5e)
    p.append(p[3] ^ p[7] ^ 0x39)
    c = map(chr, p)
    s = ''.join(c)
    f = open("pas", "w")
    f.write(s)
    f.close()
    cmd = """/bin/bash -c './L2 "$(< pas)"' """
    os.system(cmd)
