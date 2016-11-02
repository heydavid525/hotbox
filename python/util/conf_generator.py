#!/usr/bin/env python
import sys
import os
import argparse
from os.path import dirname
from os.path import join

project_dir = dirname(dirname(dirname(os.path.realpath(__file__))))
sys.path.append(join(project_dir, 'build'))

# Path to protobuf build.
#sys.path.append(join(project_dir, 'third_party', 'include'))
sys.path.append(join('/home/wdai/lib', 'third_party', 'include'))

import transform.proto.transform_pb2 as transform_pb
import schema.proto.schema_pb2 as schema_pb
import google.protobuf.text_format as text_format

#output = join(project_dir, 'test', 'resource', 'test_transform1.conf')

class TransformConfig:
  def __init__(self):
    self.config_list = transform_pb.TransformConfigList()

  def add_bucketize(self, feature, buckets, output_family=None):
    """
    Example:
    config = TransformConfig()
    config.add_bucketize_transform(':3', 'bucketize1',
        [float('-inf'), 0, 1, 2, float('inf')])
    """
    new_config = self.config_list.transform_configs.add()
    new_config.base_config.input_features.append(feature)
    if output_family:
      new_config.base_config.output_family = output_family
    new_config.bucketize_transform.buckets.extend(buckets)

  def add_constant(self, constant_val):
    new_config = self.config_list.transform_configs.add()
    new_config.constant_transform.constant = constant_val

  def add_onehot(self, selector, output_family=None):
    new_config = self.config_list.transform_configs.add()
    new_config.base_config.input_features.append(selector)
    if output_family:
      new_config.base_config.output_family = output_family
    new_config.one_hot_transform.SetInParent()

  def add_select(self, selector, output_family=None,
      output_store_type=None):
    new_config = self.config_list.transform_configs.add()
    new_config.base_config.input_features.append(selector)
    if output_family:
      new_config.base_config.output_family = output_family
    if output_store_type:
      new_config.base_config.output_store_type = output_store_type
    # Because SelectTransform is an empty field, use SetInParent() to set it.
    new_config.select_transform.SetInParent()

  def add_ngram(self, family1, family2):
    new_config = self.config_list.transform_configs.add()
    new_config.base_config.input_features.append('%s:*,%s:*' % (family1,
      family2))
    # Because SelectTransform is an empty field, use SetInParent() to set it.
    new_config.ngram_transform.SetInParent()

  def add_normalize(self, family):
    new_config = self.config_list.transform_configs.add()
    new_config.base_config.input_features.append('%s:*' % family))
    # Because SelectTransform is an empty field, use SetInParent() to set it.
    new_config.normalize_transform.SetInParent()

  def add_tf(self, selector, graph_path, weight_path, output_vars,
    output_family=None):
    new_config = self.config_list.transform_configs.add()
    new_config.base_config.input_features.append(selector)
    new_config.tf_transform.graph_path = graph_path
    new_config.tf_transform.weight_path = weight_path
    new_config.tf_transform.output_vars.extend(output_vars)

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument("--output")
  args = parser.parse_args()
  if not args.output:
    print('--output must be specified (a .conf file).')
    sys.exit(1)

  config = TransformConfig()
  """
  config.add_bucketize('3', [float('-inf'), 0, 1, 2, float('inf')],
      output_family='bucket1')
  config.add_constant(3.15)
  config.add_select('default:1,2', output_family="bucket1",
      output_store_type=schema_pb.SPARSE_NUM)
  #AddSelectTransform(config_list, 'default:80,83', output_family="tmp_family2",
  #    output_store_type=schema_pb.SPARSE_NUM)
  #AddSelectTransform(config_list, 'tmp_family:*')
  #AddSelectTransform(config_list, 'tmp_family2:*')
  config.add_onehot('customer:region', output_family='onehot-region')
  config.add_ngram('bucket1', 'onehot-region')
  """
  graph_path = '/users/wdai/hotbox/test/resource/tf_models/higgs.pb'
  weight_path = '/users/wdai/hotbox/test/resource/tf_models/higgs-29'
  config.add_tf('default:0-27', graph_path, weight_path, \
    ['dnn/FullyConnected_2/Relu:0', 'dnn/FullyConnected_3/Relu:0'], \
    output_family='dnn')
  config_list_str = text_format.MessageToString(config.config_list)
  with open(args.output, 'w') as f:
    f.write(config_list_str)
  print('Output to %s' % args.output)

