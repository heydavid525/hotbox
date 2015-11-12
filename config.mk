# Requires PROJECT to be defined as repo root dir

ifndef JAVA_HOME
  JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
endif
ifndef LIBJVM
  LIBJVM=$(JAVA_HOME)/jre/lib/amd64/server
endif

HB_LIB = $(PROJECT)/build/lib/libhotbox.a
HB_SHARED_LIB = $(PROJECT)/build_shared/lib/libhotbox.so

HB_LIB_LINK = $(HB_LIB)

# Configure where install_third_party.py script builds to. $(PROJECT) points
# to hotbox repo path. The path should start with /
THIRD_PARTY = $(PROJECT)/third_party
