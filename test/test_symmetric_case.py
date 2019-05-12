#!/usr/bin/env python3
import os
import sys
import time
import subprocess
#args = ' '.join(sys.argv[1:])
#os.spawnl(os.P_NOWAIT, sys.executable, 'py','satl_test.py slave '+args)
#time.sleep(1)
#os.spawnl(os.P_WAIT, sys.executable, 'py','satl_test.py master '+args)

cmd = [ sys.executable,'satl_test.py','slave']
for a in sys.argv[1:]:
    cmd.append(a)

print(' '.join(cmd))
subprocess.Popen(cmd)
time.sleep(1)
cmd[2]='master'
print(' '.join(cmd))
subprocess.run(cmd,check=True)
