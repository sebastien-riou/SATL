import os
import sys


import shutil
import subprocess


def np(*unix_paths):
    p = os.sep.join(unix_paths)
    return p.replace('/',os.sep)
    
BIN=np('../../bin/socket')
if not os.path.exists(BIN):
    os.makedirs(BIN)

gcc = shutil.which('gcc')

cmd = [gcc, '-std=c99','-I','.',np('../../master.c'),'-o',np(BIN,'master_socket'),'-lws2_32']
print(' '.join(cmd))
subprocess.run(cmd,check=True)    

cmd = [gcc, '-std=c99','-I','.',np('../../slave.c'),'-o',np(BIN,'slave_socket'),'-lws2_32']
print(' '.join(cmd))
subprocess.run(cmd,check=True)    
    
#gcc -std=c99 -fsanitize=address,undefined -I . ../../master.c -o $BIN/master_socket
#gcc -std=c99 -fsanitize=address,undefined -I . ../../slave.c  -o $BIN/slave_socket
