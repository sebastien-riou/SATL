#!/usr/bin/env python3
import os
import sys
import time

os.spawnl(os.P_WAIT, sys.executable, 'py','build.py')

def np(*unix_paths):
    p = os.sep.join(unix_paths)
    return p.replace('/',os.sep)

argspy = ''
if len(sys.argv)>1:
    argspy = sys.argv[1]+' '
if len(sys.argv)>4:
    argspy += sys.argv[4]+' '
    
args = ' '.join(sys.argv[1:])
os.spawnl(os.P_NOWAIT, np('../../bin/com16/slave_com16'), 'slave_com16 '+args)
time.sleep(1)
os.spawnl(os.P_WAIT, sys.executable, 'py','../../../satl_test.py master 0 2 2 1 '+argspy)

