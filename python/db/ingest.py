import sys
import os
from os.path import dirname
from os.path import join

project_dir = dirname(dirname(dirname(os.path.realpath(__file__))))
sys.path.append(join(project_dir, 'python/db'))

from db_client import HBClient

if __name__ == "__main__":
  server_ip = "localhost"
  db_client = HBClient(server_ip)
  test_db = db_client.CreateDB('url_combined_db')
  test_db.ReadFile('/home/wanghy/datasets/url_combined',
      file_format='libsvm')
