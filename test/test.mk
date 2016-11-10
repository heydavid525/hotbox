
#ROCKS_TEST_SRC = $(shell find test -type f -name "*.cc")
#ROCKS_CPP = $(patsubst test/%.cc, test/%.cpp, $(ROCKS_TEST_SRC))
TEST_DIR = $(PROJECT)/$(BUILD)/test
TEST_SRC = $(shell find $(PROJECT)/test -type f -name "*.cpp")
TEST_BIN = $(patsubst $(PROJECT)/test/%.cpp, $(TEST_DIR)/%, $(TEST_SRC))
TEST_PROTO += $(shell find test -type f -name "*.proto")
TEST_PROTO_HDRS = $(patsubst test/%.proto, $(TEST_DIR)/%.pb.h, $(TEST_PROTO))
TEST_PROTO_OBJS = $(patsubst test/%.proto, $(TEST_DIR)/%.pb.o, $(TEST_PROTO))
TEST_INCFLAGS = -I$(PROJECT) -Ibuild/test

TEST_LDFLAGS = -Wl,-rpath,$(PROJECT)/$(LIB) \
          		-L$(PROJECT)/$(LIB) \
			   -lhotbox

$(TEST_PROTO_HDRS): $(TEST_DIR)/%.pb.h: $(PROJECT)/test/%.proto
	@mkdir -p $(@D)
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	$(PROTOC) --cpp_out=$(TEST_DIR) --python_out=$(TEST_DIR) \
	--proto_path=$(PROJECT)/test $<

test_proto: $(TEST_PROTO_HDRS)

$(TEST_PROTO_OBJS): $(TEST_DIR)/%.pb.o: $(TEST_DIR)/%.pb.cc
	@echo TEST_PROTO_OBJS_
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) -c $< -o $@

#$(TEST_DIR)/facility/test_facility.o: test/facility/test_facility.cpp
#	mkdir -p $(TEST_DIR)/facility/
#	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
#		 $(LDFLAGS) -c $< -o $@

$(TEST_DIR)/%: $(PROJECT)/test/%.cpp $(HB_LIB_LINK) \
	test/facility/test_facility.hpp $(TEST_PROTO_OBJS)
	mkdir -p $(@D)
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< $(TEST_PROTO_OBJS) -o $@ -lgtest $(TEST_LDFLAGS) $(LDFLAGS)

db_server_main: $(PROJECT)/test/db/db_server_main.cpp $(HB_LIB_LINK) \
	test/facility/test_facility.hpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< -o $(TEST_DIR)/db/db_server_main \
		-lgtest $(TEST_LDFLAGS) $(LDFLAGS)

proxy_server_main: $(PROJECT)/test/client/proxy_server_main.cpp $(HB_LIB_LINK) \
	test/facility/test_facility.hpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< -o $(TEST_DIR)/client/proxy_server_main \
		-lgtest $(TEST_LDFLAGS) $(LDFLAGS)

hotbox_client_main: $(PROJECT)/test/client/hotbox_client_main.cpp \
	$(HB_LIB_LINK) test/facility/test_facility.hpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< -o $(TEST_DIR)/client/hotbox_client_main\
		-lgtest  $(TEST_LDFLAGS) $(LDFLAGS)

stat_client: $(PROJECT)/test/client/stat_client.cpp \
	$(HB_LIB_LINK) test/facility/test_facility.hpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< -o $(TEST_DIR)/client/stat_client\
		-lgtest  $(TEST_LDFLAGS) $(LDFLAGS)

test: test_proto $(TEST_BIN) class_registry_test stream_test db_server_main \
	 compressed_streams_test util_test

# RocksDB test generates a lot of files.
expensive_test: rocks_db_test rocks_if_test

class_registry_test: $(TEST_BIN)
	$(TEST_DIR)/util/class_registry_test

stream_test: $(TEST_BIN)
	$(TEST_DIR)/io/stream_test

rocks_if_test: $(TEST_BIN)
	$(TEST_DIR)/util/rocksdb_if_test

rocks_db_test: $(TEST_BIN)
	$(TEST_DIR)/util/rocks_db_test

compressed_streams_test: $(TEST_BIN)
	$(TEST_DIR)/io/compressed_streams_test

util_test: $(TEST_BIN)
	$(TEST_DIR)/util/util_test
