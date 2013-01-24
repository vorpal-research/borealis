################################################################################
# Defs
################################################################################

CXX := clang++ # -stdlib=libc++

LLVMCOMPONENTS := analysis archive asmparser asmprinter bitreader bitwriter codegen core cppbackend cppbackendcodegen cppbackendinfo debuginfo engine executionengine instcombine instrumentation interpreter ipa ipo jit linker mc mcdisassembler mcjit mcparser native nativecodegen object runtimedyld scalaropts selectiondag support tablegen target transformutils vectorize linker

RTTIFLAG := -fno-rtti
#RTTIFLAG :=

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

CXXFLAGS += $(foreach w,$(WARNINGS_ON),-W$(w)) \
	$(foreach w,$(WARNINGS_OFF),-Wno-$(w)) \
	$(foreach w,$(WARNINGS_TAE),-Werror-$(w))

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
	$(PWD)/Query \
	$(PWD)/Solver \
	$(PWD)/State \
	$(PWD)/Term \
	$(PWD)/Util \
	$(PWD)/lib/range-analysis/src \
	$(PWD)/lib/poolalloc/src
	
ADDITIONAL_INCLUDE_DIRS := \
	$(PWD) \
	$(PWD)/lib/pegtl/include
	
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
	-llog4cpp

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
	$(CXX) -g -o $@ -rdynamic $(OBJECTS) $(LIBS) $(LLVMLDFLAGS) $(LIBS)

$(TEST_EXES): $(TEST_OBJECTS)
	$(CXX) -o $@ $(TEST_OBJECTS) $(LIBS) $(LLVMLDFLAGS) $(LIBS) -lgtest

tests: $(TEST_EXES)

check: tests
	$(PWD)/$(TEST_EXES)

clean:
	rm -f $(EXES) $(OBJECTS) $(DEPS) $(TEST_OBJECTS) $(TEST_DEPS) $(TEST_EXES)

################################################################################
-include $(DEPS)
-include $(TEST_DEPS)
################################################################################
