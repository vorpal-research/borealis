CXX := clang++

LLVMCOMPONENTS := all

RTTIFLAG := -fno-rtti
#RTTIFLAG :=

DEFS = -DDEBUG_PRINT=true

CXXFLAGS := $(shell llvm-config --cxxflags) $(RTTIFLAG) -std=c++11 -g $(DEFS)
LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))

DDD := $(shell echo $(LLVMLDFLAGS))

ADDITIONAL_SOURCE_DIRS = $(PWD)/Passes \
	$(PWD)/lib/range-analysis/src \
	$(PWD)/lib/poolalloc/src
SOURCES = $(shell ls *.cpp) \
    $(shell find $(ADDITIONAL_SOURCE_DIRS) -type f | grep "\.cpp")

OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = $(shell ls *.hpp)
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

all: $(OBJECTS) $(HEADERS) $(EXES)

wrapper: $(OBJECTS) $(HEADERS)
	$(CXX) -o $@ -rdynamic $(OBJECTS) $(CLANGLIBS) $(LLVMLDFLAGS) -g

clean:
	-rm -f $(EXES) $(OBJECTS) *~
