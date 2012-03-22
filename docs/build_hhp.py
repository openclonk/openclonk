#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys

f = file(sys.argv[1], "w")
fin = file(sys.argv[2], "r")
for line in fin:
    if line.find("[INFOTYPES]") != -1:
        for filename in sys.argv[3:]:
            f.write(filename[:-3].replace("/", "\\") + 'html\r\n')
    f.write(line)
f.close()
fin.close()
