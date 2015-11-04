# Assuming this Makefile lives in project root directory
PROJECT := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)

include $(PROJECT)/config.mk

#BUILD := $(PROJECT)/build
BUILD :=build
BUILD_SHARED := build_shared

SRC_DIR:=$(PROJECT)/src

LIB = $(BUILD)/lib
LIB_SHARED = $(BUILD_SHARED)/lib

NEED_MKDIR = $(BUILD) $(LIB) $(BUILD_SHARED) $(LIB_SHARED)

all: proto hotbox_lib hotbox_sharedlib test

path: $(NEED_MKDIR)

$(NEED_MKDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -rf $(BUILD_SHARED)
	rm -rf db_testbed

.PHONY: all path clean

CXX = g++
CXXFLAGS += -O2 \
           -std=c++11 \
           -Wall \
					 -Wno-sign-compare \
           -fno-builtin-malloc \
           -fno-builtin-calloc \
           -fno-builtin-realloc \
           -fno-builtin-free \
           -fno-omit-frame-pointer \
					 -DDMLC_USE_GLOG=1 \

CXXFLAGS_SHARED = $(CXXFLAGS)
CXXFLAGS_SHARED += -fPIC \

THIRD_PARTY = $(PROJECT)/third_party
THIRD_PARTY_SRC = $(THIRD_PARTY)/src
THIRD_PARTY_INCLUDE = $(THIRD_PARTY)/include
THIRD_PARTY_LIB = $(THIRD_PARTY)/lib
THIRD_PARTY_BIN = $(THIRD_PARTY)/bin

#INCFLAGS =  -Isrc/ -I$(THIRD_PARTY_INCLUDE)
INCFLAGS =  -I$(SRC_DIR) -I$(THIRD_PARTY_INCLUDE)
INCFLAGS += -Ibuild/ # include generated *pb.h
INCFLAGS += -I$(JAVA_HOME)/include # include java for HDFS/DMLC access
INCFLAGS += $(HDFS_INCFLAGS)

LDFLAGS = -Wl,-rpath,$(THIRD_PARTY_LIB) \
		  -Wl,-rpath=$(LIBJVM) \
          -L$(THIRD_PARTY_LIB) \
          -L$(LIBJVM) -ljvm \
          -lpthread -lrt -lnsl \
          -lzmq \
          -lglog \
          -lgflags \
          -ltcmalloc \
					-lprotobuf \
					-D_GLIBCXX_USE_NANOSLEEP \
					-lboost_filesystem \
					-lboost_system \
					-lpthread \
					-lyaml-cpp \
					-lsnappy \
          -ldmlc \
          -lhdfs \
          -lrocksdb \

HB_SRC = $(shell find src -type f -name "*.cpp")
HB_PROTO = $(shell find src -type f -name "*.proto")
HB_HEADERS = $(shell find src -type f -name "*.hpp")
HB_LIB_SHARED_OBJS = $(shell find $(BUILD_SHARED) -type f -name "*.o")

###
PROTO_HDRS = $(patsubst src/%.proto, $(BUILD)/%.pb.h, $(HB_PROTO))
PROTO_OBJS = $(patsubst src/%.proto, $(BUILD)/%.pb.o, $(HB_PROTO))
HB_OBJS = $(patsubst src/%.cpp, $(BUILD)/%.o, $(HB_SRC))
PROTO_OBJS_SHARED = $(patsubst src/%.proto, $(BUILD_SHARED)/%.pb.o, $(HB_PROTO))
HB_OBJS_SHARED = $(patsubst src/%.cpp, $(BUILD_SHARED)/%.o, $(HB_SRC))


PROTOC = $(THIRD_PARTY_BIN)/protoc

$(PROTO_HDRS): $(BUILD)/%.pb.h: $(SRC_DIR)/%.proto
	@mkdir -p $(@D)
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	$(PROTOC) --cpp_out=$(BUILD) --python_out=$(BUILD) --proto_path=$(SRC_DIR) $<
	
$(HB_LIB): $(PROTO_OBJS) $(HB_OBJS)
	@echo HB_LIB_
	mkdir -p $(@D)
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	ar csrv $@ $(filter %.o, $?) $(THIRD_PARTY_LIB)/libdmlc.a
	# Make $(BUILD)/ into a python module.
	python $(PROJECT)/python/util/modularize.py $(BUILD)

$(PROTO_OBJS): $(BUILD)/%.pb.o: $(BUILD)/%.pb.cc
	@echo PROTO_OBJS_
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

$(HB_OBJS): $(BUILD)/%.o: $(SRC_DIR)/%.cpp $(PROTO_OBJS)
	@echo HB_OBJS_
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

$(HB_SHARED_LIB): $(HB_OBJS_SHARED) $(PROTO_OBJS_SHARED)
	@echo HB_LIB_SHARED_
	mkdir -p $(@D)
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	$(CXX) -shared -o $@ $(HB_LIB_SHARED_OBJS) $(LDFLAGS)
	
$(PROTO_OBJS_SHARED): $(BUILD_SHARED)/%.pb.o: $(BUILD)/%.pb.cc
	@echo PROTO_OBJS_SHARED
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS_SHARED) $(INCFLAGS) -c $< -o $@

$(HB_OBJS_SHARED): $(BUILD_SHARED)/%.o: $(SRC_DIR)/%.cpp
	@echo HB_OBJS_SHARED
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS_SHARED) $(INCFLAGS) -c $< -o $@

proto:$(PROTO_HDRS)

#$PY_CLIENT_SRC=$(shell find src/client/py_client -type f -name "*.cpp")
#py_hb_client: $(HB_LIB)
#	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LDFLAGS) -shared -Wl,--export-dynamic $(PY_CLIENT_SRC) -L./build/lib -lhotbox -lboost_python -L/usr/lib/python2.7/config -lpython2.7 -o py_hb_client.so
spark_exp_server: experiment/spark_exp/server/server.cpp $(HB_LIB)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) experiment/spark_exp/server/server.cpp $(HB_LIB) $(LDFLAGS) -o spark_exp_server

hotbox_lib: path proto $(HB_LIB)

hotbox_sharedlib: path proto $(HB_SHARED_LIB)

include $(PROJECT)/test/test.mk
