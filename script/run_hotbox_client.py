#!/usr/bin/env python

import sys, os, time
from os.path import dirname
from os.path import join

project_dir = dirname(dirname(os.path.realpath(__file__)))
db_testbed = join(project_dir, 'db_testbed')
os.system('mkdir -p %s' % db_testbed)
prog = join(project_dir, "build", "test", "client", "hotbox_client_main")

env_params = (
  "GLOG_logtostderr=true "
  "GLOG_v=-1 "
  "GLOG_minloglevel=0 "
  )

params = {
    #"transform_config": "select_transform.conf"
    "transform_config": "select_all.conf"
    }

cmd = env_params + prog
cmd += "".join([" --%s=%s" % (k,v) for k,v in params.items()])
print cmd
os.system(cmd)
