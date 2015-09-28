# Assuming this Makefile lives in project root directory
PROJECT := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)

include $(PROJECT)/config.mk

BUILD = $(PROJECT)/build
LIB = $(BUILD)/lib

NEED_MKDIR = $(BUILD) $(LIB)

all: mldb_lib test

path: $(NEED_MKDIR)

$(NEED_MKDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

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
					 -DDMLC_USE_GLOG=1

THIRD_PARTY = $(PROJECT)/third_party
THIRD_PARTY_SRC = $(THIRD_PARTY)/src
THIRD_PARTY_INCLUDE = $(THIRD_PARTY)/include
THIRD_PARTY_LIB = $(THIRD_PARTY)/lib
THIRD_PARTY_BIN = $(THIRD_PARTY)/bin

INCFLAGS =  -Isrc/ -I$(THIRD_PARTY_INCLUDE)
INCFLAGS += -Ibuild/ # include generated *pb.h
INCFLAGS += -I$(JAVA_HOME)/include # include java for HDFS/DMLC access

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
          -lhdfs

MLDB_SRC = $(shell find src -type f -name "*.cpp")
MLDB_PROTO = $(shell find src -type f -name "*.proto")
MLDB_HEADERS = $(shell find src -type f -name "*.hpp")
MLDB_OBJ = $(patsubst src/%.cpp, build/%.o, $(MLDB_SRC)) \
	$(patsubst src/%.proto, build/%.pb.o, $(MLDB_PROTO))
MLDB_PROTO_HEADERS = $(MLDB_PROTO:.proto=.pb.h)

$(MLDB_LIB): $(MLDB_OBJ) path
	ar csrv $@ $(filter %.o, $?)
	# Make $(BUILD)/ into a python module.
	python $(PROJECT)/python/util/modularize.py $(BUILD)

build/%.pb.o: build/%.pb.cc $(MLDB_PROTO_HEADERS)
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(HDFS_INCFLAGS) \
		$(HDFS_LDFLAGS) -c $< -o $@

build/%.o: src/%.cpp $(MLDB_HEADERS) $(MLDB_PROTO_HEADERS)
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LDFLAGS) $(HDFS_INCFLAGS) \
		$(HDFS_LDFLAGS) -c $< -o $@

%.pb.cc %.pb.h: %.proto path
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	$(THIRD_PARTY_BIN)/protoc --cpp_out=$(BUILD) --python_out=$(BUILD) \
		--proto_path=src $<

mldb_lib: path $(MLDB_LIB)

include $(PROJECT)/test/test.mk
