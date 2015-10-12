# Requires PROJECT to be defined as repo root dir

ifndef JAVA_HOME
  JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
endif
ifndef LIBJVM
  LIBJVM=$(JAVA_HOME)/jre/lib/amd64/server
endif

HB_LIB = $(PROJECT)/build/lib/libhotbox.a
