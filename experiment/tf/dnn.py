#!/usr/bin/env python

from __future__ import print_function
import tensorflow as tf
import numpy as np
import argparse
import json
import os
import time
import sys
from itertools import islice

import tensorflow as tf
import tflearn
import tflearn.initializations as tfi
import tflearn.data_flow
import tflearn.helpers as tfh

parser = argparse.ArgumentParser()
parser.add_argument('--train', type=str, default='')
parser.add_argument('--test', type=str, default='')
parser.add_argument('--exp', type=str, default='')
parser.add_argument('--training', dest='training', action='store_true')
parser.add_argument('--no-training', dest='training', action='store_false')
parser.set_defaults(training=True)
# path for generated features
parser.add_argument('--output', type=str, default='')
args = parser.parse_args()
assert(args.train != '')
#assert(args.test != '')
assert(args.exp != '')

training = args.training
test = args.test != ''
batch_size = 1000
#num_batches_per_eval = 1000
num_data_per_eval = 1000000

def parse_libsvm_line(line, x, feature_one_based=True):
  """
  Store parsed value to x:np.zeros of dim. Return label 1 (int)
  """
  fields = line.split(' ')
  for p in fields[1:]:
    a = p.split(':')
    idx = int(a[0])
    if feature_one_based:
      idx -= 1
    x[idx] = float(a[1])
  return int(fields[0])

def write_csv(f, X, sep=','):
  """
  x: np.array of shape [num_data, dim]
  """
  for i in range(X.shape[0]):
    x = X[i]
    line = sep.join(['%f' % v for v in x])
    f.write(line + '\n')

net_config = [
  {'name': 'dense', 'num_outputs': 500}
  , {'name': 'bn'}
  , {'name': 'dropout', 'rate': 0.2}
  , {'name': 'dense', 'num_outputs': 500}
  , {'name': 'bn'}
  , {'name': 'dropout', 'rate': 0.2}
  , {'name': 'dense', 'num_outputs': 500}
  , {'name': 'bn'}
  , {'name': 'dropout', 'rate': 0.2}
  , {'name': 'dense', 'num_outputs': 500}
  , {'name': 'bn'}
  , {'name': 'dropout', 'rate': 0.2}
  , {'name': 'dense', 'num_outputs': 500}
  , {'name': 'bn'}
  , {'name': 'dropout', 'rate': 0.2}
  # output = True to output as feature.
  , {'name': 'dense', 'num_outputs': 500, 'output_feature': True}
  , {'name': 'bn'}
  , {'name': 'dropout', 'rate': 0.2}
  , {'name': 'dense', 'num_outputs': 2, 'output_feature': True}
  ]

num_epochs = 1

# Read the meta file
with open(args.train + '.meta.json') as meta_file:
  meta = json.load(meta_file)
feature_dim = meta['feature_dim']
feature_one_based = meta['feature_one_based']

x_ = tf.placeholder(tf.float32, shape=[None, feature_dim], name='x')
net = x_

output_features = []
with tf.variable_scope("dnn") as scope:
  for c in net_config:
    if c['name'] == 'dense':
      net = tflearn.fully_connected(net, c['num_outputs'], activation='relu',
                                  weight_decay=0.0001,
                                  weights_init='xavier',
                                  bias_init='xavier')
    elif c['name'] == 'bn':
      net = tflearn.layers.normalization.batch_normalization(net)

    elif c['name'] == 'dropout':
      net = tflearn.dropout(net, 1 - c['rate'])

    if 'output_feature' in c and c['output_feature']:
      output_features.append(net)
  if training:
    logit = net
    y_ = tf.placeholder(tf.int32, shape=[None,], name='y')
    cross_entropy = tf.reduce_mean(
        tf.nn.sparse_softmax_cross_entropy_with_logits(logit, y_))
    correct_pred = tf.equal(tf.cast(tf.argmax(logit, 1), tf.int32), y_)
    accuracy = tf.reduce_mean(tf.cast(correct_pred, tf.float32))

if test:
  start_time = time.time()
  with open(args.test, 'r') as f:
    lines = f.readlines()
    Xtest = np.zeros((len(lines), feature_dim))
    ytest = np.zeros((len(lines), ))
    for i, line in enumerate(lines):
      ytest[i] = parse_libsvm_line(line, Xtest[i], feature_one_based)
  print('Read', Xtest.shape[0], 'data from', args.test, 'time:', \
    time.time() - start_time)



def split_every(n, iterable):
  i = iter(iterable)
  piece = list(islice(i, n))
  while piece:
    yield piece
    piece = list(islice(i, n))

