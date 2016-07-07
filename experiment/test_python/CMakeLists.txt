project(test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(/home/wdai/lib/anconda2/include/python2.7 /home/wdai/lib/third_party/include)
link_directories(/home/wdai/lib/anconda2/lib/python2.7/config
  /home/wdai/lib/third_party/lib)

add_executable (test test.cpp) 
target_link_libraries(test boost_python python2.7)
