project(fm)

file(GLOB_RECURSE FM_SOURCE_FILES src/*.[ch]pp)

find_package(Hotbox REQUIRED)
find_package(Protobuf 3.0.0 REQUIRED)
find_package(Yaml REQUIRED)
find_package(RocksDB REQUIRED)
#find_package(Jvm REQUIRED)
#find_package(Hdfs REQUIRED)
find_package(Dmlc REQUIRED)
#link_directories("/usr/lib64")

#set(CMAKE_INSTALL_RPATH, ${CMAKE_PREFIX_PATH}
#  ${CMAKE_INSTALL_RPATH}
#  ${JVM_ROOT_DIR} ${HDFS_ROOT_DIR})

add_executable(petuum_fm ${FM_SOURCE_FILES})

target_include_directories(
    petuum_fm PUBLIC ${iterparallel_INCLUDE_DIRS})
target_include_directories(
  petuum_fm PUBLIC ${Hotbox_INCLUDE_DIRS})

  #set (CMAKE_SHARED_LINKER_FLAGS
#  "-Wl,-rpath=$HADOOP_HOME -L$HADOOP_HOME -Wl,-rpath=/home/wdai/lib/third_party/lib")

target_link_libraries(petuum_fm petuum_iterparallel)
target_link_libraries(petuum_fm ${Hotbox_LIBRARIES})
target_link_libraries(petuum_fm ${PROTOBUF_LIBRARIES})
target_link_libraries(petuum_fm ${YAML_LIBRARIES})
target_link_libraries(petuum_fm ${ROCKSDB_LIBRARIES})
target_link_libraries(petuum_fm ${JVM_LIBRARIES})
target_link_libraries(petuum_fm ${HDFS_LIBRARIES})
target_link_libraries(petuum_fm ${DMLC_LIBRARIES})
target_link_libraries(petuum_fm boost_python python2.7)
