import sys
import os
from os.path import dirname
from os.path import join

project_dir = dirname(dirname(dirname(os.path.realpath(__file__))))
sys.path.append(join(project_dir, 'build'))
sys.path.append(join(project_dir, 'third_party', 'include'))

import transform.proto.transform_pb2 as transform_pb
import google.protobuf.text_format as text_format

output = join(project_dir, 'test', 'resource', 'test_transform1.conf')

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

if __name__ == '__main__':
  config_list = transform_pb.TransformConfigList()
  AddBucketizeTransform(config_list, '3', 'bucketize1',
      [float('-inf'), 0, 1, 2, float('inf')])
  AddBucketizeTransform(config_list, '67', 'bucketize2',
      [0, 1, float('inf')])
  config_list_str = text_format.MessageToString(config_list)
  with open(output, 'w') as f:
    f.write(config_list_str)
  print('Output to %s' % output)

