CXX := clang++
LLVMCOMPONENTS := all
RTTIFLAG := -fno-rtti
#RTTIFLAG :=
CXXFLAGS := -Wall -DNDEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O0 -fno-exceptions -fPIC -Woverloaded-virtual -Wcast-qual -std=c++11 -g
#CXXFLAGS := $(shell llvm-config --cxxflags) $(RTTIFLAG) -std=c++11 -g
LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))
DDD := $(shell echo $(LLVMLDFLAGS))
ADDITIONAL_SOURCE_DIRS = $(PWD)/Passes $(PWD)/lib/range-analysis/src $(PWD)/lib/poolalloc/src
SOURCES = $(shell ls *.cpp) \
    $(shell find $(ADDITIONAL_SOURCE_DIRS) -type f | grep "\.cpp")

OBJECTS = $(SOURCES:.cpp=.o)
EXES = wrapper
CLANGLIBS = \
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

all: $(OBJECTS) $(EXES)

wrapper: $(OBJECTS)
	$(CXX) -o $@ -rdynamic $(OBJECTS) $(CLANGLIBS) $(LLVMLDFLAGS) -g

clean:
	-rm -f $(EXES) $(OBJECTS) *~