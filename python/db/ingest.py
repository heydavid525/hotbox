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
  parser.add_argument("--reps")  # repeating ingest
  args = parser.parse_args()

  if not args.path:
    print('--path must be specified (directory or single file).')
    sys.exit(1)
  if not args.db:
    print('--db must be specified.')
    sys.exit(1)
  num_reps = int(args.reps) if args.reps else 1
  if num_reps > 1:
    print('Repeat data', num_reps, 'times')

  hb_client = HBClient(yconfig['server_ip'])
  db = hb_client.CreateDB(args.db, use_dense_weight=False)

  if os.path.isdir(args.path):
    files = [join(args.path, f) for f in os.listdir(args.path) if
        os.path.isfile(join(args.path, f))]
  else:
    files = [args.path]
  files = files * num_reps
  for i, f in enumerate(files):
    commit = True if i == len(files) - 1 else False
    print('-' * 10, 'Ingesting', f)
    format = 'libsvm' if not args.format else args.format
    db.ReadFile(f, file_format=format, commit=commit,
        collect_stats=args.stats)
