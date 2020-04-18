#!/usr/bin/env python3

#build and import utils
from build import *

launch_c_impl('slave','serial')
launch_c_impl('master','serial')
