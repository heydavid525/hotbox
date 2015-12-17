from __future__ import print_function
import sys
import os
from os.path import dirname
from os.path import join
import argparse

project_dir = dirname(dirname(dirname(os.path.realpath(__file__))))
sys.path.append(join(project_dir, 'python/db'))

from db_client import HBClient

"""
Usage (Ingest one file):
  python python/db/ingest.py --path /path/to/file.libsvm

Usage (Ingest a directory):
  python python/db/ingest.py --path /path/to/dir/

"""

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--path")
  args = parser.parse_args()

  print('args.path', args.path)
  if not args.path:
    print('--path must be specified (directory or single file).')
    sys.exit(1)

  server_ip = "localhost"
  db_client = HBClient(server_ip)
  test_db = db_client.CreateDB('test_db', use_dense_weight=False)

  if os.path.isdir(args.path):
    files = [join(args.path, f) for f in os.listdir(args.path) if
        os.path.isfile(join(args.path, f))]
  else:
    files = [args.path]
  for i, f in enumerate(files):
    no_commit = False if i == len(files) - 1 else True
    print('-' * 10, 'Ingesting', f)
    test_db.ReadFile(f, file_format='libsvm', no_commit=no_commit)
