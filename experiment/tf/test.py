#!/usr/bin/env python
from __future__ import print_function
import sys, os, time
from keras.models import Sequential
from keras.layers import Dense, Activation
import numpy as np
from keras.models import model_from_json
from keras import backend

model_path = "/users/wdai/hotbox/test/resource/keras/model.json"
weight_path = "/users/wdai/hotbox/test/resource/keras/weight"

def get_activations(model, layer, X_batch):
  if not isinstance(model.layers[layer], Dense):
    return []
  get_activations = backend.function([model.layers[0].input, \
    backend.learning_phase()], [model.layers[layer].output,])
  #activations = get_activations([np.array([X_batch]),0])
  activations = get_activations([X_batch, 0])
  #return activations[0][0].tolist()
  return activations

def get_model(model_path, weight_path):
  with open(model_path, 'r') as f:
    model_json = f.read()
    model = model_from_json(model_json)
  model.load_weights(weight_path)
  return model


model = get_model(model_path, weight_path)
#print('input size:', model.layers[0].input)
N = 100
X = np.random.rand(N, 104)
start_time = time.time()
for i in range(N):
  f = get_activations(model, -1, np.expand_dims(X[i,:], 0))
print('transformed shape', len(f[0]), 'time:', time.time() - start_time)

print('num layers', len(model.layers))
