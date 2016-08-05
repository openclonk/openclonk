#!/usr/bin/python
# Add missing entries existing in one Language** file but not in the other
# Also sort entries by key

import os
import glob

language_files = glob.glob('../planet/System.ocg/Language*.txt')
data = [open(fn).read().splitlines() for fn in language_files]
n = len(data)
all_entries = {}

for i,d in enumerate(data):
    for l in d:
        k,v = l.split('=', 1)
        if not k in all_entries:
            all_entries[k] = ['MISSING'] * n
        all_entries[k][i] = v

fids = [open(fn, 'wt') for fn in language_files]

for k,v in sorted(all_entries.iteritems()):
    for i,fid in enumerate(fids):
        fid.write('%s=%s\n' % (k, v[i]))
    
for fid in fids:
    fid.close()
