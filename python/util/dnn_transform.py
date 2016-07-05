from keras.models import Sequential
from keras.layers import Dense, Activation
import numpy as np
from keras.models import model_from_json
from keras import backend

def get_activations(model, layer, X_batch):
    get_activations = backend.function([model.layers[0].input, backend.learning_phase()], [model.layers[layer].output,])
    activations = get_activations([np.array([X_batch]),0])
    return activations
def get_model(model_path, weight_path):
	file = open(model_path, 'r')
	model_json = file.read()
	model = model_from_json(model_json)
	model.load_weights(weight_path)
	return model