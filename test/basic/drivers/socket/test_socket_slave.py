#!/usr/bin/env python3
import os
import sys
import time
import subprocess

#build and import utils like np
from build import *

argspy = ''
if len(sys.argv)>1:
    argspy = ' '+sys.argv[1]
if len(sys.argv)>4:
    argspy += ' '+sys.argv[4]

args = ' '.join(sys.argv[1:])

#import shlex
#args = shlex.split(command_line)
cmd = [ np('../../bin/socket/slave_socket')]
if len(args):
    cmd+= args.split(" ")
print(cmd)
subprocess.Popen(cmd)


time.sleep(1)
a2 = '../../../satl_test.py master 0 1 1 1'+argspy
cmd = [ sys.executable] + a2.split(" ")
print(cmd)
subprocess.run(cmd,check=True)
