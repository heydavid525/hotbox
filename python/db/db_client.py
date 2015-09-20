#!/usr/bin/env python

import zmq
import argparse
import sys
import os
import time
from os.path import dirname
from os.path import join
import yaml

project_dir = dirname(dirname(dirname(os.path.realpath(__file__))))
sys.path.append(join(project_dir, 'build'))
sys.path.append(join(project_dir, 'third_party', 'include'))

from util.proto.warp_msg_pb2 import *
from util.proto.warp_config_pb2 import WarpClientConfig
from db.proto.db_pb2 import *

class WarpClient:
  def __init__(self, warp_client_config):
    self.context = zmq.Context()
    self.sock = self.context.socket(zmq.REQ)
    with open(join(project_dir, 'config.yaml')) as f:
      default_config = yaml.load(f)
    server_ip = warp_client_config.server_ip
    server_port = default_config['default_server_port']
    connect_point = "tcp://%s:%s" % (server_ip, server_port)
    print("WarpClient connecting to ", connect_point)
    self.sock.connect(connect_point)
    # send and receive handshake msg
    handshake_msg = ClientMsg()
    handshake_msg.handshake_msg.dummy = True
    assert handshake_msg.HasField("handshake_msg")
    self.Send(handshake_msg)
    server_msg = self.Recv()
    print('Done handshake with server')

  def SendRecv(self, client_msg):
    """
    Request with client msg, return msg from server.
    """
    self.Send(client_msg)
    return self.Recv()

  def Send(self, client_msg):
    try:
      serialized = client_msg.SerializeToString()
      print serialized
      print len(serialized)
      self.sock.send(serialized, copy=False);
    except Exception as e:
      print(e.message)

  def Recv(self):
    try:
      data = self.sock.recv()
    except zmq.ZMQError as e:
      print(e.message)
    server_msg = ServerMsg()
    server_msg.ParseFromString(data)
    return server_msg

class DBClient:
  def __init__(self, server_ip):
    warp_client_config = WarpClientConfig()
    warp_client_config.server_ip = server_ip
    self.warp_client = WarpClient(warp_client_config)

  def CreateDB(self, db_name, db_description='', int_label=True,
    use_dense_weight=True):
    """
    Input:
      db_name: string
      int_label: True to use integer label (classification),otherwise
        float (regression)
      use_dense_weight: True if dataset has many weights != 1
    """
    msg = ClientMsg()
    msg.create_db_req.db_config.db_name = db_name
    msg.create_db_req.db_config.db_description = db_description
    msg.create_db_req.db_config.schema_config.int_label = int_label
    msg.create_db_req.db_config.schema_config.use_dense_weight = \
        use_dense_weight
    print 'reply from CreateDBReq:', self.warp_client.SendRecv(msg)

if __name__ == "__main__":
  server_ip = "localhost"
  db_client = DBClient(server_ip)
  db_client.CreateDB('test_db')
