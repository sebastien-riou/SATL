#!/usr/bin/env python3
import os
import sys
import time

os.spawnl(os.P_WAIT, sys.executable, 'py','build.py')

def np(*unix_paths):
    p = os.sep.join(unix_paths)
    return p.replace('/',os.sep)
   
args = ' '.join(sys.argv[1:])
os.spawnl(os.P_NOWAIT, np('../../bin/socket/slave_socket'), 'slave_socket '+args)
time.sleep(1)
os.spawnl(os.P_WAIT, np('../../bin/socket/master_socket'), 'master_socket '+args)

