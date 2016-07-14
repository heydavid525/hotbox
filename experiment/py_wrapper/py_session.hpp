#pragma once
#include <boost/python.hpp>

#include "client/session.hpp"
#include <iostream>
#include <vector>
using namespace boost::python;

namespace hotbox {
class PYSession {
	public:
	  PYSession(){};
	  PYSession(Session* ss);
	  ~PYSession();	
	  
	  BigInt GetNumData() const;

	  dict GetData(BigInt begin = 0, BigInt end = -1);	  
	
	private:
	  Session* session_;
	  
	  template<class T>
	  static list VectorToList(std::vector<T> vector) {
	    typename std::vector<T>::iterator iter;
	    list list;
	    for (iter = vector.begin(); iter != vector.end(); ++iter) {
	        list.append(*iter);
	    }
	    return list;
	  }
};
}
