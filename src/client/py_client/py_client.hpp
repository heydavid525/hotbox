#pragma once

#include <boost/python.hpp>

#include "client/hb_client.hpp"
#include "client/session.hpp"
#include "client/session_options.hpp"
#include "client/py_client/py_session.hpp"

using namespace boost::python;

namespace hotbox {
class PYClient{
	public:
		PYClient();
		PYSession CreateSession(dict options);
	private:
		HBClient* hbClient_;
};	
}

