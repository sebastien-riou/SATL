import os
import sys
import time
import subprocess

from psb import *

if platform.system()=='Windows':
    libs='ws2_32'
else:
    libs=''

def build_test(role,name):
    BIN=np('../../bin',name)
    if not os.path.exists(BIN):
        os.makedirs(BIN)

    main = '../../'+role+'.c'
    files = main

    for emu in [name+'_emu.c',name+'_'+role+'_emu.c']:
        if os.path.exists(emu):
            files = [main,emu]

    compile(files,np(BIN,role+'_'+name),libs)

def build_tests(name):
    build_test('master',name)
    build_test('slave',name)

def launch_py_impl(role,buffer_length,granularity,sfr_granularity,skip_init):
    cmd = [ sys.executable,'../../../satl_test.py',role,str(buffer_length),str(granularity),str(sfr_granularity),str(skip_init)]
    if len(sys.argv)>1:
        cmd.append(sys.argv[1])
    if len(sys.argv)>4:
        cmd.append(sys.argv[4])

    print(' '.join(cmd))
    if role=='master':
        time.sleep(1)
        subprocess.run(cmd,check=True)
    else:
        subprocess.Popen(cmd)

def launch_c_impl(role,name):
    args = ' '.join(sys.argv[1:])

    #import shlex
    #args = shlex.split(command_line)
    cmd = [ np('../../bin/'+name+'/'+role+'_'+name)]
    if len(args):
        cmd+= args.split(" ")
    print(' '.join(cmd))
    if role=='master':
        time.sleep(1)
        subprocess.run(cmd,check=True)
    else:
        subprocess.Popen(cmd)

def test_c(role,name,buffer_length,granularity,sfr_granularity,skip_init):
    if role=='slave':
        launch_c_impl('slave',name)
        launch_py_impl('master',buffer_length,granularity,sfr_granularity,skip_init)
    else:
        launch_py_impl('slave',buffer_length,granularity,sfr_granularity,skip_init)
        launch_c_impl('master',name)

def test_symmetric_case(buffer_length,granularity,sfr_granularity,skip_init):
    cmd = [ sys.executable,'test_symmetric_case.py']
    for i in [buffer_length,granularity,sfr_granularity,skip_init]:
        cmd.append(str(i))
    for a in sys.argv[1:]:
        cmd.append(a)

    subprocess.run(cmd,check=True)
