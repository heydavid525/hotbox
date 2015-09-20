# Requires PROJECT to be defined as repo root dir

# Modify system dependent parameters for each environment:
JAVA_HOME = /usr/lib/jvm/java-7-openjdk-amd64
HADOOP_HOME = /usr/local/hadoop/hadoop-2.6.0

ifndef LIBJVM
  LIBJVM=$(JAVA_HOME)/jre/lib/amd64/server
endif

# Using default hadoop_home
ifndef HADOOP_HDFS_HOME
  HADOOP_HDFS_HOME=$(HADOOP_HOME)
endif

# whether use HDFS support during compile
USE_HDFS = 0

ifeq ($(USE_HDFS),1)
  ifndef HDFS_INC_PATH
    HDFS_INC_PATH=$(HADOOP_HDFS_HOME)/include
  endif
  ifndef HDFS_LIB_PATH
    HDFS_LIB_PATH=$(HADOOP_HDFS_HOME)/lib/native
  endif

  DMLC_CFLAGS+= -DDMLC_USE_HDFS=1 -I$(HDFS_INC_PATH) -I$(JAVA_HOME)/include

  ifneq ("$(wildcard $(HDFS_LIB_PATH)/libhdfs.so)","")
    DMLC_LDFLAGS+= -L$(HDFS_LIB_PATH) -lhdfs
  else
    DMLC_LDFLAGS+= $(HDFS_LIB_PATH)/libhdfs.a
  endif
  DMLC_LDFLAGS += -L$(LIBJVM) -ljvm -Wl,-rpath=$(LIBJVM)
else
  DMLC_CFLAGS+= -DDMLC_USE_HDFS=0
endif

#HAS_HDFS = # Leave empty to build without hadoop.
#HAS_HDFS = -DHAS_HADOOP # Uncomment this line to enable hadoop
#ifdef HAS_HDFS
#  #$(info Hadoop is enabled.)
#  # Given $HADOOP_HOME, HDFS_LDFLAGS and HDFS_INCFLAGS usually doesn't
#  # need changing, unless your hadoop is not installed in a standard way.
#  HDFS_LDFLAGS=-Wl,-rpath,${HADOOP_HOME}/lib/native/ \
#               -Wl,-rpath,${HADOOP_HOME}/lib/ \
#               -Wl,-rpath,${JAVA_HOME}/jre/lib/amd64/server/ \
#               -L${HADOOP_HOME}/lib/native/ \
#               -L${JAVA_HOME}/jre/lib/amd64/server/ \
#               -lhdfs -ljvm
#  HDFS_INCFLAGS = -I${HADOOP_HOME}/include
#else
#  #$(info Hadoop is disabled)
#  HDFS_LDFLAGS =
#  HDFS_INCFLAGS =
#endif

MLDB_LIB = $(PROJECT)/build/lib/libmldb.a
