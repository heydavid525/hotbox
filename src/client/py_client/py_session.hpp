#pragma once

#include <boost/python.hpp>

#include "client/session.hpp"
#include "client/session_options.hpp"

using namespace boost::python;

namespace hotbox {
class PYSession {
	public:
	  PYSession(){};
	  PYSession(Session* ss);
	  ~PYSession();	
	  
	  BigInt GetNumData() const;

	  list GetData(BigInt begin = 0, BigInt end = -1);	  
	
	private:
	  Session* session_;
};

}


