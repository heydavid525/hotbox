#include <glog/logging.h>
#include "transform/transform_api.hpp"
#include "transform/dnn_transform.hpp"
#include "transform/transform_util.hpp"
#include <sstream>
#include "util/util.hpp"
#include "schema/flexi_datum.hpp"

namespace hotbox {
//PythonRuntimeWrapper DnnTransform::prw_;
void DnnTransform::initialModel(const std::string model_path, const std::string weight_path) const{
  LOG(INFO) << "start initial model";
  char* argv[] = {"hotbox_dnn_transform"};
  PySys_SetArgv(1, argv);
  main_module_ = import("__main__");
  main_namespace_ = main_module_.attr("__dict__");
  try{
   exec("import numpy as np\n", main_namespace_);
    exec("from keras.models import Sequential\n"
		"from keras.layers import Dense, Activation\n"
		"import numpy as np\n"
		"from keras.models import model_from_json\n"
		"from keras import backend\n",
  	  main_namespace_);
	exec("def get_activations(model, layer, X_batch):\n"
	    "\tif not isinstance(model.layers[layer], Dense):\n"
	    "\t\treturn []\n"
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
  LOG(INFO) << "Finally!!!!!!";
  }  
  catch (error_already_set const&){
  	PyErr_Print();
  }
}

void DnnTransform::TransformSchema(const TransformParam& param,
    TransformWriter* writer) const {
  const DnnTransformConfig& config = param.GetConfig().dnn_transform();
  BigInt num_neurals = config.num_neurals();
  writer->AddFeatures(num_neurals);
}

std::function<void(TransDatum*)> DnnTransform::GenerateTransform(
    const TransformParam& param) const {
  const DnnTransformConfig& config = param.GetConfig().dnn_transform();
  std::string model_path = config.model_path();
  std::string weight_path = config.weight_path();
  const std::multimap<std::string, WideFamilySelector>& w_family_selectors
    = param.GetWideFamilySelectors();
  CHECK_EQ(1, w_family_selectors.size());
  WideFamilySelector selector = w_family_selectors.begin()->second;
  PythonRuntimeWrapper *prw_ = new PythonRuntimeWrapper();
  
  initialModel(model_path, weight_path);
  object func = main_module_.attr("get_activations");
  object model = main_module_.attr("model");
  int num_layers = extract<int>(eval("len(model.layers)", main_namespace_));
  LOG(INFO) << "num layers: " << num_layers;
  
  return [selector, func, model, num_layers] (TransDatum* datum) {
  	try{
	  int offset = 0;
	  list features = VectorToList(GetDenseVals(*datum, selector));
	  for(int i = 0; i < num_layers; i++){
		object activations = func(model, i, features);
		auto length = len(activations);
		for (int j = 0; j < length; ++j) {
	        float val = boost::python::extract<float>(activations[j]);
			datum->SetFeatureValRelativeOffset(offset, val);
			offset++;
	    }
	  }
	}  
    catch (error_already_set const&){
      PyErr_Print();
    }
  };
}
} // namespace hotbox

