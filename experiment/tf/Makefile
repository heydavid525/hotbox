
APP_DIR := $(shell readlink $(dir $(lastword $(MAKEFILE_LIST))) -f)
HOTBOX_ROOT = $(APP_DIR)/../..

include $(HOTBOX_ROOT)/config.mk

NEED_MKDIR = lib

APP_SRC = $(wildcard $(APP_DIR)/src/*.cpp)
APP_HDR = $(wildcard $(APP_DIR)/src/*.hpp)
APP_BIN = $(APP_DIR)/bin

TF_INC = /users/wdai/local/tf_include/tensorflow/include
TF_LIB = /users/wdai/local/tensorflow/bazel-bin/tensorflow
TF_INCFLAGS = -I$(TF_INC) -Iinclude
TF_LDFLAGS = -Wl,-rpath,$(TF_LIB) \
          -L$(TF_LIB) \
          -ltensorflow

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

all: path lib/libtf_session.a

$(APP_BIN):
	mkdir -p $(APP_BIN)

path: $(NEED_MKDIR)

$(NEED_MKDIR):
	mkdir -p $@

bin/tf_session.o: src/tf_session.cpp
	$(CXX) $(CXXFLAGS) $(TF_INCFLAGS) -c $< -o $@

lib/libtf_session.so: $(APP_BIN) bin/tf_session.o
	$(CXX) -o $@ -shared bin/tf_session.o

lib/libtf_session.a: $(APP_BIN) bin/tf_session.o
	ar rvs $@ bin/tf_session.o

clean:
	rm -rf $(APP_BIN) lib

.PHONY: clean path
