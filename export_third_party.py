#!/usr/bin/env python

"""
Export hotbox to a path as third party.
"""

from __future__ import print_function
import sys, os, time
from os.path import dirname
from os.path import join

path = '/home/wdai/lib/hotbox'

lib_path = join(path, 'lib')
print('lib_path:', lib_path)
os.system('mkdir -p %s' % lib_path)
os.system('cp build/lib/libhotbox.so %s' % lib_path)

include_path = join(path, 'include', 'hotbox')
print('include path:', include_path)
os.system('rm -r %s' % include_path)
os.system('mkdir -p %s' % include_path)
os.system('cp -r src/* %s' % include_path)
os.system('cp -r build %s' % include_path)
