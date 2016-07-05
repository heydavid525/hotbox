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
  string model_path = "../../test/resource/dataset/model.json";
  string weight_path = "../../test/resource/dataset/weight";
  exec(str("import sys\n"
    "sys.path.append('/home/wanghy/github/hotbox/python/util/')\n"
    "import dnn_transform as dt\n"),
    main_namespace_);
  exec(str("model = dt.get_model('" + model_path + "','" + weight_path + "')\n"
    "get_activations = dt.get_activations\n"),
    main_namespace_);
  
  Py_Finalize();
}
catch (error_already_set const&){
  PyErr_Print();
}
  return 0;
}
