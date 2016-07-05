#pragma once

#include "transform/transform_api.hpp"
#include "glog/logging.h"

#include <string>
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <mutex> 

using namespace boost::python;

namespace hotbox {

class PythonRuntimeWrapper{
public:
  PythonRuntimeWrapper(){
    LOG(INFO) << "initialize py runtime";
  	Py_Initialize();
	LOG(INFO) << "initialize py runtime finished";
  }
  ~PythonRuntimeWrapper(){
  	Py_Finalize();
  }
  std::mutex mtx;
};

class DnnTransform : public TransformIf {
public:
  void TransformSchema(const TransformParam& param,
      TransformWriter* writer) const override;

  std::function<void(TransDatum*)> GenerateTransform(
      const TransformParam& param) const override;
  
  static const std::vector<float>& GetDenseVals(TransDatum& datum);
  template<class T>
  static inline
  list VectorToList(const std::vector<T>& v){
    object get_iter = iterator<std::vector<T> >();
    object iter = get_iter(v);
    list l(iter);
    return l;
  }

private:
  void initialModel(const std::string model_path, 
  	const std::string weight_path) const;
      
  //static PythonRuntimeWrapper prw_;
  mutable object main_namespace_;
  mutable object main_module_;
};
} // namespace hotbox

