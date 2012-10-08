################################################################################
# Defs
################################################################################

CXX := clang++

LLVMCOMPONENTS := all

RTTIFLAG := -fno-rtti
#RTTIFLAG :=

DEFS :=

INCLUDE_DIRS := \
	$(PWD) \
	/usr/include/z3 

INCLUDES := $(foreach d, $(INCLUDE_DIRS), -I$d)

CXXFLAGS := \
	-Wall \
	-Woverloaded-virtual \
	-Wcast-qual \
	-DNDEBUG \
	-D_GNU_SOURCE \
	-D__STDC_CONSTANT_MACROS \
	-D__STDC_FORMAT_MACROS \
	-D__STDC_LIMIT_MACROS \
	-O0 \
	-fno-exceptions \
	-fPIC \
	-std=c++11 \
	-g \
	$(INCLUDES) \
	$(DEFS)

LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))

################################################################################
# Sources
################################################################################

ADDITIONAL_SOURCE_DIRS := \
	$(PWD)/Passes \
	$(PWD)/Predicate \
	$(PWD)/State \
	$(PWD)/lib/range-analysis/src \
	$(PWD)/lib/poolalloc/src

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
	-lz3

default: all

.PHONY: all clean

################################################################################
# Deps management
################################################################################
%.d: %.cpp
	@$(CXX) $(CXXFLAGS) -MM $*.cpp > $*.d
	@mv -f $*.d $*.dd
	@sed -e 's|.*:|$*.o:|' < $*.dd > $*.d
	@sed -e 's|.*:||' -e 's|\\$$||' < $*.dd | fmt -1 | \
	  sed -e 's|^\s*||' -e 's|$$|:|' >> $*.d
	@rm -f $*.dd
################################################################################
# Rules
################################################################################

all: $(EXES)

$(EXES): $(OBJECTS)
	$(CXX) -g -o $@ -rdynamic $(OBJECTS) $(LIBS) $(LLVMLDFLAGS)

$(TEST_EXES): $(TEST_OBJECTS)
	$(CXX) -o $@ $(TEST_OBJECTS) $(LIBS) $(LLVMLDFLAGS) -lgtest

tests: $(TEST_EXES)

check: tests
	$(PWD)/$(TEST_EXES)

clean:
	rm -f $(EXES) $(OBJECTS) $(DEPS) $(TEST_OBJECTS) $(TEST_DEPS) $(TEST_EXES)

################################################################################
-include $(DEPS)
-include $(TEST_DEPS)
################################################################################
