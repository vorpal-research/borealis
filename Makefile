################################################################################
# Defs
################################################################################

CXX := clang++

RTTIFLAG := -fno-rtti

DEFS := \
	-DGOOGLE_PROTOBUF_NO_RTTI \
#	-DUSE_MATHSAT_SOLVER

USER_DEFS := # -DNO_TRACING

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
	-std=c++14 \
	-g \
	$(RTTIFLAG) \
	$(INCLUDES) \
	$(DEFS) \
	$(USER_DEFS)

#LLVMCOMPONENTS := analysis asmparser asmprinter bitreader bitwriter \
#	codegen core cppbackend cppbackendcodegen cppbackendinfo debuginfo engine \
#	executionengine instcombine instrumentation interpreter ipa ipo jit linker \
#	mc mcdisassembler mcjit mcparser native nativecodegen object runtimedyld \
#	scalaropts selectiondag support tablegen target transformutils vectorize \
#	linker
#

LLVMCOMPONENTS := all
LLVMCONFIG := $(if $(LLVMDIR),$(LLVMDIR)/bin/llvm-config,llvm-config)

LLVMLDFLAGS := $(shell $(LLVMCONFIG) --ldflags)
LLVMLIBS := $(shell $(LLVMCONFIG) --libs $(LLVMCOMPONENTS))
LLVMSYSTEMLIBS := $(shell $(LLVMCONFIG) --system-libs)

################################################################################
# Compilation tweaking
################################################################################

# warnings to show
WARNINGS_ON := all extra cast-qual float-equal switch \
	undef init-self pointer-arith cast-align effc++ \
	strict-prototypes strict-overflow=5 write-strings \
	aggregate-return super-class-method-mismatch
# warnings to hide
WARNINGS_OFF := unused-function redundant-decls
# warnings to treat as errors
WARNINGS_TAE := overloaded-virtual return-stack-address \
	implicit-function-declaration address-of-temporary \
	delete-non-virtual-dtor return-type

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
	$(PWD)/Driver \
	$(PWD)/Executor \
	$(PWD)/Factory \
	$(PWD)/Logging \
	$(PWD)/Passes \
	$(PWD)/Predicate \
	$(PWD)/Protobuf \
	$(PWD)/Reanimator \
	$(PWD)/SMT \
	$(PWD)/State \
	$(PWD)/Statistics \
	$(PWD)/Term \
	$(PWD)/Type \
	$(PWD)/Util \

#$(PWD)/lib/poolalloc/src

ADDITIONAL_INCLUDE_DIRS := \
	$(PWD) \
	$(PWD)/Protobuf/Gen \
	$(PWD)/lib \
	$(PWD)/lib/pegtl/include \
	$(PWD)/lib/google-test/include \
	$(PWD)/lib/yaml-cpp/include \
	$(PWD)/lib/cfgparser/include

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

TEST_ARCHIVES :=

TEST_OUTPUT := test_results.xml

################################################################################
# Google Test
################################################################################

GOOGLE_TEST_DIR := $(PWD)/lib/google-test

GOOGLE_TEST_LIB := $(GOOGLE_TEST_DIR)/make/gtest.a
TEST_ARCHIVES += $(GOOGLE_TEST_LIB)

CXXFLAGS += -isystem $(GOOGLE_TEST_DIR)/include

GOOGLE_TEST_FILTER := *

################################################################################
# yaml-cpp
################################################################################

YAML_CPP_DIR := $(PWD)/lib/yaml-cpp

YAML_CPP_LIB := $(YAML_CPP_DIR)/build/libyaml-cpp.a
ARCHIVES += $(YAML_CPP_LIB)

################################################################################
# cfgparser
################################################################################

# cfgparser needs dbglog, but we don't
DBGLOG_DIR := $(PWD)/lib/dbglog
DBGLOG_DIST := $(DBGLOG_DIR)/dist
DBGLOG_LIB := $(DBGLOG_DIST)/lib/libdbglog.a

CFGPARSER_DIR := $(PWD)/lib/cfgparser
CFGPARSER_LIB := $(CFGPARSER_DIR)/dist/lib/libcfgparser.a
ARCHIVES += $(CFGPARSER_LIB)
ARCHIVES += $(DBGLOG_LIB)

################################################################################
# andersen
################################################################################

ANDERSEN_CPP_DIR := $(PWD)/lib/andersen

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
	$(LLVMLIBS) \
	-lz3 \
	-llog4cpp \
	-lprofiler \
	-ljsoncpp \
	-lprotobuf \
	-ltinyxml2 \
	-lmathsat \
	-lgmpxx \
	-lgmp \
	-luuid \
	$(LLVMSYSTEMLIBS)

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
	@$(PROTOEXT) `find $(PWD) -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | sort`
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

