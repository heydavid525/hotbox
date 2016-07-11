project(py_hb_wrapper)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(/usr/include/python2.7 /home/ubuntu/github/hotbox/src /home/ubuntu/github/hotbox/third_party/include /home/ubuntu/github/hotbox/build)
link_directories(/usr/lib /home/ubuntu/github/hotbox/build/lib /home/ubuntu/github/hotbox/third_party/lib)

file(GLOB PY_WRAPPER_SRC
  "*.hpp"
  "*.cpp"
)
add_library(py_hb_wrapper SHARED ${PY_WRAPPER_SRC} )
target_link_libraries(py_hb_wrapper boost_python python2.7 hotbox protobuf glog gflags tcmalloc dmlc rocksdb boost_system boost_filesystem yaml-cpp snappy zmq)
