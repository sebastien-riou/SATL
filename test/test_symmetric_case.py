#!/usr/bin/env python3
import os
import sys
import time
args = ' '.join(sys.argv[1:])
os.spawnl(os.P_NOWAIT, sys.executable, 'py','satl_test.py slave '+args)
time.sleep(1)
os.spawnl(os.P_WAIT, sys.executable, 'py','satl_test.py master '+args)
