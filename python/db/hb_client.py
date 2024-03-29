#!/usr/bin/env python

from __future__ import print_function
import zmq
import argparse
import sys
import os
import time
from os.path import dirname
from os.path import join
import yaml
import snappy
import json

project_dir = dirname(dirname(dirname(os.path.realpath(__file__))))
sys.path.append(join(project_dir, 'build'))
sys.path.append(join(project_dir, 'third_party', 'include'))

import util.proto.warp_msg_pb2 as warp_msg_pb
import util.proto.util_pb2 as util_pb

class WarpClient:
  def __init__(self):
    self.context = zmq.Context()
    self.sock = self.context.socket(zmq.REQ)
    with open(join(project_dir, 'config.yaml')) as f:
      default_config = yaml.load(f)
    server_ip = default_config['server_ip']
    server_port = default_config['server_port']
    connect_point = "tcp://%s:%s" % (server_ip, server_port)
    print("WarpClient connecting to %s" % connect_point)
    self.sock.connect(connect_point)
    # send and receive handshake msg
    handshake_msg = warp_msg_pb.ClientMsg()
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
      self.sock.send(snappy.compress(client_msg.SerializeToString()),
          copy=False)
    except Exception as e:
      print(e.message)

  def Recv(self):
    try:
      data = snappy.uncompress(self.sock.recv())
    except zmq.ZMQError as e:
      print(e.message)
    server_msg = warp_msg_pb.ServerMsg()
    server_msg.ParseFromString(data)
    return server_msg

class HBClient:
  """
  HBClient is the python portal to Hotbox server.
  HBClient is considered a singleton class, though python has no way to enforce
  that.
  """
  def __init__(self, server_ip):
    self.warp_client = WarpClient()

  def CreateDB(self, db_name, db_description='', int_label=True,
    use_dense_weight=True):
    """
    Input:
      db_name: string
      int_label: True to use integer label (classification),otherwise
        float (regression)
      use_dense_weight: True if dataset has many weights != 1

    Return:
      A DB object for querying/modifying the created DB.
    """
    msg = warp_msg_pb.ClientMsg()
    msg.create_db_req.db_config.db_name = db_name
    msg.create_db_req.db_config.db_description = db_description
    msg.create_db_req.db_config.schema_config.int_label = int_label
    msg.create_db_req.db_config.schema_config.use_dense_weight = \
        use_dense_weight
    reply = self.warp_client.SendRecv(msg)
    assert reply.HasField('generic_reply')
    print(reply.generic_reply.msg)
    return DB(db_name, self.warp_client)

  def GetDB(self, db_name):
    return DB(db_name, self.warp_client)

class DB:
  """
  A proxy object for accessing a DB on server.
  """

  def __init__(self, db_name, warp_client):
    """
    Input:
      db_name: string
      warp_client: WarpClient reference.
    """
    self.db_name = db_name
    self.warp_client = warp_client

  # header is the line # to be treated as header (0 for no header). Data will
  # be read after header line.
  # TODO(wdai): currently header isn't supported.
  def ReadFile(self, file_paths, file_format='csv', commit=True, header=0,
      collect_stats=True, num_features_default=0):
    msg = warp_msg_pb.ClientMsg()
    msg.read_file_req.db_name = self.db_name
    msg.read_file_req.file_paths.extend(file_paths)
    # A python switch statement on file_format.
    # TODO(wdai): This is easy to break when adding new file format. Find way
    # to automatically generate this based on proto definition.
    msg.read_file_req.file_format = {
        'csv': util_pb.CSV,
        'libsvm': util_pb.LIBSVM,
        'family': util_pb.FAMILY,
        }.get(file_format, 0)
    # Figure out the dimension from meta files.
    has_meta = all([os.path.isfile(fname + '.meta.json') for fname in
      file_paths])
    if has_meta and file_format == 'libsvm' and num_features_default == 0:
      for fname in file_paths:
        meta_file = fname + '.meta.json'
        print('Reading', meta_file)
        with open(meta_file, 'r') as f:
          data = json.load(f)
          num_features_default = max(num_features_default, \
            data['feature_dim'])
      print('num_features_default:', num_features_default)
    print('num_features_default:', num_features_default)
    msg.read_file_req.num_features_default = num_features_default

    if file_format == 'libsvm':
      msg.read_file_req.parser_config.libsvm_config.feature_one_based = True
          
    msg.read_file_req.header = header
    msg.read_file_req.commit = commit
    msg.read_file_req.parser_config.collect_stats = collect_stats
    reply = self.warp_client.SendRecv(msg)
    print('Reading file %s ...' % file_paths)
    print(reply.generic_reply.msg)
