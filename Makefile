################################################################################
# Defs
################################################################################

CXX := clang++

RTTIFLAG := -fno-rtti

DEFS := -DGOOGLE_PROTOBUF_NO_RTTI

INCLUDE_DIRS := \
	/usr/include/z3

INCLUDES := $(foreach dir, $(INCLUDE_DIRS), -I"$(dir)")

CXXFLAGS := \
	-D_GNU_SOURCE \
	-D__STDC_CONSTANT_MACROS \
	-D__STDC_FORMAT_MACROS \
	-D__STDC_LIMIT_MACROS \
	-O0 \
	-fPIC \
	-std=c++11 \
	-g \
	$(RTTIFLAG) \
	$(INCLUDES) \
	$(DEFS)

LLVMCOMPONENTS := analysis archive asmparser asmprinter bitreader bitwriter \
	codegen core cppbackend cppbackendcodegen cppbackendinfo debuginfo engine \
	executionengine instcombine instrumentation interpreter ipa ipo jit linker \
	mc mcdisassembler mcjit mcparser native nativecodegen object runtimedyld \
	scalaropts selectiondag support tablegen target transformutils vectorize \
	linker

LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))

################################################################################
# Compilation tweaking
################################################################################

# warnings to show 
WARNINGS_ON := all extra cast-qual float-equal switch \
	undef init-self pointer-arith cast-align effc++ \
	strict-prototypes strict-overflow=5 write-strings \
	aggregate-return super-class-method-mismatch
# warnings to hide
WARNINGS_OFF := unused-function
# warnings to treat as errors
WARNINGS_TAE := overloaded-virtual return-stack-address \
	implicit-function-declaration address-of-temporary \
	delete-non-virtual-dtor

ifeq ($(CXX), clang++)
CXXFLAGS += \
	$(foreach w,$(WARNINGS_ON), -W$(w)) \
	$(foreach w,$(WARNINGS_OFF),-Wno-$(w)) \
	$(foreach w,$(WARNINGS_TAE),-Werror-$(w))
endif

################################################################################
# Sources
################################################################################

ADDITIONAL_SOURCE_DIRS := \
	$(PWD)/Actions \
	$(PWD)/Anno \
	$(PWD)/Annotation \
	$(PWD)/Codegen \
	$(PWD)/Config \
	$(PWD)/Logging \
	$(PWD)/Passes \
	$(PWD)/Predicate \
	$(PWD)/Protobuf \
	$(PWD)/SMT \
	$(PWD)/State \
	$(PWD)/Term \
	$(PWD)/Type \
	$(PWD)/Util \
	$(PWD)/lib/poolalloc/src

ADDITIONAL_INCLUDE_DIRS := \
	$(PWD) \
	$(PWD)/Protobuf/Gen \
	$(PWD)/lib \
	$(PWD)/lib/pegtl/include \
	$(PWD)/lib/google-test/include \
	$(PWD)/lib/yaml-cpp/include

CXXFLAGS += $(foreach dir,$(ADDITIONAL_INCLUDE_DIRS),-I"$(dir)")

