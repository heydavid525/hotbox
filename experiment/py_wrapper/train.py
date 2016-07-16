from keras.models import Sequential
from keras.layers import Dense, Activation, Dropout, BatchNormalization
import numpy as np
from keras.models import model_from_json
from keras.regularizers import l2
from keras import backend
import sys
import time
import os
from os.path import dirname
from os.path import join

def getData(session, start, end):
	data = session.GetData(start, end)
	num_data = len(data[0])
	label = data[0]
	train_sparse = data[1]
	train_dense = []
	for i in range(num_data):
		datum = [0 for j in range(dim)]
		for k in train_sparse[i]:
			datum[k] = train_sparse[i][k]
		train_dense.append(datum)
	return train_dense, label

file_dir = dirname(os.path.realpath(__file__))
sys.path.append(join(file_dir, 'build'))
import py_hb_wrapper
dim = 29
client = py_hb_wrapper.PYClient()
options = dict()
options["db_name"] = "HIGGS"
options["session_id"] = "higgs_session"
options["transform_config_path"] = "/home/ubuntu/github/hotbox/test/resource/select_all.conf"
options["type"] = "sparse"
session = client.CreateSession(options)

dropout = 0.2
num_hidden_layer = 3
model = Sequential()
model.add(Dense(output_dim=200, input_dim=dim, init="glorot_uniform", W_regularizer=l2(0.01), activation='relu'))
model.add(BatchNormalization())
model.add(Dropout(dropout))
for i in range(num_hidden_layer):
	model.add(Dense(output_dim=200, init="glorot_uniform", W_regularizer=l2(0.01), activation='relu'))
	model.add(BatchNormalization())
	model.add(Dropout(dropout))
model.add(Dense(1, activation="sigmoid"))
model.compile(optimizer='adam', loss='binary_crossentropy', metrics=['accuracy'])
print("compiled")

get_batch_size = 1000000
batch_size = 200
offset = 0
last_save = int(time.time())
while(True):
	train_dense, label = getData(session, offset, offset + get_batch_size)
	num_data = len(label)
	offset = offset + num_data
	print(num_data)
	for i in range(num_data / batch_size):
		train_array = np.array(train_dense[i * batch_size : (i + 1) * batch_size])
		label_array = np.array(label[i * batch_size : (i + 1) * batch_size]).reshape(batch_size, 1)
		history = model.train_on_batch(train_array, label_array)	
		if(int(time.time()) - last_save > 300):
			print(history)
			model.save_weights("weight", overwrite=True)
			m = model.to_json()
			file_object = open('model.json', 'w')
			file_object.write(m)
			file_object.close()
			last_save = int(time.time())
			print("saved")