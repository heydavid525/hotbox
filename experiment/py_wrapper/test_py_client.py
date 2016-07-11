import sys
sys.path.append("build/")
import py_hb_wrapper

client = py_hb_wrapper.PYClient()
options = dict()
options["db_name"] = "a1a"
options["session_id"] = "a1a_session"
options["transform_config_path"] = "/home/ubuntu/github/hotbox/test/resource/select_all.conf"
options["type"] = "sparse"
session = client.CreateSession(options)
print(session.GetNumData())
print(session.GetData(0, -1))
