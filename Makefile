################################################################################
# Defs
################################################################################

CXX := clang++

LLVMCOMPONENTS := all

RTTIFLAG := -fno-rtti
#RTTIFLAG :=

DEFS :=

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
	$(DEFS)

LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))

ADDITIONAL_SOURCE_DIRS := \
	$(PWD)/Passes \
	$(PWD)/Predicate \
	$(PWD)/lib/range-analysis/src \
	$(PWD)/lib/poolalloc/src

SOURCES := \
	$(shell ls *.cpp) \
    $(shell find $(ADDITIONAL_SOURCE_DIRS) -name "*.cpp" -type f)

OBJECTS := $(SOURCES:.cpp=.o)
HEADERS := $(SOURCES:.cpp=.h)

EXES := wrapper

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

default: all

################################################################################
# Deps management
################################################################################
deps: .DEPS

.DEPS: $(SOURCES)
		$(CXX) $(CXXFLAGS) -MM $^ > .DEPS
################################################################################
# Rules
################################################################################

all: $(EXES)

wrapper: $(OBJECTS)
	$(CXX) -g -o $@ -rdynamic $(OBJECTS) $(CLANGLIBS) $(LLVMLDFLAGS)

clean:
	-rm -f $(EXES) $(OBJECTS)

################################################################################
-include .DEPS
################################################################################
