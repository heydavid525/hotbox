#!/usr/bin/env python

import sys, os, time
from os.path import dirname
from os.path import join

project_dir = dirname(dirname(os.path.realpath(__file__)))
src_dir = join(project_dir, 'src')

cmd = "find %s -name '*.hpp' -o -name '*.cpp' | xargs wc -l" % src_dir
print cmd
os.system(cmd)

cmd = "find %s -name '*.proto' | xargs wc -l" % src_dir
print cmd
os.system(cmd)
