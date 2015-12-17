#!/usr/bin/env python

import zmq
import argparse
import sys
import os
import time
from os.path import dirname
from os.path import join

project_dir = dirname(dirname(dirname(os.path.realpath(__file__))))
print project_dir
#sys.path.insert(1, join(project_dir, 'build'))
#sys.path.insert(1, join(project_dir, 'third_party', 'include'))
sys.path.append(join(project_dir, 'build'))
sys.path.append(join(project_dir, 'third_party', 'include'))

from util.proto.warp_msg_pb2 import *

if __name__ == "__main__":
  parser = argparse.ArgumentParser(
      formatter_class=argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('--server_ip', default='', help='server ip',
      required=True)
  parser.add_argument('--server_port', default='19856', help='server port')
  args = parser.parse_args()
  print("server_ip: " + args.server_ip)
  #parser.add_argument('--foo', type=int, default=42, help='FOO!')
  #parser.add_argument('--server_ip', action='store_const', const=42)
  #context = zmq.Context.instance()
  context = zmq.Context()
  #sock = context.socket(zmq.ROUTER)
  sock = context.socket(zmq.REQ)
  print("connect to " + "tcp://%s:%s" % (args.server_ip, args.server_port))
  sock.connect("tcp://%s:%s" % (args.server_ip, args.server_port))
  handshake_msg = ClientMsg()
  handshake_msg.handshake_msg.dummy = True
  assert handshake_msg.HasField("handshake_msg")
  try:
    sock.send(handshake_msg.SerializeToString(), copy=False);
  except Exception as e:
    print(e.message)
  time.sleep(5)
