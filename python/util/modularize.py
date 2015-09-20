#!/usr/bin/env python

import sys, os, time
from os.path import dirname
from os.path import join

if __name__ == '__main__':
  if len(sys.argv) != 2:
    print "usage: %s <build_path>" % sys.argv[0]
    sys.exit(1)

build_path = sys.argv[1]

for root, subdirs, files in os.walk(build_path):
  # Create an empty __init__.py in every directory.
  print 'creating',join(root, '__init__.py')
  with open(join(root, '__init__.py'), 'w'):
    pass
