#!/usr/bin/env python

"""
Export hotbox to a path as third party.
"""

from __future__ import print_function
import sys, os, time
from os.path import dirname
from os.path import join
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--install_path")
args = parser.parse_args()
if not args.install_path:
  print('--install_path must be specified (installed to install_path/lib'
    + ', install_path/include/hotbox.)')
  sys.exit(1)

lib_path = join(args.install_path, 'lib')
print('lib_path:', lib_path)
os.system('mkdir -p %s' % lib_path)
os.system('cp build/lib/libhotbox.so %s' % lib_path)

include_path = join(args.install_path, 'include', 'hotbox')
print('include path:', include_path)
os.system('rm -r %s' % include_path)
os.system('mkdir -p %s' % include_path)
os.system('cp -r src/* %s' % include_path)
os.system('cp -r build %s' % include_path)
