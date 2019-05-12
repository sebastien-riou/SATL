#!/usr/bin/env python3
import os
import shutil
import ntpath


from basic.clean import *


for p in os.walk('..'):
    if '__pycache__'==ntpath.basename(p[0]):
        shutil.rmtree(p[0],ignore_errors=True)
