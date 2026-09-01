# ************************************************************
#  Copyright (C), BroadXT Hangzhou 2019
#  FileName: CMakeLists.txt
#  Author: BroadXT
#  Version: 2.0
#  Date:2019-10-21
#  Description: cmake file for DRC_Fusion algorithm project
# -----------------------------------------------------------
#  History:     
#  <author>       <time>      <version>   <desc>
#  chengbs        2019/11/21   2.0        create file
# *************************************************************

INCLUDE(FindProtobuf)

# Protobuf Package
find_package(Protobuf)
if (NOT Protobuf_FOUND)
    message("Protobuf Not Found! Start to install protobuf ... ")
    execute_process(COMMAND sudo apt-get install libprotobuf-dev protobuf-compiler libprotoc-dev)
endif (NOT Protobuf_FOUND)
include_directories(${PROTOBUF_INCLUDE_DIRS})
message("Found Protobuf in: ${PROTOBUF_INCLUDE_DIRS}")
set_property(GLOBAL PROPERTY GLOBAL_PROTOBUF_INCLUDE_DIRS "${PROTOBUF_INCLUDE_DIRS}")
set_property(GLOBAL PROPERTY GLOBAL_PROTOBUF_LIBRARY "${PROTOBUF_LIBRARY}")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

function(PROTOBUF_GENERATE_CPP_ SRCS HDRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_CPP_() called without any proto files")
    return()
  endif()

  if(PROTOBUF_GENERATE_CPP_APPEND_PATH)
    # Create an include path for each file specified
    foreach(FIL ${ARGN})
      get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
      get_filename_component(ABS_PATH ${ABS_FIL} PATH)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  else()
    set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  if(DEFINED PROTOBUF_IMPORT_DIRS AND NOT DEFINED Protobuf_IMPORT_DIRS)
    set(Protobuf_IMPORT_DIRS "${PROTOBUF_IMPORT_DIRS}")
  endif()

  if(DEFINED Protobuf_IMPORT_DIRS)
    foreach(DIR ${Protobuf_IMPORT_DIRS})
      get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
      list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${ABS_PATH})
      endif()
    endforeach()
  endif()

  set(${SRCS})
  set(${HDRS})
  
  if(NOT DEFINED Protobuf_PROTOC_EXECUTABLE)
    set(Protobuf_PROTOC_EXECUTABLE /usr/local/bin/protoc)
  endif()

  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_NAME ${FIL} NAME)
    string(REGEX MATCH "^(.*)\\.[^.]*$" dummy ${FIL_NAME})
    set(FIL_WE ${CMAKE_MATCH_1})

    list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc")
    list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h")
    
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc"
             "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h"
      COMMAND  ${Protobuf_PROTOC_EXECUTABLE}
      ARGS --cpp_out  ${CMAKE_CURRENT_BINARY_DIR} ${_protobuf_include_path} ${ABS_FIL}
      DEPENDS ${ABS_FIL} ${Protobuf_PROTOC_EXECUTABLE}
      COMMENT "Running C++ protocol buffer compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

FILE(GLOB FILE_PROTOS "*.proto")
FOREACH(FIL ${FILE_PROTOS})
    GET_FILENAME_COMPONENT(FIL_NAME ${FIL} NAME)
    LIST(APPEND FILE_NAME_PROTOS ${FIL_NAME})
ENDFOREACH()

PROTOBUF_GENERATE_CPP_(
    PROTO_SRCS 
    PROTO_HDRS 
    ${FILE_NAME_PROTOS}
)

ADD_LIBRARY(proto ${PROTO_HDRS} ${PROTO_SRCS})