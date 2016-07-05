project(test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(/usr/include/python2.7 /usr/include)

add_executable (test test.cpp) 
target_link_libraries(test boost_python python2.7)