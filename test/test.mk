
#ROCKS_TEST_SRC = $(shell find test -type f -name "*.cc")
#ROCKS_CPP = $(patsubst test/%.cc, test/%.cpp, $(ROCKS_TEST_SRC))
TEST_DIR = $(PROJECT)/$(BUILD)/test
TEST_SRC = $(shell find $(PROJECT)/test -type f -name "*.cpp")
TEST_BIN = $(patsubst $(PROJECT)/test/%.cpp, $(TEST_DIR)/%, $(TEST_SRC))
TEST_INCFLAGS = -I$(PROJECT)

TEST_LDFLAGS = -Wl,-rpath,$(PROJECT)/$(LIB) \
          		-L$(PROJECT)/$(LIB) \
			   -lhotbox

#$(TEST_DIR)/facility/test_facility.o: test/facility/test_facility.cpp
#	mkdir -p $(TEST_DIR)/facility/
#	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
#		 $(LDFLAGS) -c $< -o $@

$(TEST_DIR)/%: $(PROJECT)/test/%.cpp $(HB_LIB_LINK) test/facility/test_facility.hpp
	mkdir -p $(@D)
	LD_LIBRARY_PATH=$(THIRD_PARTY_LIB) \
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< -o $@ -lgtest $(TEST_LDFLAGS) $(LDFLAGS) 

db_server_main: $(PROJECT)/test/db/db_server_main.cpp $(HB_LIB_LINK) \
	test/facility/test_facility.hpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< -o $(TEST_DIR)/db/db_server_main \
		-lgtest $(TEST_LDFLAGS) $(LDFLAGS) 
		

hotbox_client_main: $(PROJECT)/test/client/hotbox_client_main.cpp $(HB_LIB_LINK) \
	test/facility/test_facility.hpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< -o $(TEST_DIR)/client/hotbox_client_main\
		-lgtest  $(TEST_LDFLAGS) $(LDFLAGS) 

test: $(TEST_BIN) class_registry_test stream_test db_server_main rocks_if_test

class_registry_test: $(TEST_BIN)
	$(TEST_DIR)/util/class_registry_test

stream_test: $(TEST_BIN)
	$(TEST_DIR)/io/stream_test

rocks_if_test: $(TEST_BIN)
	$(TEST_DIR)/db/rocksdb/rocksdb_if_test
