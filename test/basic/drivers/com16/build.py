import os
import sys


import shutil
import subprocess


def np(*unix_paths):
    p = os.sep.join(unix_paths)
    return p.replace('/',os.sep)
    
BIN=np('../../bin/com16')
if not os.path.exists(BIN):
    os.makedirs(BIN)

gcc = shutil.which('gcc')

cmd = [gcc, '-std=c99','-I','.',np('../../master.c'),np('./com16_emu.c'),'-o',np(BIN,'master_com16'),'-lws2_32']
print(' '.join(cmd))
subprocess.run(cmd,check=True)    

cmd = [gcc, '-std=c99','-I','.',np('../../slave.c'),np('./com16_emu.c'),'-o',np(BIN,'slave_com16'),'-lws2_32']
print(' '.join(cmd))
subprocess.run(cmd,check=True)    

