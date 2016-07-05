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

def AddBucketizeTransform(config_list, feature, output_family_name, buckets):
  """
  Example:
  config_list = transform_pb.TransformConfigList()
  AddBucketizeTransform(config_list, ':3', 'bucketize1',
      [float('-inf'), 0, 1, 2, float('inf')])
  """
  new_config = config_list.transform_configs.add()
  new_config.base_config.input_features.append(feature)
  new_config.base_config.output_family = output_family_name
  new_config.bucketize_transform.buckets.extend(buckets)

def AddConstantTransform(config_list, constant_val):
  new_config = config_list.transform_configs.add()
  new_config.constant_transform.constant = constant_val

def AddOnehotTransform(config_list, selector):
  new_config = config_list.transform_configs.add()
  new_config.base_config.input_features.append(selector)
  new_config.one_hot_transform.SetInParent()

def AddSelectTransform(config_list, selector, output_family=None,
    output_store_type=None):
  new_config = config_list.transform_configs.add()
  new_config.base_config.input_features.append(selector)
  if output_family:
    new_config.base_config.output_family = output_family
  if output_store_type:
    new_config.base_config.output_store_type = output_store_type
  # Because SelectTransform is an empty field, use SetInParent() to set it.
  new_config.select_transform.SetInParent()

def AddNgramTransform(config_list, family1, family2):
  new_config = config_list.transform_configs.add()
  new_config.base_config.input_features.append('%s:*,%s:*' % (family1,
    family2))
  # Because SelectTransform is an empty field, use SetInParent() to set it.
  new_config.ngram_transform.SetInParent()

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument("--output")
  args = parser.parse_args()
  if not args.output:
    print('--output must be specified (a .conf file).')
    sys.exit(1)

  config_list = transform_pb.TransformConfigList()
  #AddBucketizeTransform(config_list, '3', 'bucketize1',
  #    [float('-inf'), 0, 1, 2, float('inf')])
  #AddBucketizeTransform(config_list, '67', 'bucketize2',
  #    [0, 1, float('inf')])
  #AddConstantTransform(config_list, 3.15)
  #AddSelectTransform(config_list, 'default:1,2', output_family="tmp_family",
  #    output_store_type=schema_pb.SPARSE_NUM)
  #AddSelectTransform(config_list, 'default:80,83', output_family="tmp_family2",
  #    output_store_type=schema_pb.SPARSE_NUM)
  #AddSelectTransform(config_list, 'tmp_family:*')
  #AddSelectTransform(config_list, 'tmp_family2:*')
  AddOnehotTransform(config_list, 'tmp:1')
  #AddNgramTransform(config_list, 'tmp_family', 'tmp_family2')
  config_list_str = text_format.MessageToString(config_list)
  with open(args.output, 'w') as f:
    f.write(config_list_str)
  print('Output to %s' % args.output)