# FIXME libdbglog cannot be built with 'make -jN', any workaround would be much appreciated...
.dbglog:
	[[ -e $(DBGLOG_DIR)/Makefile ]] && $(MAKE) CXX=$(CXX) -C $(DBGLOG_DIR) distclean || true
	cd $(DBGLOG_DIR); $(DBGLOG_DIR)/configure --prefix=$(DBGLOG_DIST)
	$(MAKE) CXX=$(CXX) -C $(DBGLOG_DIR) install
	touch $@

clean.dbglog:
	rm -f .dbglog
	$(MAKE) CXX=$(CXX) -C $(DBGLOG_DIR) clean
	$(MAKE) CXX=$(CXX) -C $(DBGLOG_DIR) distclean
	rm -rf $(DBGLOG_DIST)

.cfgparser: .dbglog
	[[ -e $(CFGPARSER_DIR)/Makefile ]] && $(MAKE) CXX=$(CXX) -C $(CFGPARSER_DIR) distclean || true
	cd $(CFGPARSER_DIR); $(CFGPARSER_DIR)/configure --prefix=$(CFGPARSER_DIR)/dist --with-dbglog=$(DBGLOG_DIST)
	$(MAKE) CXX=$(CXX) -C $(CFGPARSER_DIR) install
	touch $@

clean.cfgparser: clean.dbglog
	rm -f .cfgparser
	$(MAKE) CXX=$(CXX) -C $(CFGPARSER_DIR) clean
	$(MAKE) CXX=$(CXX) -C $(CFGPARSER_DIR) distclean
	rm -rf $(CFGPARSER_DIR)/dist

.andersen:
	cd $(ANDERSEN_CPP_DIR) && cmake .
	$(MAKE) CXX=$(CXX) -C $(ANDERSEN_CPP_DIR)
	touch $@

$(EXES): $(OBJECTS) .protobuf .yaml-cpp .cfgparser .andersen
	$(CXX) -fuse-ld=gold -rdynamic -g -o $@ $(OBJECTS) $(LLVMLDFLAGS) $(LIBS) $(ARCHIVES)

.google-test:
	$(MAKE) CXX=$(CXX) -C $(GOOGLE_TEST_DIR)/make gtest.a
	touch $@

clean.google-test:
	rm -f .google-test
	$(MAKE) CXX=$(CXX) -C $(GOOGLE_TEST_DIR)/make clean


$(TEST_EXES): $(TEST_OBJECTS) .google-test
	$(CXX) -fuse-ld=gold -g -o $@ $(TEST_OBJECTS) $(LLVMLDFLAGS) $(LIBS) $(ARCHIVES) $(TEST_ARCHIVES)


tests: $(EXES) $(TEST_EXES)

.regenerate-test-defs: $(TEST_DEFS)
	@find $(TEST_DIRS) -name "*.tmp" -delete


check: tests .regenerate-test-defs
	$(RUN_TEST_EXES) --gtest_filter=-*Long/*:-*Summary* # --gtest_filter=$(GOOGLE_TEST_FILTER)

check-with-valgrind: tests .regenerate-test-defs
	$(VALGRIND) $(RUN_TEST_EXES) --gtest_filter=-*Long/*:-*Summary* # --gtest_filter=$(GOOGLE_TEST_FILTER)

check-long: tests .regenerate-test-defs
	$(RUN_TEST_EXES) --gtest_filter=*Long/*:-*Summary* # --gtest_filter=$(GOOGLE_TEST_FILTER)

check-summary: tests .regenerate-test-defs
	$(RUN_TEST_EXES) --gtest_filter=*Summary*:-*Long/* # --gtest_filter=$(GOOGLE_TEST_FILTER)

check-summary-long: tests .regenerate-test-defs
	$(RUN_TEST_EXES) --gtest_filter=*SummaryLong/* # --gtest_filter=$(GOOGLE_TEST_FILTER)

check-summary-all: tests .regenerate-test-defs
	$(RUN_TEST_EXES) --gtest_filter=*Summary* # --gtest_filter=$(GOOGLE_TEST_FILTER)

check-all: tests .regenerate-test-defs
	$(RUN_TEST_EXES) --gtest_filter=-*Summary*# --gtest_filter=$(GOOGLE_TEST_FILTER)


clean:
	@rm -f $(EXES) $(OBJECTS) $(DEPS) $(TEST_EXES) $(TEST_OBJECTS) $(TEST_DEPS) $(TEST_OUTPUT)

################################################################################
-include $(DEPS)
-include $(TEST_DEPS)
################################################################################
