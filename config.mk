# Build Test?
BUILD_TEST = 1
# Build Shared Lib? 0 for static only, 1 for shared only
USE_SHARED_LIB = 1
# Configure whether to use HDFS.
USE_HDFS = 0


# Configure where install_third_party.py script builds to. $(PROJECT) points
# to hotbox repo path. The path should start with /
THIRD_PARTY = /home/ubuntu/lib/hotbox_third_party
THIRD_PARTY_HB = /home/ubuntu/lib/hotbox

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

# Path containing pyconfig.h
PYTHON_INCLUDE=/usr/include/python2.7

# Path containing libpython2.7.a
PYTHON_LIB=/usr/lib/python2.7

ifeq ($(USE_HDFS), 1)
  HDFS_LDFLAGS = -Wl,-rpath=$(LIBJVM) \
	          	   -L$(LIBJVM) -ljvm \
		-Wl,-rpath=$(HADOOP_HOME)/lib/native \
			  -L$(HADOOP_HOME)/lib/native -lhdfs
  # -DUSE_HDFS is used by rocksdb_hdfs 
  HDFS_INCFLAGS = -I${HADOOP_HOME}/include -DUSE_HDFS
else
  HDFS_LDFLAGS =
  HDFS_INCFLAGS =
endif