SOURCES := \
	$(shell ls $(PWD)/*.cpp) \
	$(shell find $(ADDITIONAL_SOURCE_DIRS) -name "*.cpp" -type f) \
	$(shell find $(ADDITIONAL_SOURCE_DIRS) -name "*.cc"  -type f)

SOURCES_WITH_MAIN := $(PWD)/wrapper.cpp

SOURCES_WITHOUT_MAIN := $(filter-out $(SOURCES_WITH_MAIN),$(SOURCES))

OBJECTS := $(SOURCES:.cpp=.o)
OBJECTS := $(OBJECTS:.cc=.o)

OBJECTS_WITHOUT_MAIN := $(SOURCES_WITHOUT_MAIN:.cpp=.o)
OBJECTS_WITHOUT_MAIN := $(OBJECTS_WITHOUT_MAIN:.cc=.o)

DEPS := $(SOURCES:.cpp=.d)
DEPS := $(DEPS:.cc=.d)

ARCHIVES :=

################################################################################
# Tests
################################################################################

TEST_DIRS := $(PWD)/test
TEST_SOURCES := $(shell find $(TEST_DIRS) -name "*.cpp" -type f)

TEST_OBJECTS := $(OBJECTS_WITHOUT_MAIN) $(TEST_SOURCES:.cpp=.o)
TEST_DEPS := $(TEST_SOURCES:.cpp=.d)

TEST_OUTPUT := "test_results.xml"

################################################################################
# Google Test
################################################################################

GOOGLE_TEST_DIR := $(PWD)/lib/google-test

GOOGLE_TEST_LIB := $(GOOGLE_TEST_DIR)/make/gtest.a
ARCHIVES += $(GOOGLE_TEST_LIB)

CXXFLAGS += -isystem $(GOOGLE_TEST_DIR)/include

################################################################################
# yaml-cpp
################################################################################

YAML_CPP_DIR := $(PWD)/lib/yaml-cpp

YAML_CPP_LIB := $(YAML_CPP_DIR)/build/libyaml-cpp.a
ARCHIVES += $(YAML_CPP_LIB)

################################################################################
# Exes
################################################################################

EXES := wrapper
TEST_EXES := run-tests

RUN_TEST_EXES := $(PWD)/$(TEST_EXES) \
	--gtest_output="xml:$(TEST_OUTPUT)" \
	--gtest_color=yes

CLANGLIBS := \
    -lclangFrontendTool \
    -lclangFrontend \
    -lclangDriver \
    -lclangSerialization \
    -lclangCodeGen \
    -lclangParse \
    -lclangSema \
    -lclangStaticAnalyzerFrontend \
    -lclangStaticAnalyzerCheckers \
    -lclangStaticAnalyzerCore \
    -lclangAnalysis \
    -lclangARCMigrate \
    -lclangRewrite \
    -lclangEdit \
    -lclangAST \
    -lclangLex \
    -lclangBasic

LIBS := \
	$(CLANGLIBS) \
	$(ARCHIVES) \
	-lz3 \
	-ldl \
	-lrt \
	-lcfgparser \
	-llog4cpp \
	-lprofiler \
	-ljsoncpp \
	-lprotobuf \
	-ltinyxml2

################################################################################
# Test defs management
################################################################################

TEST_DEFS := $(shell find $(TEST_DIRS) -name "tests.def")

$(TEST_DEFS): .FORCE
	@ls -1 $(@D) | grep '^.*.c$$' | xargs -n1 basename > $@.all
	@sed -e 's|^#\s*||' $(@D)/"tests.def.long" > $@.long.all
	@grep -v -x -f $@.long.all $@.all > $@

################################################################################
# Valgrind
################################################################################

VALGRIND := valgrind --leak-check=yes --suppressions=valgrind.supp

################################################################################
# Protobuf
################################################################################

PROTO_SOURCE_DIR := $(PWD)/Protobuf/Gen

PROTOC := protoc --proto_path=$(PWD) --cpp_out=$(PROTO_SOURCE_DIR)

PROTOEXT := $(PWD)/extract-protobuf-desc.awk

################################################################################
# Deps management
################################################################################

%.d: %.cpp .protobuf
	@$(CXX) $(CXXFLAGS) -MM $*.cpp > $*.d
	@mv -f $*.d $*.dd
	@sed -e 's|.*:|$*.o $*.d:|' < $*.dd > $*.d
	@rm -f $*.dd

%.d: %.cc .protobuf
	@$(CXX) $(CXXFLAGS) -MM $*.cc > $*.d
	@mv -f $*.d $*.dd
	@sed -e 's|.*:|$*.o $*.d:|' < $*.dd > $*.d
	@rm -f $*.dd

################################################################################
# Meta rules
################################################################################

.DEFAULT_GOAL := all

.PHONY: .FORCE

.FORCE:

################################################################################
# Rules
################################################################################

all: $(EXES)


.protobuf:
	@mkdir -p $(PROTO_SOURCE_DIR)
	@for H in `find $(PWD) -name "*.cpp" -o -name "*.h" -o -name "*.hpp"`; do \
		$(PROTOEXT) $$H; \
	done
	@find $(PWD) -name "*.proto" | xargs $(PROTOC)
	@touch $@

clean.protobuf:
	@rm -f .protobuf
	@find $(PWD) -name "*.proto" -delete
	@rm -rf $(PROTO_SOURCE_DIR)


.yaml-cpp:
	$(MAKE) CXX=$(CXX) -C $(YAML_CPP_DIR) all
	touch $@

clean.yaml-cpp:
	rm -f .yaml-cpp
	$(MAKE) CXX=$(CXX) -C $(YAML_CPP_DIR) clean


$(EXES): $(OBJECTS) .protobuf
	$(CXX) -g -o $@ -rdynamic $(OBJECTS) $(LIBS) $(LLVMLDFLAGS) $(LIBS)


.google-test:
	$(MAKE) CXX=$(CXX) -C $(GOOGLE_TEST_DIR)/make gtest.a
	touch $@

clean.google-test:
	rm -f .google-test
	$(MAKE) CXX=$(CXX) -C $(GOOGLE_TEST_DIR)/make clean


$(TEST_EXES): $(TEST_OBJECTS) .protobuf .google-test
	$(CXX) -g -o $@ $(TEST_OBJECTS) $(LIBS) $(LLVMLDFLAGS) $(LIBS) $(GOOGLE_TEST_LIB)


tests: $(EXES) $(TEST_EXES)

.regenerate-test-defs: $(TEST_DEFS)


check: tests .regenerate-test-defs
	$(RUN_TEST_EXES) --gtest_filter=-*Long/*

check-with-valgrind: tests .regenerate-test-defs
	$(VALGRIND) $(RUN_TEST_EXES) --gtest_filter=-*Long/*

check-long: tests .regenerate-test-defs
	$(RUN_TEST_EXES) --gtest_filter=*Long/*

check-all: tests .regenerate-test-defs
	$(RUN_TEST_EXES)


clean:
	@rm -f $(EXES) $(OBJECTS) $(DEPS) $(TEST_EXES) $(TEST_OBJECTS) $(TEST_DEPS) $(TEST_OUTPUT)

################################################################################
-include $(DEPS)
-include $(TEST_DEPS)
################################################################################
