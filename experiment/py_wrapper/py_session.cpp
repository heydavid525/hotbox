#include <boost/python.hpp>
#include <vector>

#include "py_session.hpp"
#include "client/session_options.hpp"
#include "client/data_iterator.hpp"
#include "schema/all.hpp"
#include "db/proto/db.pb.h"


using namespace boost::python;

namespace hotbox {
  PYSession::PYSession(Session * ss){
	session_ = ss;
  }
  
  PYSession::~PYSession(){
	delete session_;
  }	
  
  BigInt PYSession::GetNumData() const{
	return session_->GetNumData();
  }
  
  dict PYSession::GetData(BigInt begin, BigInt end){
	DataIterator it = session_->NewDataIterator(begin, end);
	std::vector<std::vector<float>> dense_res;
	std::vector<dict> sparse_res;
	std::vector<float> labels;
	bool is_datum_dense;
	while(it.HasNext()){
		FlexiDatum datum = it.GetDatum();
		labels.push_back(datum.GetLabel());
		is_datum_dense = datum.isDense();
		if(is_datum_dense){
			std::vector<float> dense_vals = datum.GetDenseStore();
			dense_res.push_back(dense_vals);
		}
		else{
			std::vector<BigInt> sparse_idx = datum.GetSparseIdx();
			std::vector<float> sparse_val = datum.GetSparseVals();
			dict sparse_datum;
			for (int i = 0; i < sparse_idx.size(); ++i) {
				sparse_datum[sparse_idx[i]] = sparse_val[i];
			}
			sparse_res.push_back(sparse_datum);
		}
	}
	dict res;
	res[0] = VectorToList(labels);
	if(is_datum_dense){	
		res[1] = VectorToList(dense_res);
	}
	else{
		res[1] = VectorToList(sparse_res);
	}
	return res;
  }
	  
}

