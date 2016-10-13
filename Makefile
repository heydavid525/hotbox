# Assuming this Makefile lives in project root directory
PROJECT := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)

include $(PROJECT)/config.mk

SRC_DIR := $(PROJECT)/src
BUILD := build
LIB = $(BUILD)/lib

NEED_MKDIR = $(BUILD) $(LIB)

ifeq ($(USE_SHARED_LIB), 0)
all: proto hotbox_lib test
HB_LIB_LINK = $(HB_LIB)
else
all: proto hotbox_sharedlib test
HB_LIB_LINK = $(HB_SHARED_LIB)
endif

path: $(NEED_MKDIR)

$(NEED_MKDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -rf db_testbed

.PHONY: all path clean

CXX = g++
CXXFLAGS += -O2 \
           -std=c++11 \
           -Wall \
					 -fPIC \
					 -Wno-sign-compare \
           -fno-builtin-malloc \
           -fno-builtin-calloc \
           -fno-builtin-realloc \
           -fno-builtin-free \
           -fno-omit-frame-pointer \
					 -DDMLC_USE_GLOG
					 #-DUSE_ROCKS

THIRD_PARTY_SRC = $(THIRD_PARTY)/src
THIRD_PARTY_INCLUDE = $(THIRD_PARTY)/include
THIRD_PARTY_LIB = $(THIRD_PARTY)/lib
THIRD_PARTY_BIN = $(THIRD_PARTY)/bin

INCFLAGS =  -I$(SRC_DIR) -I$(THIRD_PARTY_INCLUDE)
ifeq ($(USE_TF), 1)
INCFLAGS += -I$(TF_INC) 
endif
INCFLAGS += -Ibuild/ # include generated *pb.h
INCFLAGS += -I$(JAVA_HOME)/include # include java for HDFS/DMLC access
INCFLAGS += $(HDFS_INCFLAGS)
INCFLAGS += -I$(PYTHON_INCLUDE)

LDFLAGS = -Wl,-rpath,$(THIRD_PARTY_LIB) \
          -L$(THIRD_PARTY_LIB) \
					-Wl,-rpath,$(BUILD)/lib \
          -L$(BUILD)/lib \
          -lpthread -lrt -lnsl \
          -lzmq \
          -lgflags \
					-lprotobuf \
          -ltcmalloc \
					-lprofiler \
					-D_GLIBCXX_USE_NANOSLEEP \
					-lboost_filesystem \
					-lboost_system \
					-lyaml-cpp \
					-lsnappy \
	        -ldmlc \
					-lpthread \
	        -lrocksdb \
          -lglog
					# don't use tcmalloc in building shared library.
          #-ltcmalloc \
					-lprofiler \
          # lglog must come after ldmlc, which depends on glog.
          #-lrocksdb
LDFLAGS += $(HDFS_LDFLAGS)
LDFLAGS += -L$(PYTHON_LIB) \
           -lboost_python \
           -lpython2.7
ifeq ($(USE_TF), 1)
LDFLAGS += -Wl,-rpath,$(TF_LIB) \
          -L$(TF_LIB) \
          -ltensorflow \
          -DUSE_TF
endif
HB_SRC = $(shell find src -type f -name "*.cpp")
HB_PROTO = $(shell find src -type f -name "*.proto")
HB_HEADERS = $(shell find src -type f -name "*.hpp")

PROTO_HDRS = $(patsubst src/%.proto, $(BUILD)/%.pb.h, $(HB_PROTO))
PROTO_OBJS = $(patsubst src/%.proto, $(BUILD)/%.pb.o, $(HB_PROTO))
HB_OBJS = $(patsubst src/%.cpp, $(BUILD)/%.o, $(HB_SRC))
HB_LIB_OBJS = $(shell find $(BUILD) -type f -name "*.o")


PROTOC = $(THIRD_PARTY_BIN)/protoc

$(PROTO_HDRS): $(BUILD)/%.pb.h: $(SRC_DIR)/%.proto
	@mkdir -p $(@D)
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	$(PROTOC) --cpp_out=$(BUILD) --python_out=$(BUILD) \
	--proto_path=$(SRC_DIR) $<

$(PROTO_OBJS): $(BUILD)/%.pb.o: $(BUILD)/%.pb.cc
	@echo PROTO_OBJS_
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

$(HB_OBJS): $(BUILD)/%.o: $(SRC_DIR)/%.cpp $(PROTO_OBJS)
	@echo HB_OBJS_
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

$(HB_LIB): $(PROTO_OBJS) $(HB_OBJS) 
	@echo HB_LIB_
	mkdir -p $(@D)
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	ar csrv $@ $(filter %.o, $?) $(THIRD_PARTY_LIB)/libdmlc.a
	# Make $(BUILD)/ into a python module.
	python $(PROJECT)/python/util/modularize.py $(BUILD)

$(HB_SHARED_LIB): $(HB_OBJS) $(PROTO_OBJS) 
	@echo HB_LIB_SHARED_
	mkdir -p $(@D)
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	$(CXX) -shared -o $@ $(HB_LIB_OBJS) #$(LDFLAGS)
	# Make $(BUILD)/ into a python module.
	python $(PROJECT)/python/util/modularize.py $(BUILD)

proto: $(PROTO_HDRS)

#$PY_CLIENT_SRC=$(shell find src/client/py_client -type f -name "*.cpp")
#py_hb_client: $(HB_LIB)
#	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LDFLAGS) -shared -Wl,--export-dynamic $(PY_CLIENT_SRC) -L./build/lib -lhotbox -lboost_python -L/usr/lib/python2.7/config -lpython2.7 -o py_hb_client.so
spark_exp_server: experiment/spark_exp/server/server.cpp $(HB_LIB)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) experiment/spark_exp/server/server.cpp $(HB_LIB) $(LDFLAGS) -lboost_thread -o spark_exp_server
spark_streaming_server: experiment/spark_exp/server/stream_server.cpp $(HB_LIB)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) experiment/spark_exp/server/stream_server.cpp $(HB_LIB) $(LDFLAGS) -o spark_streaming_server

hotbox_lib: path proto $(HB_LIB) 

hotbox_sharedlib: path proto $(HB_SHARED_LIB) 

ifeq ($(BUILD_TEST), 1)
include $(PROJECT)/test/test.mk
endif
