# Assuming this Makefile lives in project root directory
PROJECT := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)

include $(PROJECT)/config.mk

#BUILD := $(PROJECT)/build
BUILD :=build

SRC_DIR:=src

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
          -lrocksdb 

MLDB_SRC = $(shell find src -type f -name "*.cpp")
MLDB_PROTO = $(shell find src -type f -name "*.proto")
MLDB_HEADERS = $(shell find src -type f -name "*.hpp")

###
PROTO_HDRS = $(patsubst src/%.proto, build/%.pb.h, $(MLDB_PROTO))
PROTO_OBJS = $(patsubst src/%.proto, build/%.pb.o, $(MLDB_PROTO))
MLDB_OBJS = $(patsubst src/%.cpp, build/%.o, $(MLDB_SRC))

PROTOC = $(THIRD_PARTY_BIN)/protoc

$(PROTO_HDRS): $(BUILD)/%.pb.h: $(SRC_DIR)/%.proto
	@mkdir -p $(@D)
	$(PROTOC) --cpp_out=$(BUILD) --python_out=$(BUILD) --proto_path=src $<
	
$(MLDB_LIB): $(PROTO_OBJS) $(MLDB_OBJS)
	@echo MLDB_LIB_
	mkdir -p $(@D)
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	ar csrv $@ $(filter %.o, $?)
	# Make $(BUILD)/ into a python module.
	python $(PROJECT)/python/util/modularize.py $(BUILD)
	
$(PROTO_OBJS): %.pb.o: %.pb.cc
	@echo PROTO_OBJS_
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

$(MLDB_OBJS): $(BUILD)/%.o: $(SRC_DIR)/%.cpp
	@echo MLDB_OBJS_
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

proto:$(PROTO_HDRS)

mldb_lib: path $(MLDB_LIB)

include $(PROJECT)/test/test.mk
