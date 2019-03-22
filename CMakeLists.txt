cmake_minimum_required(VERSION 3.2)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(borealis)

################################################################################
# Link directories
################################################################################
link_directories(/usr/lib)                              # system libraries directory
link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib)       # subprojects output directory

################################################################################
# LLVM
################################################################################
find_package(LLVM QUIET REQUIRED COMPONENTS ALL
        CONFIG
        PATHS /opt/clang/3.5.1/share/llvm/cmake/
        NO_DEFAULT_PATH)

link_directories(${LLVM_LIBRARY_DIRS})

if (NOT ${LLVM_FOUND})
    message(FATAL_ERROR "LLVM libraries not found")
endif ()

################################################################################
# Warnings
################################################################################
set(WARNINGS_ON all extra cast-qual float-equal switch
        undef init-self pointer-arith cast-align effc++
        strict-prototypes strict-overflow=5 write-strings
        aggregate-return super-class-method-mismatch)

set(WARNINGS_OFF unused-function redundant-decls unused-local-typedef
        pessimizing-move return-std-move)

set(WARNINGS_TAE overloaded-virtual return-stack-address
        implicit-function-declaration address-of-temporary
        delete-non-virtual-dtor return-type)

if (${CMAKE_CXX_COMPILER} STREQUAL "/usr/bin/clang++")
    foreach(w ${WARNINGS_ON})
        list(APPEND COMPILER_WARNINGS -W${w})
    endforeach()
    foreach(w ${WARNINGS_OFF})
        list(APPEND COMPILER_WARNINGS -Wno-${w})
    endforeach()
    foreach(w ${WARNINGS_TAE})
        list(APPEND COMPILER_WARNINGS -Werror-${w})
    endforeach()
endif ()

################################################################################
# Compiler flags
################################################################################
add_definitions(${LLVM_DEFINITIONS}
        -std=c++14)

set(COMPILER_DEFINITIONS
        -D_GLIBCXX_USE_CXX11_ABI=1
        -DGOOGLE_PROTOBUF_NO_RTTI
        -DPROTOBUF_INLINE_NOT_IN_HEADERS=0
        -DPROTOBUF_MIN_PROTOC_VERSION=1000000   # define this to aviod warnings during protobuf compilation
        -DNO_TRACING)

set(COMPILER_FLAGS
        ${CMAKE_CXX_FLAGS}
        ${LLVM_CXX_FLAGS}
        ${LLVM_LD_FLAGS}
        -fPIC
        -fno-rtti
        ${COMPILER_WARNINGS})

list(APPEND COMPILER_FLAGS -O2 -g)

set(PROTO_COMPILER_FLAGS
        ${CMAKE_CXX_FLAGS}
        -O2
        -g
        -fPIC
        -fno-rtti)

################################################################################
# Include directories
################################################################################
set(INCLUDE_DIRS
        ${PROJECT_SOURCE_DIR}/Protobuf/Gen
        ${PROJECT_SOURCE_DIR}/lib
        ${PROJECT_SOURCE_DIR}/lib/pegtl/include
        ${PROJECT_SOURCE_DIR}/lib/rapidjson/include
        ${PROJECT_SOURCE_DIR}/lib/google-test/include
        ${PROJECT_SOURCE_DIR}/lib/yaml-cpp/include
        ${PROJECT_SOURCE_DIR}/lib/yaml-cpp/test
        ${PROJECT_SOURCE_DIR}/lib/log4cpp/include
        ${PROJECT_SOURCE_DIR}/lib/backward-cpp
        ${PROJECT_SOURCE_DIR}/lib/leveldb-mp/include
        ${PROJECT_SOURCE_DIR}/lib/leveldb-mp/libsocket/headers)

include_directories(${PROJECT_SOURCE_DIR} ./ ../)

include_directories(SYSTEM /usr/include/)
include_directories(SYSTEM /usr/include/z3)             # using SYSTEM parameter to exclude
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})        # compiler warnings on include libraries
include_directories(SYSTEM ${INCLUDE_DIRS})

################################################################################
# Source directories
################################################################################

