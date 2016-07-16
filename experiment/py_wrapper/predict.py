from keras.models import Sequential
from keras.layers import Dense, Activation, Dropout, BatchNormalization
import numpy as np
from keras.models import model_from_json
from keras.regularizers import l2
from keras import backend
import sklearn
import time
from sklearn.datasets import load_svmlight_file
import copy 

def get_activations(model, layer, X_batch):
	if not isinstance(model.layers[layer], Dense):
		return []
	get_activations = backend.function([model.layers[0].input, backend.learning_phase()], [model.layers[layer].output,])
	activations = get_activations([X_batch,0])
	return activations[0][0].tolist()
outfile = open('/home/ubuntu/data/transformed.txt', 'w')
start = int(time.time())
data, label = load_svmlight_file("/home/ubuntu/dataset/HIGGS", zero_based=True)
data = data.toarray()
num_data = len(label)
print(num_data)
model_file = open('/home/ubuntu/github/hotbox/test/resource/dataset/higgs_model.json', 'r')
model_json = model_file.read()
model = model_from_json(model_json)
model.load_weights("/home/ubuntu/github/hotbox/test/resource/dataset/higgs_weight")
print("model loaded")

batch_size = 10000
for offset in range((num_data / batch_size)):
	transformed = []
	for i in range(batch_size):
		it = offset * batch_size + i
		features = copy.deepcopy(data[it].tolist())
		for layer in range(len(model.layers)):
			activations = get_activations(model, layer, data[it].reshape(1, len(data[it])))
			features.extend(activations)
		outstr = str(label[it]) + " "
		for index in range(len(features)):
			outstr = outstr + str(index) + ":" + str(features[index]) + " "
		outstr = outstr.strip() + "\n"
		transformed.append(outstr)
	print("feature generated " + str(offset))
	outfile.writelines(transformed)

outfile.close()
end = int(time.time())
print("time: " + str(end - start))



