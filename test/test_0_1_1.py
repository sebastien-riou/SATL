import os
import sys
import time
args = ' '.join(sys.argv[1:])
os.spawnl(os.P_WAIT, sys.executable, 'py','test_symmetric_case.py 0 1 1 1 '+args)
