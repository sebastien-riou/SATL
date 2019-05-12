#!/usr/bin/env python3

import sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__),'..','..','..', 'utils')))
from testbuilder import *

build_tests('com16')
