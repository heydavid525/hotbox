#!/usr/bin/env python

import sys, os, time
from os.path import dirname
from os.path import join

if len(sys.argv) < 2:
  print "usage: %s <folder-or-file> [folder-or-file]" % sys.argv[0]
  sys.exit(1)

project_dir = dirname(dirname(os.path.realpath(__file__)))
script_dir = join(project_dir, 'script')
prog = join(script_dir, "cpplint.py")

lint_options = (
    '--filter=-legal/copyright' # exclude legal checks
    ',-build/include_order' # exclude include order
    ',-build/c++11'  # we support c++11
    ' --extensions=hpp,cpp' # cpplint.py doesn't support hpp by default
    )

cmd = "find %s -name '*.hpp' -o -name '*.cpp' | xargs %s %s"  % \
    (' '.join(sys.argv[1:]), prog, lint_options)
print cmd
os.system(cmd)
