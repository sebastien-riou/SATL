from psb import *

if platform.system()=='Windows':
    libs='-lws2_32'
else:
    libs=''

def build_test(role,name):
    BIN=np('../../bin',name)
    if not os.path.exists(BIN):
        os.makedirs(BIN)

    main = '../../'+role+'.c'
    emu = name+'_emu.c'
    if os.path.exists(emu):
        files = [main,emu]
    else:
        files = main
    compile(files,np(BIN,role+'_'+name),libs)

def build_tests(name):
    build_test('master',name)
    build_test('slave',name)
