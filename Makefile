# Assuming this Makefile lives in project root directory
PROJECT := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)

include $(PROJECT)/config.mk

BUILD = $(PROJECT)/build
LIB = $(BUILD)/lib
BIN = $(BUILD)/bin

NEED_MKDIR = $(BUILD) \
						 $(BIN) \
             $(LIB)

all: path \
		 mldb_lib

path: $(NEED_MKDIR)

$(NEED_MKDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

.PHONY: all path clean

CXX = g++
CXXFLAGS = -O3 \
           -std=c++11 \
           -Wall \
					 -Wno-sign-compare \
           -fno-builtin-malloc \
           -fno-builtin-calloc \
           -fno-builtin-realloc \
           -fno-builtin-free \
           -fno-omit-frame-pointer

THIRD_PARTY = $(PROJECT)/third_party
THIRD_PARTY_SRC = $(THIRD_PARTY)/src
THIRD_PARTY_INCLUDE = $(THIRD_PARTY)/include
THIRD_PARTY_LIB = $(THIRD_PARTY)/lib
THIRD_PARTY_BIN = $(THIRD_PARTY)/bin

INCFLAGS = -Isrc/ -I$(THIRD_PARTY_INCLUDE) \
					 -Ibuild/ # include generated *pb.h
INCFLAGS += $(HDFS_INCFLAGS)
LDFLAGS = -Wl,-rpath,$(THIRD_PARTY_LIB) \
          -L$(THIRD_PARTY_LIB) \
          -pthread -lrt -lnsl \
          -lzmq \
          -lglog \
          -lgflags \
          -ltcmalloc \
					-lprotobuf
LDFLAGS += $(HDFS_LDFLAGS)

MLDB_SRC = $(shell find src -type f -name "*.cpp")
MLDB_PROTO = $(shell find src -type f -name "*.proto")
MLDB_HEADERS = $(shell find src -type f -name "*.hpp")
MLDB_OBJ = $(patsubst src/%.cpp, build/%.o, $(MLDB_SRC)) \
	$(patsubst src/%.proto, build/%.pb.o, $(MLDB_PROTO))
MLDB_PROTO_HEADERS = $(MLDB_PROTO:.proto=.pb.h)

$(MLDB_LIB): $(MLDB_OBJ) path
	ar csrv $@ $(MLDB_OBJ)

build/%.pb.o: build/%.pb.cc $(MLDB_PROTO_HEADERS)
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(HDFS_INCFLAGS) \
		$(HDFS_LDFLAGS) -c $< -o $@

build/%.o: src/%.cpp $(MLDB_HEADERS) $(MLDB_PROTO_HEADERS)
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(HDFS_INCFLAGS) \
		$(HDFS_LDFLAGS) -c $< -o $@

#$(MLDB_PROTO_OBJ): %.pb.o: %.pb.cc
#	$(CXX) $(CXXFLAGS) $(INCFLAGS) -c $< -o $@

%.pb.cc %.pb.h: %.proto
	$(THIRD_PARTY_BIN)/protoc --cpp_out=$(BUILD) --proto_path=src $<

mldb_lib: $(MLDB_LIB)
