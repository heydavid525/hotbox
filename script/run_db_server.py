#!/usr/bin/env python
from __future__ import print_function
import sys, os, time
from os.path import dirname
from os.path import join
import yaml
import argparse

"""
Usage:
  ./script/run_db_server.py --delete_old to remove existing db meta file.
"""
parser = argparse.ArgumentParser()
parser.add_argument('--delete_old', dest='delete_old', action='store_true')
parser.set_defaults(delete_old=False)

args = parser.parse_args()

project_dir = dirname(dirname(os.path.realpath(__file__)))
db_testbed = join(project_dir, 'db_testbed')
os.system('mkdir -p %s' % db_testbed)
prog = join(project_dir, "build", "test", "db", "db_server_main")

if args.delete_old:
  with open(os.path.join(project_dir, 'config.yaml'), 'r') as stream:
    try:
        configs = yaml.load(stream)
        print('deleting %s' % configs['db_meta_dir'])
        os.system('rm -rf %s' % configs['db_meta_dir'])
        os.system('mkdir -p %s' % configs['db_meta_dir'])
    except yaml.YAMLError as exc:
        print(exc)

env_params = (
  "GLOG_logtostderr=true "
  "GLOG_v=-1 "
  "GLOG_minloglevel=0 "
  "/usr/bin/time -v"
  )

cmd = env_params + prog
#cmd += "".join([" --%s=%s" % (k,v) for k,v in params.items()])
print(cmd)
os.system(cmd)
