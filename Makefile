CXX := clang++
LLVMCOMPONENTS := all
RTTIFLAG := -fno-rtti
#RTTIFLAG :=
CXXFLAGS := $(shell llvm-config --cxxflags) $(RTTIFLAG) -std=c++11
LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))
DDD := $(shell echo $(LLVMLDFLAGS))
SOURCES = $(shell ls *.cpp)

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
    -lclangBasic \

all: $(OBJECTS) $(EXES)

wrapper: wrapper.o comments.o util.o
	$(CXX) -o $@ wrapper.o comments.o util.o $(CLANGLIBS) $(LLVMLDFLAGS)

clean:
	-rm -f $(EXES) $(OBJECTS) *~