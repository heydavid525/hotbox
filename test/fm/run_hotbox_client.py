#!/usr/bin/env python

import sys, os, time
from os.path import dirname
from os.path import join

project_dir = dirname(os.path.realpath(__file__))
#db_testbed = join(project_dir, 'db_testbed')
#os.system('mkdir -p %s' % db_testbed)
prog = join(project_dir, "bin", "petuum_fm")
#/home/ubuntu/github/hotbox/test/fm/bin/petuum_fm


env_params = (
  "GLOG_logtostderr=true "
  "GLOG_v=-1 "
  "GLOG_minloglevel=0 "
  )


params = {
    #"transform_config": "select_transform.conf"
    "db_name": "higgs"
    , "session_id": "weiren_testdnn"
    , "transform_config": "dnn.conf"
#    , "master_endpoint": "127.0.0.1:24599"
#    , "min_table_threads": 1
#    , "min_work_threads": 1 
#    , "num_sync_threads": 1
#    , "num_table_partitions": 1
#    , "num_table_threads": 1 
#    , "num_work_partitions": 1
#    , "num_work_threads": 1 
    #, "transform_config": "onehot_toy.conf"
    #, "transform_config": "dnn.conf"
    #, "transform_config": "select_toy.conf"
    #, "transform_config": "select_all_ngram.conf"
    }

cmd = env_params + prog
cmd += "".join([" --%s=%s" % (k,v) for k,v in params.items()])
print cmd
os.system(cmd)