set(SOURCE_DIRS
        # ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/Actions
        ${PROJECT_SOURCE_DIR}/Anno
        ${PROJECT_SOURCE_DIR}/Annotation
        ${PROJECT_SOURCE_DIR}/Codegen
        ${PROJECT_SOURCE_DIR}/Config
        ${PROJECT_SOURCE_DIR}/Database
        ${PROJECT_SOURCE_DIR}/Driver
        ${PROJECT_SOURCE_DIR}/Executor
        ${PROJECT_SOURCE_DIR}/Factory
        ${PROJECT_SOURCE_DIR}/Interpreter
        ${PROJECT_SOURCE_DIR}/Logging
        ${PROJECT_SOURCE_DIR}/Passes
        ${PROJECT_SOURCE_DIR}/Predicate
        # ${PROJECT_SOURCE_DIR}/Protobuf
        ${PROJECT_SOURCE_DIR}/Reanimator
        ${PROJECT_SOURCE_DIR}/SMT
        ${PROJECT_SOURCE_DIR}/State
        ${PROJECT_SOURCE_DIR}/Statistics
        ${PROJECT_SOURCE_DIR}/Term
        ${PROJECT_SOURCE_DIR}/Type
        ${PROJECT_SOURCE_DIR}/Util)

################################################################################
# Protobuf generation
################################################################################
set(PROTO_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Protobuf/Gen)
set(PROTOC protoc --proto_path=${PROJECT_SOURCE_DIR} --cpp_out=${PROTO_SOURCE_DIR})
set(PROTOEXT ${PROJECT_SOURCE_DIR}/extract-protobuf-desc.awk)

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/.protobuf")
    message("-- Generating protobuf files")
    file(MAKE_DIRECTORY ${PROTO_SOURCE_DIR})
    file(GLOB_RECURSE PROTO_DESC_FILES ${PROJECT_SOURCE_DIR}/*.cpp
            ${PROJECT_SOURCE_DIR}/*.h
            ${PROJECT_SOURCE_DIR}/*.hpp)

    execute_process(COMMAND ${PROTOEXT} ${PROTO_DESC_FILES}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

    file(GLOB_RECURSE PROTO_FILES ${PROJECT_SOURCE_DIR}/*.proto)
    foreach(proto_file ${PROTO_FILES})
        execute_process(COMMAND ${PROTOC} ${proto_file})
    endforeach()

    file(WRITE ${PROJECT_SOURCE_DIR}/.protobuf "")
endif ()

add_custom_target(clean.protobuf
        COMMAND rm ${PROJECT_SOURCE_DIR}/.protobuf || true
        COMMAND find ${PROJECT_SOURCE_DIR} -name *.proto -delete
        COMMAND rm -rf ${PROTO_SOURCE_DIR})

add_custom_target(regenerate_protobuf
        COMMAND make clean.protobuf
        COMMAND cmake ${PROJECT_SOURCE_DIR})

################################################################################
# Protobuf sources
################################################################################

set(PROTO_DIR ${PROJECT_SOURCE_DIR}/Protobuf)
file(GLOB_RECURSE PROTO_SRC ${PROTO_DIR}/*.cc
                            ${PROTO_DIR}/*.cpp
                            ${PROTO_DIR}/*.h
                            ${PROTO_DIR}/*.hpp)

foreach(file ${PROTO_SRC})
    list(APPEND PROTO_SOURCES "${file}")
endforeach()

add_library(protobuf_objects OBJECT ${PROTO_SOURCES})

target_compile_options(protobuf_objects PUBLIC ${PROTO_COMPILER_FLAGS})
target_compile_definitions(protobuf_objects PUBLIC ${COMPILER_DEFINITIONS})

################################################################################
# Source files
################################################################################

foreach(dir ${SOURCE_DIRS})
    file(GLOB_RECURSE SRC ${dir}/*.cc
            ${dir}/*.cpp)
    foreach(file ${SRC})
        list(APPEND SOURCES "${file}")
    endforeach()
endforeach()

add_library(source_objects OBJECT ${SOURCES})

target_compile_options(source_objects PUBLIC ${COMPILER_FLAGS})
target_compile_definitions(source_objects PUBLIC ${COMPILER_DEFINITIONS})

################################################################################
# Header files
################################################################################

foreach(dir ${SOURCE_DIRS})
    file(GLOB_RECURSE SRC ${dir}/*.h
            ${dir}/*.hpp)
    foreach(file ${SRC})
        list(APPEND HEADERS "${file}")
    endforeach()
endforeach()

################################################################################
# Test sources
################################################################################
set(TEST_DIRS ${PROJECT_SOURCE_DIR}/test)

foreach(dir ${TEST_DIRS})
    file(GLOB_RECURSE SRC ${dir}/*.cpp)
    foreach(file ${SRC})
        list(APPEND TEST_SOURCES "${file}")
    endforeach()
endforeach()

add_library(test_objects OBJECT ${TEST_SOURCES})

target_compile_options(test_objects PUBLIC ${COMPILER_FLAGS})
target_compile_definitions(test_objects PUBLIC ${COMPILER_DEFINITIONS})

################################################################################
# Google Test
################################################################################
set(GOOGLE_TEST_DIR ${PROJECT_SOURCE_DIR}/lib/google-test)

add_subdirectory(${GOOGLE_TEST_DIR})

link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib/google-test)
list(APPEND TEST_ARCHIVES gtest)

################################################################################
# yaml-cpp
################################################################################
set(YAML_CPP_DIR ${PROJECT_SOURCE_DIR}/lib/yaml-cpp)

add_subdirectory(${YAML_CPP_DIR})

link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib/yaml-cpp)
list(APPEND ARCHIVES yaml-cpp)

################################################################################
# log4cpp
################################################################################
set(LOG_4_CPP_DIR ${PROJECT_SOURCE_DIR}/lib/log4cpp)
set(LOG4CPP_LIBRARY_NAME "log4cpp")


add_subdirectory(${LOG_4_CPP_DIR})

link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib/log4cpp)
list(APPEND ARCHIVES ${LOG4CPP_LIBRARY_NAME})

################################################################################
# LevelDB-MP
################################################################################
set(LEVELDB_MP_SOURCE_DIR ${PROJECT_SOURCE_DIR}/lib/leveldb-mp)

add_subdirectory(${LEVELDB_MP_SOURCE_DIR})


link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib/leveldb-mp)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib/leveldb-mp/libsocket/C++)

list(APPEND ARCHIVES socket++)          #needed for leveldb-mp
list(APPEND ARCHIVES leveldb_mp)

################################################################################
# andersen
################################################################################
set(ANDERSEN_CPP_DIR ${PROJECT_SOURCE_DIR}/lib/andersen)

add_subdirectory(${ANDERSEN_CPP_DIR})

link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib/andersen/lib)
list(APPEND ARCHIVES libAndersen.so)

################################################################################
# inih
################################################################################
set(INIH_CPP_DIR ${PROJECT_SOURCE_DIR}/lib/inih)

add_subdirectory(${INIH_CPP_DIR})

link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib/inih)
list(APPEND ARCHIVES libinih.a)

################################################################################
# Valgrind
################################################################################
set(VALGRIND "valgrind --leak-check=yes --suppressions=valgrind.supp")

################################################################################
# Libraries
################################################################################
set(SYSTEM_LIBS
        z3
        cvc4
        boolector
        lgl
        cln
        profiler
        jsoncpp
        protobuf
        tinyxml2
        mathsat
        stp
        gmpxx
        gmp
        uuid
        stdc++
        rt
        dl
        curses
        pthread
        z
        m)

set(CLANG_LIBS
        clangFrontendTool
        clangFrontend
        clangDriver
        clangSerialization
        clangCodeGen
        clangParse
        clangSema
        clangStaticAnalyzerFrontend
        clangStaticAnalyzerCheckers
        clangStaticAnalyzerCore
        clangAnalysis
        clangARCMigrate
        clangRewrite
        clangEdit
        clangAST
        clangLex
        clangBasic)

################################################################################
# Executables
################################################################################
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

# wrapper
add_executable(wrapper wrapper.cpp $<TARGET_OBJECTS:source_objects>
                                   $<TARGET_OBJECTS:protobuf_objects>
                                   ${HEADERS})

target_compile_options(wrapper PUBLIC ${COMPILER_FLAGS})
target_compile_definitions(wrapper PUBLIC ${COMPILER_DEFINITIONS})

add_dependencies(wrapper
        inih
        yaml-cpp
        Andersen)

target_link_libraries(wrapper ${ARCHIVES})
target_link_libraries(wrapper ${SYSTEM_LIBS} ${CLANG_LIBS} ${LLVM_AVAILABLE_LIBS})

# ar_wrapper
add_executable(ar_wrapper ar_wrapper.cpp $<TARGET_OBJECTS:source_objects>
                                         $<TARGET_OBJECTS:protobuf_objects>
                                         ${HEADERS})

target_compile_options(ar_wrapper PUBLIC ${COMPILER_FLAGS})
target_compile_definitions(ar_wrapper PUBLIC ${COMPILER_DEFINITIONS})

add_dependencies(ar_wrapper
        inih
        yaml-cpp
        Andersen)

target_link_libraries(ar_wrapper ${ARCHIVES})
target_link_libraries(ar_wrapper ${SYSTEM_LIBS} ${CLANG_LIBS} ${LLVM_AVAILABLE_LIBS})

################################################################################
# Test executable
################################################################################
set(TEST_OUTPUT test_results.xml)
set(TEST_EXE wrapper-tests)
set(RUN_TEST_EXES ${PROJECT_SOURCE_DIR}/${TEST_EXE} --gtest_output="xml:${TEST_OUTPUT}" --gtest_color=yes)

add_executable(${TEST_EXE} EXCLUDE_FROM_ALL $<TARGET_OBJECTS:source_objects>
                                            $<TARGET_OBJECTS:test_objects>
                                            $<TARGET_OBJECTS:protobuf_objects>
                                            ${HEADERS})
add_dependencies(${TEST_EXE}
        wrapper
        ar_wrapper
        inih
        yaml-cpp
        Andersen
        gtest)

target_compile_options(${TEST_EXE} PUBLIC ${COMPILER_FLAGS})
target_compile_definitions(${TEST_EXE} PUBLIC ${COMPILER_DEFINITIONS})

target_link_libraries(${TEST_EXE} ${ARCHIVES} ${TEST_ARCHIVES})
target_link_libraries(${TEST_EXE} ${SYSTEM_LIBS} ${CLANG_LIBS} ${LLVM_AVAILABLE_LIBS})

################################################################################
# Regenerate tests.def files
################################################################################

execute_process(COMMAND find ${TEST_DIRS} -name "tests.def" -printf "%h;" OUTPUT_VARIABLE TEST_DEF_FILES)

foreach(def_file_dir ${TEST_DEF_FILES})
    file(GLOB_RECURSE SRC ${def_file_dir}/*.c)
    file(WRITE ${def_file_dir}/tests.def.all "")
    file(WRITE ${def_file_dir}/tests.def.long.all "")

    foreach(test_file ${SRC})
        get_filename_component(name ${test_file} NAME)
        file(APPEND ${def_file_dir}/tests.def.all "${name}\n")
    endforeach()

    file(READ ${def_file_dir}/tests.def.long FILECONTENT)
    if (NOT (FILECONTENT STREQUAL ""))
        string(REGEX REPLACE "#[ \t]*" "" CONTENT_WITHOUT_COMM ${FILECONTENT})
        file(APPEND ${def_file_dir}/tests.def.long.all ${CONTENT_WITHOUT_COMM})
    endif ()

    execute_process(COMMAND grep -v -x -f ${def_file_dir}/tests.def.long.all ${def_file_dir}/tests.def.all
            OUTPUT_VARIABLE RUNNING_TESTS)
    list(SORT RUNNING_TESTS)
    file(WRITE ${def_file_dir}/tests.def ${RUNNING_TESTS})
endforeach()

################################################################################
# Testing targets
################################################################################
add_custom_target(.regenerate-test-defs
        COMMAND find ${TEST_DIRS} -name "*.tmp" -delete)

add_custom_target(check
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${RUN_TEST_EXES} --gtest_filter=-*Long/*:-*Summary* || true)
add_dependencies(check
        .regenerate-test-defs
        ${TEST_EXE})

add_custom_target(check-with-valgrind
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${VALGRIND} ${RUN_TEST_EXES} --gtest_filter=-*Long/*:-*Summary* || true)
add_dependencies(check-with-valgrind
        .regenerate-test-defs
        ${TEST_EXE})

add_custom_target(check-long
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${RUN_TEST_EXES} --gtest_filter=*Long/*:-*Summary* || true)
add_dependencies(check-long
        .regenerate-test-defs
        ${TEST_EXE})

add_custom_target(check-summary
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${RUN_TEST_EXES} --gtest_filter=*Summary*:-*Long/* || true)
add_dependencies(check-summary
        .regenerate-test-defs
        ${TEST_EXE})

add_custom_target(check-summary-long
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${RUN_TEST_EXES} --gtest_filter=*SummaryLong/* || true)
add_dependencies(check-summary-long
        .regenerate-test-defs
        ${TEST_EXE})

add_custom_target(check-all
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${RUN_TEST_EXES} --gtest_filter=-*Summary* || true)
add_dependencies(check-all
        .regenerate-test-defs
        ${TEST_EXE})

################################################################################
# Custom clean target
################################################################################
add_custom_target(clean-all
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND make clean.cfgparser
        COMMAND make clean)
