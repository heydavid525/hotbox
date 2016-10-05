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
  	LOG(INFO) << "finalize py runtime";
  	Py_Finalize();
	LOG(INFO) << "finalize py runtime finished";
  }
};

class DnnTransform : public TransformIf {
public:
  void TransformSchema(const TransformParam& param,
      TransformWriter* writer) const override;

  std::function<void(TransDatum*)> GenerateTransform(
      const TransformParam& param) const override;

  template<class T>
  static list VectorToList(std::vector<T> vector) {
    typename std::vector<T>::iterator iter;
    boost::python::list list;
    for (iter = vector.begin(); iter != vector.end(); ++iter) {
        list.append(*iter);
    }
    return list;
}

private:
  void initialModel(const std::string model_path, 
  	const std::string weight_path) const;
      
  mutable object main_namespace_;
  mutable object main_module_;
};
} // namespace hotbox
