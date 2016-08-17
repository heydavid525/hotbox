#!/usr/bin/env python

import sys, os, time
from os.path import dirname
from os.path import join
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--num_proxy_servers', help='number of proxy servers',
    type=int)
args = parser.parse_args()
num_proxy_servers = 1
if args.num_proxy_servers:
  num_proxy_servers = args.num_proxy_servers

project_dir = dirname(dirname(os.path.realpath(__file__)))
db_testbed = join(project_dir, 'db_testbed')
os.system('mkdir -p %s' % db_testbed)
prog_name = 'proxy_server_main'
prog = join(project_dir, "build", "test", "client", \
   prog_name) 

env_params = (
  "GLOG_logtostderr=true "
  "GLOG_v=-1 "
  "GLOG_minloglevel=0 "
  )

params = {
    "server_id": 0
    }

os.system("killall -q " + prog_name)
print "Done killing"

for s in range(num_proxy_servers):
  cmd = env_params + prog
  params['server_id'] = s
  cmd += "".join([" --%s=%s" % (k,v) for k,v in params.items()])
  cmd += '&'
  print cmd
  os.system(cmd)
