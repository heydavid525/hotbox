#include <boost/python.hpp>
#include <vector>

#include "client/py_client/py_session.hpp"
#include "client/data_iterator.hpp"
#include "schema/all.hpp"
#include "db/proto/db.pb.h"


using namespace boost::python;
using namespace std;

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
/*  
  PYDataIterator PYSession::NewDataIterator(BigInt data_begin = 0,
  	BigInt data_end = -1) const{
	DataIterator di = session_->NewDataIterator(data_begin, data_end);
	return PYDataIterator(di);
  }
*/

  boost::python::list PYSession::GetData(BigInt begin, BigInt end){
	DataIterator it = session_->NewDataIterator(begin, end);
	vector<vector<float>> dense_res;
	vector<dict> sparse_res;
	bool is_datum_dense;
	for(; it.HasNext(); it.Next()){
		FlexiDatum datum = it.GetDatum();
		is_datum_dense = datum.isDense();
		if(is_datum_dense){
			vector<float> dense_vals = datum.GetDenseStore();
			dense_res.push_back(dense_vals);
		}
		else{
			vector<BigInt> sparse_idx = datum.GetSparseIdx();
			vector<float> sparse_val = datum.GetSparseVals();
			dict sparse_datum;
			for (int i = 0; i < sparse_idx.size(); ++i) {
				sparse_datum[sparse_idx[i]] = sparse_val[i];
			}
			sparse_res.push_back(sparse_datum);
		}
	}
	if(is_datum_dense){
		boost::python::object get_iter = boost::python::iterator<std::vector<vector<float> > >();
	    boost::python::object iter = get_iter(dense_res);
	    return boost::python::list(iter);
	}
	else{
		boost::python::object get_iter = boost::python::iterator<std::vector<dict > >();
	    boost::python::object iter = get_iter(sparse_res);
	    return boost::python::list(iter);
	}
  }
	  
}

