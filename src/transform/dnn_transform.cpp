#include <glog/logging.h>
#include "transform/transform_api.hpp"
#include "transform/dnn_transform.hpp"
#include <sstream>
#include "util/util.hpp"
#include "schema/flexi_datum.hpp"


namespace hotbox {
//PythonRuntimeWrapper DnnTransform::prw_;
/*
void DnnTransform::initialModel(const std::string model_path, const std::string weight_path) const{
  LOG(INFO) << "start initial model";
  char* argv[] = {"hotbox_dnn_transform"};
  PySys_SetArgv(1, argv);
  main_module_ = import("__main__");
  main_namespace_ = main_module_.attr("__dict__");
  try{
  LOG(INFO) << 1;
  exec("from keras.models import Sequential\n"
    "from keras.layers import Dense, Activation\n"
    "import numpy\n"
    "from keras.models import model_from_json\n"
    "from keras import backend\n",
    main_namespace_
    );
  LOG(INFO) << 2;
  exec(str("file_object = open('" + model_path + "', 'r')\n"), main_namespace_);
  LOG(INFO) << 3;
  exec("model_json = file_object.read()\n", main_namespace_);
  LOG(INFO) << 4;
  exec("model1 = Sequential()\n", main_namespace_);
  LOG(INFO) << 5;
  exec("model1.add(Dense(256, input_dim=104))\n", main_namespace_);
  LOG(INFO) << 6;
  exec("model1.add(Dense(1))\n", main_namespace_);

  LOG(INFO) << 7;
  exec("model_json1 = model1.to_json()\n"
	"print(model_json1)\n", main_namespace_);
  LOG(INFO) << 8;
  exec("model = model_from_json(model_json1)\n", main_namespace_);
  LOG(INFO) << 9;
  exec(str("model.load_weights('" + weight_path+ "')\n"), main_namespace_);
  
  //exec(str("file_object = open('" + model_path + "', 'r')\n"
  	//"model_json = file_object.read()\n"
  	//"model = model_from_json(model_json)\n"
  	//"model.load_weights('" + weight_path+ "')\n"),
  	//main_namespace_
  	//);
  LOG(INFO) << 3;
  exec("count = 0\n"
  	"for layer in model.layers:\n"
  	"\tcount = count + layer.output_dim\n",
  	main_namespace_
  	);
  LOG(INFO) << 4;
  exec("def get_activations(model, layer, X_batch):\n"
    "\tget_activations = backend.function([model.layers[0].input, backend.learning_phase()], [model.layers[layer].output,])\n"
    "\tactivations = get_activations([numpy.array([X_batch]),0])\n"
    "\treturn activations\n",
    main_namespace_
    );
  LOG(INFO) << 5;
  }
  catch (error_already_set const&){
  PyErr_Print();
  }
  LOG(INFO) << "model initial finish";
  return ;
}
*/
void DnnTransform::initialModel(const std::string model_path, const std::string weight_path) const{
  LOG(INFO) << "start initial model";
  char* argv[] = {"hotbox_dnn_transform"};
  PySys_SetArgv(1, argv);
  main_module_ = import("__main__");
  main_namespace_ = main_module_.attr("__dict__");
  try{
  exec(str("import sys\n"
  	"sys.path.append('/home/wanghy/github/hotbox/python/util/')\n"
  	"import dnn_transform as dt\n"),
  	main_namespace_);
  LOG(INFO) << 111;
  exec(str("get_model = dt.get_model('" + model_path + "','" + weight_path + "')\n"
  	"get_activations = dt.get_activations\n"),
  	main_namespace_);
  LOG(INFO) << 222;
  }  
  catch (error_already_set const&){
  	PyErr_Print();
  }
}

const std::vector<float>& DnnTransform::GetDenseVals(TransDatum& datum) {
  static std::vector<float> dense_vals;
  FlexiDatum flexi = datum.GetFlexiDatum();
  if(flexi.isDense()){
  	return flexi.GetDenseStore();
  }
  else{
  	if(dense_vals.size() <= datum.GetFlexiDatum().GetFeatureDim()){
	  dense_vals.resize(datum.GetFlexiDatum().GetFeatureDim());
	}
	std::vector<BigInt> inx = datum.GetFlexiDatum().GetSparseIdx();
	std::vector<float> vals = datum.GetFlexiDatum().GetSparseVals();
	for(BigInt i = 0; i < datum.GetFlexiDatum().GetFeatureDim(); i++){
	  dense_vals[i] = 0;
	}
	for(size_t i = 0; i < inx.size(); i++){
	  dense_vals[inx[i]] = vals[i];
	}
  }
  //LOG(INFO) << "dense vals:" << dense_vals;
  return dense_vals;
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
  static PythonRuntimeWrapper prw_;
  {
  	std::lock_guard<std::mutex> lock(prw_.mtx);
  	initialModel(model_path, weight_path);
  }
  object func = main_module_.attr("get_activations");
  object model = main_module_.attr("model");
  int num_layers = extract<int>(eval("len(model.layers)", main_namespace_));
  LOG(INFO) << "num layers: " << num_layers;
  //auto main_namespace = main_namespace_;
  //auto main_module = main_module_;
  return [func, model, num_layers] (TransDatum *datum) {	  
	  int offset = 0;
	  //main_namespace["features"] = VectorToList(DnnTransform::GetDenseVals(*datum));
	  list features = VectorToList(DnnTransform::GetDenseVals(*datum));
	  
	  for(int i = 0; i < num_layers; i++){
	  	//str cmd = str("activations = get_activations(model, " + std::to_string(i) + ", numpy.array([features]))");
		//exec(cmd, main_namespace);
		//list activations = extract<list>(main_namespace["activations"]);
		object activations = func(model, i, features);
		auto length = len(activations);
		for (int j = 0; j < length; ++j) {
	        float val = boost::python::extract<float>(activations[j]);
			datum->SetFeatureValRelativeOffset(offset, val);
			offset++;
	    }
	  }
    };
}

} // namespace hotbox

