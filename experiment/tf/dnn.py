#!/usr/bin/env python

from __future__ import print_function
import tensorflow as tf
import numpy as np
import argparse
import json
import os
import time
import sys

import tensorflow as tf
import tflearn
import tflearn.initializations as tfi
import tflearn.data_flow
import tflearn.helpers as tfh

parser = argparse.ArgumentParser()
parser.add_argument('--data', type=str, default='')
args = parser.parse_args()
assert(args.data != '')

training = True
batch_size = 100
num_batches_per_eval = 1000

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

net_config = [
  {'name': 'dense', 'num_outputs': 500}
  #, {'name': 'bn'}
  , {'name': 'dense', 'num_outputs': 500}
  #, {'name': 'bn'}
  , {'name': 'dense', 'num_outputs': 500}
  #, {'name': 'bn'}
  , {'name': 'dense', 'num_outputs': 500}
  #, {'name': 'bn'}
  , {'name': 'dense', 'num_outputs': 500}
  #, {'name': 'bn'}
  # output = True to output as feature.
  , {'name': 'dense', 'num_outputs': 500, 'output_feature': True}
  #, {'name': 'bn'}
  , {'name': 'dense', 'num_outputs': 2, 'output_feature': True}
  ]

num_epochs = 30

# Read the meta file
with open(args.data + '.meta.json') as meta_file:
  meta = json.load(meta_file)
feature_dim = meta['feature_dim']
feature_one_based = meta['feature_one_based']

x_ = tf.placeholder(tf.float32, shape=[None, feature_dim], name='x')
x = x_

output_features = []
with tf.variable_scope("dnn") as scope:
  for c in net_config:
    if c['name'] == 'dense':
      x = tflearn.fully_connected(x, c['num_outputs'], activation='relu',
                                  weight_decay=0.0001,
                                  weights_init='xavier',
                                  bias_init='xavier')
    elif c['name'] == 'bn':
      x = tflearn.layers.normalization.batch_normalization(x)

    if 'output_feature' in c and c['output_feature']:
      output_features.append(x)
  if training:
    logit = x
    y_ = tf.placeholder(tf.int32, shape=[None,], name='y')
    cross_entropy = tf.reduce_mean(
        tf.nn.sparse_softmax_cross_entropy_with_logits(logit, y_))
    correct_pred = tf.equal(tf.cast(tf.argmax(logit, 1), tf.int32), y_)
    accuracy = tf.reduce_mean(tf.cast(correct_pred, tf.float32))

start_time = time.time()
with open(args.data, 'r') as f:
  lines = f.readlines()
  X = np.zeros((len(lines), feature_dim))
  y = np.zeros((len(lines), ))
  for i, line in enumerate(lines):
    y[i] = parse_libsvm_line(line, X[i], feature_one_based)
print('Read', X.shape[0], 'data from', args.data, 'time:', \
  time.time() - start_time)

model_dir = 'models/'
exp_name = 'higgs_full'

def shuffle_in_unison_inplace(a, b):
  """
  Shuffle along the first idx of a and b with same permutation.

  a, b: np.ndarray or list of the same first dimension.
  """
  assert len(a) == len(b)
  p = np.random.permutation(len(a))
  return a[p], b[p]

if training:
  # Training session
  #optimizer = tf.train.AdamOptimizer(learning_rate=0.1).minimize(cross_entropy)
  #optimizer = tf.train.AdadeltaOptimizer().minimize(cross_entropy)
  optimizer = tf.train.GradientDescentOptimizer(
    learning_rate=0.1).minimize(cross_entropy)
  #optimizer = tf.train.MomentumOptimizer(learning_rate=0.1, momentum=0.9,
  #  use_nesterov=True).minimize(cross_entropy)
  start_time = time.time()
  num_batches = X.shape[0] // batch_size
  print('num_batches', num_batches)
  with tf.Session() as sess:
    sess.run(tf.initialize_all_variables())
    saver = tf.train.Saver(max_to_keep=1)
    for e in range(num_epochs):
      X, y = shuffle_in_unison_inplace(X, y)
      batch_loss = 0.
      batch_acc = 0.
      for b in range(num_batches):
        begin = b * batch_size
        end = begin + batch_size
        _, loss, acc = sess.run([optimizer, cross_entropy, accuracy],
            feed_dict={x_: X[begin:end,:], y_: y[begin:end]})
        batch_loss += loss
        batch_acc += acc
        if b % num_batches_per_eval == 0:
          print('Epoch', e, 'batch', b, \
            'train_loss: %.3f' % (batch_loss / (b+1)), \
            'train_acc: %.3f' % (batch_acc / (b+1)), \
            'time: %.3f' % (time.time()-start_time))
      save_path = saver.save(sess, os.path.join(model_dir, exp_name),\
        global_step=e+1)
      print('Model saved to', save_path)

    saver_def = saver.as_saver_def()
    print('filename_tensor_name:', saver_def.filename_tensor_name)
    print('restore_op_name:', saver_def.restore_op_name)
    print('save_tensor_name:', saver_def.save_tensor_name)

else:
  # Feature extraction session
  for x in output_features:
    print(x.name)
  model_file = exp_name + '.pb'
  saver = tf.train.Saver()
  with tf.Session() as sess:
    saver.restore(sess, "models/higgs-%d" % num_epochs)
    sess.run(tf.initialize_all_variables())
    features = sess.run(output_features, feed_dict={x_: X})
    print('features:', features[0])
    tf.train.write_graph(sess.graph_def, model_dir, model_file, as_text=False)
  print('Wrote graph pb to', os.path.join(model_dir, model_file))
