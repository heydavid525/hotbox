#!/usr/bin/env python
from __future__ import print_function
import sys, os, time
from os.path import dirname
from os.path import join
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--machines', dest='machine_file', default='', 
  help='List of machine IPs. If not supplied, run locally.')

args = parser.parse_args()
ips = []
num_workers = 1
if args.machine_file != '':
  with open(args.machine_file) as f:
    ips = [l.strip() for l in f.readlines()]
  num_workers = len(ips)

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
    "db_name": 'a1a'
    , 'use_proxy': 'false'
    , 'num_proxy_servers': 1
    , "session_id": "session0"
    , "transform_config": "select_all.conf"
    , 'num_workers': num_workers
    , 'num_threads': 1
    , 'num_io_threads': 1
    , 'buffer_limit': 16
    , 'batch_limit': 16
    }

ssh_cmd = (
    "ssh "
    "-o StrictHostKeyChecking=no "
    "-o UserKnownHostsFile=/dev/null "
    )

if len(ips) == 0:
  cmd = env_params + prog
  cmd += "".join([" --%s=%s" % (k,v) for k,v in params.items()])
  print(cmd)
  os.system(cmd)
  sys.exit(0)

for client_id, ip in enumerate(ips):
  params['worker_id'] = client_id
  cmd = ssh_cmd + ip + ' '
  cmd += env_params + prog
  cmd += "".join([" --%s=%s" % (k,v) for k,v in params.items()])
  cmd += ' &'
  print(cmd)
  os.system(cmd)
