# Build Test?
BUILD_TEST = 1
# Build Shared Lib? 0 for static only, 1 for shared only
# 2 for both.
USE_SHARED_LIB = 0
# Configure whether to use HDFS.
USE_HDFS = 0


# Configure where install_third_party.py script builds to. $(PROJECT) points
# to hotbox repo path. The path should start with /
THIRD_PARTY = $(PROJECT)/third_party

# Hotbox LIB.
HB_LIB = $(PROJECT)/build/lib/libhotbox.a
HB_SHARED_LIB = $(PROJECT)/build/lib/libhotbox.so

# Requires PROJECT to be defined as repo root dir
ifndef JAVA_HOME
  JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
endif
ifndef LIBJVM
  LIBJVM=$(JAVA_HOME)/jre/lib/amd64/server
endif
ifndef HADOOP_HOME
	HADOOP_HOME = /usr/local/hadoop/hadoop-2.6.0
endif

ifeq ($(USE_HDFS), 1)
  HDFS_LDFLAGS = -Wl,-rpath=$(LIBJVM) \
	          	   -L$(LIBJVM) -ljvm \
	          	   -lhdfs 
  HDFS_INCFLAGS = -I${HADOOP_HOME}/include
else
  HDFS_LDFLAGS =
  HDFS_INCFLAGS =
endif


