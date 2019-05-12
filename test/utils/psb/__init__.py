


import os
import sys
import platform
import shutil
import subprocess

def np(*unix_paths):
    p = os.sep.join(unix_paths)
    return p.replace('/',os.sep)

def appendto(list,args,op=None):
    def nop(x):
        return x
    if op is None:
        op = nop
    if isinstance(args, str) & (len(args)>0):
        list.append(op(args))
    else:
        for a in args:
            list.append(op(a))
    
def compile(files,output="a.out",libs=None):
    gcc = shutil.which('gcc')
    cmd = [gcc, '-std=c99','-I','.']
    appendto(cmd,files,np)
    cmd.append('-o')
    cmd.append(np(output))
    appendto(cmd,libs,lambda lib: '-l'+lib)
    print(' '.join(cmd))
    subprocess.run(cmd,check=True)
