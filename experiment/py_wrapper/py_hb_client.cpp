#include<boost/python.hpp>

#include "py_session.hpp"
#include "py_client.hpp"

using namespace boost::python;
using namespace hotbox;

BOOST_PYTHON_MODULE(py_hb_wrapper)
{
	class_<PYClient>("PYClient")
		.def("CreateSession", &PYClient::CreateSession, return_value_policy<reference_existing_object>());
	
	class_<PYSession>("PYSession", no_init)
		.def("GetNumData", &PYSession::GetNumData)
		.def("GetData", &PYSession::GetData);
	/*
	
	class_<SessionOptions>("SessionOptions")
		.def_readwrite("db_name", &SessionOptions::db_name)
		.def_readwrite("session_id", &SessionOptions::session_id)
		.def_readwrite("transform_config_path", &SessionOptions::transform_config_path);
	
	class_<Status>("Status")
		.def("ok", &Status::ok)
		.def("ToString", &Status::ToString);
		
	class_<DataIterator>("DataIterator", no_init)
		.def("HasNext", &DataIterator::HasNext)
		.def("Next", &DataIterator::Next)
		.def("Restart", &DataIterator::Restart)
		.def("GetDatum", &DataIterator::GetDatum, return_value_policy<manage_new_object>());
	
	class_<FlexiDatum>("FlexiDatum")
		.def("GetDenseStore", &FlexiDatum::GetDenseStore)
		.def("GetSparseIdx", &FlexiDatum::GetSparseIdx)
		.def("GetSparseVals", &FlexiDatum::GetSparseVals)
		.def("ToString", &FlexiDatum::ToString);
		*/
}
