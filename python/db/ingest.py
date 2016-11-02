#!/usr/bin/env python
from __future__ import print_function
import sys
import os
from os.path import dirname
from os.path import join
import argparse
import yaml

project_dir = dirname(dirname(dirname(os.path.realpath(__file__))))
sys.path.append(join(project_dir, 'python/db'))

from hb_client import HBClient

"""
Usage (Ingest one file):
  python python/db/ingest.py --path /path/to/file.libsvm --db some_db_name
   [--format libsvm/family]

Usage (Ingest a directory):
  python python/db/ingest.py --path /path/to/dir/ --db some_db_name

"""

if __name__ == "__main__":

  with open(join(project_dir, 'config.yaml'), 'r') as f:
      try:
          yconfig = yaml.load(f)
      except yaml.YAMLError as exc:
          print(exc)
  parser = argparse.ArgumentParser()
  parser.add_argument("--path")
  parser.add_argument("--db")
  parser.add_argument("--format")
  # Disable stats collection with --no-stats
  parser.add_argument('--no-stats', dest='stats', action='store_false')
  parser.set_defaults(stats=True)
  parser.add_argument("--reps", type=int, default=1)  # repeating ingest
  parser.add_argument("--num_features_default", type=int, default=0)  # repeating ingest
  args = parser.parse_args()

  if not args.path:
    print('--path must be specified (directory or single file).')
    sys.exit(1)
  if not args.db:
    print('--db must be specified.')
    sys.exit(1)
  num_reps = args.reps
  if num_reps > 1:
    print('Repeat data', num_reps, 'times')

  hb_client = HBClient(yconfig['server_ip'])
  db = hb_client.CreateDB(args.db, use_dense_weight=False)
  files = []
  if os.path.isdir(args.path):
    for root, dirs, file_list in os.walk(args.path):
      for name in file_list:
        filename = join(root, name)
        files.append(filename)
  else:
    files = [args.path]
  print (files)
  
  files = files * num_reps
  format = 'libsvm' if not args.format else args.format
  db.ReadFile(files, file_format=format, commit=True,
      collect_stats=args.stats, num_features_default=args.num_features_default)