class StreamReader(object):
  def __init__(self, path, feature_dim, batch_size=100,
  feature_one_based=True):
    self.__dict__.update(locals())
    self.has_file = False
    self.restart()

  def __iter__(self):
    return self

  def next(self):
    return self.__next__()

  def __next__(self):
    lines = self.line_segs.next()
    X = np.zeros((len(lines), self.feature_dim))
    y = np.zeros((len(lines), ))
    num_lines_read = 0
    for i, line in enumerate(lines):
      y[i] = parse_libsvm_line(line, X[i], self.feature_one_based)
      if i == batch_size:
        break
      num_lines_read += 1
    if num_lines_read < self.batch_size:
      raise StopIteration
    return X, y

  def restart(self):
    if self.has_file and not self.f.closed:
      self.f.close()
    self.f = open(self.path, 'r')
    self.has_file = True
    self.line_segs = split_every(self.batch_size, \
      self.f.readlines())

  def __del__(self):
    if self.has_file and not self.f.closed:
      self.f.close()

reader = StreamReader(args.train, feature_dim, batch_size=batch_size,
  feature_one_based=feature_one_based)

start_time = time.time()
#print('Read', X.shape[0], 'data from', args.data, 'time:', \
#  time.time() - start_time)

#exp_name = 'higgs_small'
exp_name = args.exp
model_dir = os.path.join('models', exp_name)
if not os.path.exists(model_dir):
  os.makedirs(model_dir)

def shuffle_in_unison_inplace(a, b):
  """
  Shuffle along the first idx of a and b with same permutation.

  a, b: np.ndarray or list of the same first dimension.
  """
  assert len(a) == len(b)
  p = np.random.permutation(len(a))
  return a[p], b[p]

model_file = exp_name + '.pb'
if training:
  # Training session
  #optimizer = tf.train.AdamOptimizer().minimize(cross_entropy)
  #optimizer = tf.train.AdadeltaOptimizer().minimize(cross_entropy)
  optimizer = tf.train.GradientDescentOptimizer(
    learning_rate=0.1).minimize(cross_entropy)
  #optimizer = tf.train.MomentumOptimizer(learning_rate=0.1, momentum=0.9,
  #  use_nesterov=True).minimize(cross_entropy)
  start_time = time.time()
  #num_batches = X.shape[0] // batch_size
  #print('num_batches', num_batches)
  with tf.Session() as sess:
    sess.run(tf.initialize_all_variables())
    saver = tf.train.Saver(max_to_keep=1)

    saver_def = saver.as_saver_def()
    print('filename_tensor_name:', saver_def.filename_tensor_name)
    print('restore_op_name:', saver_def.restore_op_name)
    print('save_tensor_name:', saver_def.save_tensor_name)

    num_data_seen = 0
    for e in range(num_epochs):
      batch_loss = 0.
      batch_acc = 0.
      reader.restart()
      batch_cnt = 0
      for X, y in reader:
        _, loss, acc = sess.run([optimizer, cross_entropy, accuracy],
            feed_dict={x_: X, y_: y})
        batch_loss += loss
        batch_acc += acc
        batch_cnt += 1
        num_data_seen += X.shape[0]
        if num_data_seen % num_data_per_eval == 0:
          if test:
            loss, acc = sess.run([cross_entropy, accuracy], \
              feed_dict={x_: Xtest, y_: ytest})
          else:
            loss, acc = 0, 0
          print('Num data seen', num_data_seen, \
            'train_loss: %.3f' % (batch_loss / (batch_cnt+1)), \
            'train_acc: %.3f' % (batch_acc / (batch_cnt+1)), \
            'test_loss: %.3f' % loss, \
            'test_acc: %.3f' % acc, \
            'time: %.3f' % (time.time()-start_time))
      if e == 0:
        tf.train.write_graph(sess.graph_def, model_dir, model_file, \
          as_text=False)
        print('Wrote graph pb to', os.path.join(model_dir, model_file))
      save_path = saver.save(sess, os.path.join(model_dir, exp_name),\
        global_step=e+1)
      print('Model saved to', save_path)


else:
  # Feature extraction session
  for x in output_features:
    print(x.name)
  assert args.output != ''
  saver = tf.train.Saver()
  print('Output features to', args.output)
  start_time = time.time()
  chpt_file = os.path.join(model_dir, 'higgs_full_bn2-5')
  with tf.Session() as sess, open(args.output, 'w') as f:
    saver.restore(sess, chpt_file)
    sess.run(tf.initialize_all_variables())
    for X, _ in reader:
      features = sess.run(output_features, feed_dict={x_: X})
      write_csv(f, features[0], ' ')
    #print('features.shape:', features[0].shape)
  print('Output to', args.output, 'Time:', time.time() - start_time)
