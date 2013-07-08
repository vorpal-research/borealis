################################################################################
# Defs
################################################################################

CXX := clang++
# -stdlib=libc++

LLVMCOMPONENTS := analysis archive asmparser asmprinter bitreader bitwriter codegen core cppbackend cppbackendcodegen cppbackendinfo debuginfo engine executionengine instcombine instrumentation interpreter ipa ipo jit linker mc mcdisassembler mcjit mcparser native nativecodegen object runtimedyld scalaropts selectiondag support tablegen target transformutils vectorize linker

RTTIFLAG := -fno-rtti

DEFS :=

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
CXXFLAGS += $(foreach w,$(WARNINGS_ON),-W$(w)) \
	$(foreach w,$(WARNINGS_OFF),-Wno-$(w)) \
	$(foreach w,$(WARNINGS_TAE),-Werror-$(w))
endif

################################################################################
# Google Test
################################################################################

GOOGLE_TEST_DIR := $(PWD)/lib/google-test

GOOGLE_TEST_LIB := $(GOOGLE_TEST_DIR)/make/gtest.a

CXXFLAGS += -isystem $(GOOGLE_TEST_DIR)/include

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
	$(PWD)/Solver \
	$(PWD)/State \
	$(PWD)/Term \
	$(PWD)/Type \
	$(PWD)/Util \
	$(PWD)/lib/range-analysis/src \
	$(PWD)/lib/poolalloc/src
	
ADDITIONAL_INCLUDE_DIRS := \
	$(PWD) \
	$(PWD)/lib/pegtl/include \
	$(PWD)/lib/google-test/include
	
CXXFLAGS += $(foreach dir,$(ADDITIONAL_INCLUDE_DIRS),-I"$(dir)")

SOURCES := \
	$(shell ls *.cpp) \
	$(shell find $(ADDITIONAL_SOURCE_DIRS) -name "*.cpp" -type f)
	
SOURCES_WITH_MAIN := wrapper.cpp

SOURCES_WITHOUT_MAIN := $(filter-out $(SOURCES_WITH_MAIN),$(SOURCES))

OBJECTS := $(SOURCES:.cpp=.o)
HEADERS := $(SOURCES:.cpp=.h)
DEPS := $(SOURCES:.cpp=.d)

################################################################################
# Tests
################################################################################

TEST_DIRS := $(PWD)/test
TEST_SOURCES := \
	$(shell find $(TEST_DIRS) -name "*.cpp" -type f)

TEST_OBJECTS := $(SOURCES_WITHOUT_MAIN:.cpp=.o) $(TEST_SOURCES:.cpp=.o)
TEST_HEADERS := $(TEST_SOURCES:.cpp=.h)
TEST_DEPS := $(TEST_SOURCES:.cpp=.d)

TEST_OUTPUT := "test_results.xml"

################################################################################
# Exes
################################################################################

EXES := wrapper
TEST_EXES := run-tests

CLANGLIBS := \
    -lclangCodeGen \
    -lclangFrontend \
    -lclangDriver \
    -lclangSerialization \
    -lclangParse \
    -lclangSema \
    -lclangAnalysis \
    -lclangRewrite \
    -lclangEdit \
    -lclangAST \
    -lclangLex \
    -lclangBasic 

LIBS := \
	$(CLANGLIBS) \
	-lz3 \
	-ldl \
	-lrt \
	-lcfgparser \
	-llog4cpp \
	-lprofiler \
	-ljsoncpp

default: all

.PHONY: all tests regenerate-test-defs check clean .FORCE

.FORCE:

################################################################################
# Test defs management
################################################################################

TEST_DEFS := $(shell find $(TEST_DIRS) -name "tests.def")

$(TEST_DEFS): .FORCE
	ls -1 $(@D) | grep '^.*.c$$' | xargs -n1 basename > $@.all
	sed 's/^#\s*//' $(@D)/"tests.long.def" > $@.long.all
	grep -v -x -f $@.long.all $@.all > $@



RUN_TEST_EXES := $(PWD)/$(TEST_EXES) \
	--gtest_output="xml:$(TEST_OUTPUT)" \
	--gtest_color=yes

################################################################################
# Valgrind
################################################################################

VALGRIND := valgrind --leak-check=yes --suppressions=valgrind.supp

################################################################################
# Deps management
################################################################################
%.d: %.cpp
	@$(CXX) $(CXXFLAGS) -MM $*.cpp > $*.d
	@mv -f $*.d $*.dd
	@sed -e 's|.*:|$*.o:|' < $*.dd > $*.d
#	@sed -e 's|.*:||' -e 's|\\$$||' < $*.dd | fmt -1 | \
#	  sed -e 's|^\s*||' -e 's|$$|:|' >> $*.d
	@rm -f $*.dd
################################################################################
# Rules
################################################################################

all: $(EXES)

$(EXES): $(OBJECTS)
	$(CXX) -g -o $@ -rdynamic $(OBJECTS) $(LIBS) $(LLVMLDFLAGS) $(LIBS)

google-test:
	$(MAKE) CXX=$(CXX) -C $(GOOGLE_TEST_DIR)/make gtest.a

clean-google-test:
	$(MAKE) CXX=$(CXX) -C $(GOOGLE_TEST_DIR)/make clean

$(TEST_EXES): $(TEST_OBJECTS) google-test
	$(CXX) -g -o $@ $(TEST_OBJECTS) $(LIBS) $(LLVMLDFLAGS) $(LIBS) $(GOOGLE_TEST_LIB)

tests: $(EXES) $(TEST_EXES)

regenerate-test-defs: $(TEST_DEFS)

check: tests regenerate-test-defs
	$(RUN_TEST_EXES) --gtest_filter=-*Long/*

check-with-valgrind: tests regenerate-test-defs
	$(VALGRIND) $(RUN_TEST_EXES) --gtest_filter=-*Long/*

check-long: tests regenerate-test-defs
	$(RUN_TEST_EXES) --gtest_filter=*Long/*

check-all: tests regenerate-test-defs
	$(RUN_TEST_EXES)

clean: clean-google-test
	rm -f $(EXES) $(OBJECTS) $(DEPS) $(TEST_OBJECTS) $(TEST_DEPS) $(TEST_EXES) $(TEST_OUTPUT)

################################################################################
-include $(DEPS)
-include $(TEST_DEPS)
################################################################################
