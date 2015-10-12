
ROCKS_TEST_SRC = $(shell find test -type f -name "*.cc")
ROCKS_CPP = $(patsubst test/%.cc, test/%.cpp, $(ROCKS_TEST_SRC))
TEST_DIR = $(BUILD)/test
TEST_SRC = $(shell find test -type f -name "*.cpp")
TEST_BIN = $(patsubst test/%.cpp, $(TEST_DIR)/%, $(TEST_SRC))
TEST_INCFLAGS = -I$(PROJECT)

#$(TEST_DIR)/facility/test_facility.o: test/facility/test_facility.cpp
#	mkdir -p $(TEST_DIR)/facility/
#	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
#		 $(LDFLAGS) -c $< -o $@

$(TEST_DIR)/%: test/%.cpp $(HB_LIB) test/facility/test_facility.hpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< test/facility/test_facility.hpp -lgtest $(HB_LIB) $(LDFLAGS) -o $@

db_server_main: test/db/db_server_main.cpp $(HB_LIB) \
	test/facility/test_facility.hpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< test/facility/test_facility.hpp -lgtest $(HB_LIB) $(LDFLAGS) -o \
		$(TEST_DIR)/db/db_server_main

hotbox_client_main: test/client/hotbox_client_main.cpp $(HB_LIB) \
	test/facility/test_facility.hpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< test/facility/test_facility.hpp -lgtest $(HB_LIB) $(LDFLAGS) -o \
		$(TEST_DIR)/client/hotbox_client_main

test: $(TEST_BIN) class_registry_test stream_test db_server_main

class_registry_test: $(TEST_BIN)
	$(TEST_DIR)/util/class_registry_test

stream_test: $(TEST_BIN)
	$(TEST_DIR)/io/stream_test
