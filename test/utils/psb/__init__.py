


import os
import sys
import platform
import shutil
import subprocess

def np(*unix_paths):
    p = os.sep.join(unix_paths)
    return p.replace('/',os.sep)

def compile(files,output="a.out",libs=None):
    gcc = shutil.which('gcc')
    cmd = [gcc, '-std=c99','-I','.']
    if isinstance(files, str) & (len(files)>0):
        cmd.append(np(files))
    else:
        for f in files:
            cmd.append(np(f))
    cmd.append('-o')
    cmd.append(np(output))
    for l in libs:
        cmd.append('-l'+l)
    print(' '.join(cmd))
    subprocess.run(cmd,check=True)
