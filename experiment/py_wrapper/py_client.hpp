#pragma once

#include <boost/python.hpp>

#include "client/hb_client.hpp"
#include "py_session.hpp"

using namespace boost::python;

namespace hotbox {
class PYClient{
	public:
		PYClient();
		PYSession const& CreateSession(dict options);
	private:
		HBClient* hbClient_;
};	
}
