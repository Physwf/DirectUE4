project(ForsythTriOO)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#IF (CMAKE_BUILD_TYPE STREQUAL "Debug") 
#add_library(ForsythTriOptimizerd STATIC Src/forsythtriangleorderoptimizer.cpp Src/forsythtriangleorderoptimizer.h)
#SET_TARGET_PROPERTIES(ForsythTriOptimizerd PROPERTIES COMPILE_FLAGS "-fPIC")
#ENDIF (CMAKE_BUILD_TYPE STREQUAL "Debug") 

#IF (CMAKE_BUILD_TYPE STREQUAL "Release") 
add_library(ForsythTriOptimizer STATIC Src/forsythtriangleorderoptimizer.cpp Src/forsythtriangleorderoptimizer.h)
#SET_TARGET_PROPERTIES(ForsythTriOptimizer PROPERTIES COMPILE_FLAGS "-fPIC")
#ENDIF (CMAKE_BUILD_TYPE STREQUAL "Release") 
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})