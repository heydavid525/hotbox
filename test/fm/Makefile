FM_DIR := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)
HOTBOX_ROOT = $(FM_DIR)/../../

include $(HOTBOX_ROOT)/config.mk

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

INCFLAGS =  -I$(THIRD_PARTY_INCLUDE)
INCFLAGS += -I$(PYTHON_INCLUDE)
INCFLAGS += -I$(THIRD_PARTY_HB)/include
INCFLAGS += -I$(HOTBOX_ROOT)/src
INCFLAGS += -I$(HOTBOX_ROOT)/build/
INCFLAGS += -I$(HOTBOX_ROOT)

FM_SRC = $(wildcard $(FM_DIR)/src/*.cpp)
FM_HDR = $(wildcard $(FM_DIR)/src/*.hpp)
FM_BIN = $(FM_DIR)/bin
FM_OBJ = $(FM_SRC:.cpp=.o)
NDEBUG = -DNDEBUG


all:  $(FM_BIN)/petuum_fm 


LDFLAGS = -Wl,-rpath,$(HOTBOX_ROOT)/build/lib \
                        -L$(HOTBOX_ROOT)/build/lib \
                           -lhotbox


#LDFLAGS += -Wl,-rpath,$(THIRD_PARTY_LIB) \
          -L$(THIRD_PARTY_LIB) \
                                        -Wl,-rpath,$(BUILD)/lib \
          -L$(BUILD)/lib \
	  -lgflags \
	-lglog

LDFLAGS += -Wl,-rpath,$(THIRD_PARTY_LIB) \
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
                                        -lpthread \
                                        -lyaml-cpp \
                                        -lsnappy \
                -ldmlc \
                -lrocksdb \
          -lglog

LDFLAGS += -L$(PYTHON_LIB) \
           -lboost_python \
           -lpython2.7




$(FM_BIN)/petuum_fm: $(FM_OBJ) 
	$(CXX) $(CXXFLAGS) $(INCFLAGS) -o $@ \
	$(FM_OBJ) -lgtest $(LDFLAGS) -lpetuum_iterparallel 

$(FM_OBJ): %.o: %.cpp $(FM_HDR)
	$(CXX) $(NDEBUG) $(CXXFLAGS) -Wno-unused-result $(INCFLAGS) -c $< -o $@
clean:
	rm -rf $(FM_OBJ)
	rm -rf $(FM_BIN)/*

.PHONY: all  clean             
