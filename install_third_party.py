#!/usr/bin/env python
import os
import sys

# build_path is mandatory, build_all is optional.
if len(sys.argv) < 2:
  print "usage: %s [build_path> [build_all]" % sys.argv[0]
  sys.exit(1)

# Build all is by default False.
build_all = False
if len(sys.argv) == 3 and sys.argv[2] == 'build_all':
  build_all = True

build_path = sys.argv[1]

cmd = 'git clone https://github.com/daiwei89/hotbox_third_party %s' \
    % build_path
print(cmd)
os.system(cmd)

if build_all:
  cmd = 'cd %s; make -j third_party_core' % (build_path)
else:
  cmd = 'cd %s; make -j third_party_special' % (build_path)
print(cmd)
os.system(cmd)
