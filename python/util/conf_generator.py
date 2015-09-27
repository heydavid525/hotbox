import sys
import os
from os.path import dirname
from os.path import join

project_dir = dirname(dirname(dirname(os.path.realpath(__file__))))
sys.path.append(join(project_dir, 'build'))
sys.path.append(join(project_dir, 'third_party', 'include'))

import transform.proto.transform_config_pb2 as transform_config_pb
import google.protobuf.text_format as text_format

output = join(project_dir, 'test', 'resource', 'test_transform1.conf')

def AddOneHotTransform(config_list):
  new_config = config_list.configs.add()
  new_config.one_hot_transform.input_features = ":1"
  new_config.one_hot_transform.output_feature_family = "one-hot1"
  new_config.one_hot_transform.buckets.extend([0, 1, 2])
  new_config.one_hot_transform.not_in_final = False

if __name__ == '__main__':
  config_list = transform_config_pb.TransformConfigList()
  AddOneHotTransform(config_list)
  config_list_str = text_format.MessageToString(config_list)
  with open(output, 'w') as f:
    f.write(config_list_str)
  print('Output to %s' % output)

