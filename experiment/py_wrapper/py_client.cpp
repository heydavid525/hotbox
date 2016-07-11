#include <boost/python.hpp>
#include <string>


#include "client/session.hpp"
#include "client/session_options.hpp"
#include "py_client.hpp"

using namespace boost::python;
using namespace std;

namespace hotbox {
	PYClient::PYClient()	{
		hbClient_ = new HBClient();
	}

	PYSession const& PYClient::CreateSession(dict options){
		SessionOptions so;
		so.db_name = extract<string>(options.get("db_name"));
		so.session_id = extract<string>(options.get("session_id"));
		so.transform_config_path = extract<string>(options.get("transform_config_path"));
		string store_type = extract<string>(options.get("type"));
		if(store_type.compare("sparse") == 0)
			so.output_store_type = OutputStoreType::SPARSE;
		else
			so.output_store_type = OutputStoreType::DENSE;
		Session* ss = hbClient_->CreateSessionPtr(so);
		return *(new PYSession(ss));
	}	
}


