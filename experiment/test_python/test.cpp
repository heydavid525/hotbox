#include <string>
#include <iostream>
#include <boost/python.hpp>
#include <thread>
#include <chrono>

using namespace std;
using namespace boost::python;
int main() {
try{
  Py_Initialize();
  char* argv[] = {"hotbox_dnn_transform"};
  PySys_SetArgv(1, argv);
  
  cout << 1 << endl;
  object main_module_ = import("__main__");
  cout << 2 << endl;
  object main_namespace_ = main_module_.attr("__dict__");
  cout << 3 << endl;
  string model_path = "../../../test/resource/dataset/model.json";
  string weight_path = "../../../test/resource/dataset/weight";
  exec("from keras.models import Sequential\n"
		"from keras.layers import Dense, Activation\n"
		"import numpy as np\n"
		"from keras.models import model_from_json\n"
		"from keras import backend\n",
  	  main_namespace_);
  exec("def get_activations(model, layer, X_batch):\n"
      "\tget_activations = backend.function([model.layers[0].input, backend.learning_phase()], [model.layers[layer].output,])\n"
      "\tactivations = get_activations([np.array([X_batch]),0])\n"
      "\treturn activations[0][0].tolist()\n",
	  main_namespace_);
  exec("def get_model(model_path, weight_path):\n"
	  "\tfile = open(model_path, 'r')\n"
	  "\tmodel_json = file.read()\n"
	  "\tmodel = model_from_json(model_json)\n"
	  "\tmodel.load_weights(weight_path)\n"
	  "\treturn model\n",
	  main_namespace_);
  exec(str("model = get_model('" + model_path + "','" + weight_path + "')\n"),
	  main_namespace_);
  object func = main_module_.attr("get_activations");
  object model = main_module_.attr("model");
  int num_layers = extract<int>(eval("len(model.layers)", main_namespace_));
  cout << "num layers: " << num_layers << endl;
  Py_Finalize();
}
catch (error_already_set const&){
  PyErr_Print();
}
  std::cout << "Done" << std::endl;
  return 0;
}
