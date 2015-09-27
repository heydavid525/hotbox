

TEST_DIR = $(BUILD)/test
TEST_SRC = $(shell find test -type f -name "*.cpp")
TEST_BIN = $(patsubst test/%.cpp, $(TEST_DIR)/%, $(TEST_SRC))
TEST_INCFLAGS = -I$(PROJECT)

#$(TEST_DIR)/facility/test_facility.o: test/facility/test_facility.cpp
#	mkdir -p $(TEST_DIR)/facility/
#	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
#		 $(LDFLAGS) -c $< -o $@

$(TEST_DIR)/%: test/%.cpp $(MLDB_LIB) test/facility/test_facility.hpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(TEST_INCFLAGS) \
		$< -lgtest $(MLDB_LIB) $(LDFLAGS) test/facility/test_facility.hpp -o $@

test: $(TEST_BIN) class_registry_test stream_test #schema_util_test one_hot_transform_test

#schema_util_test: $(TEST_BIN)
#	$(TEST_DIR)/transform/schema_util_test

#one_hot_transform_test: $(TEST_BIN)
#	$(TEST_DIR)/transform/one_hot_transform_test

class_registry_test: $(TEST_BIN)
	$(TEST_DIR)/util/class_registry_test

stream_test: $(TEST_BIN)
	$(TEST_DIR)/io/stream_test
