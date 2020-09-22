#!/usr/bin/env python3

#build and import utils
from build import *

launch_c_impl('slave','tesic_apb')
launch_c_impl('master','tesic_apb')
