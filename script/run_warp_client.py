#!/usr/bin/env python

import sys, os, time
from os.path import dirname
from os.path import join

if len(sys.argv) != 2:
  print "usage: %s <host-ip>" % sys.argv[0]
  sys.exit(1)

project_dir = dirname(dirname(os.path.realpath(__file__)))
prog = join(project_dir, "build", "test", "util", "warp_client_main")

env_params = (
  "GLOG_logtostderr=true "
  "GLOG_v=-1 "
  "GLOG_minloglevel=0 "
  )

params = {
    "server_ip": sys.argv[1]
    }

cmd = env_params + prog
cmd += "".join([" --%s=%s" % (k,v) for k,v in params.items()])
print cmd
os.system(cmd)
