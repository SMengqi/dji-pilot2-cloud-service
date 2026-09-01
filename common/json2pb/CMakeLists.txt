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
project(json2pb)
CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++11)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../../drc/common)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../../common)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../../common/if)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../../../common/if_pl)
#message(${CMAKE_CURRENT_BINARY_DIR}/../../../common)
AUX_SOURCE_DIRECTORY(. DIR_SRCS)
ADD_LIBRARY(json2pb ${DIR_SRCS})
