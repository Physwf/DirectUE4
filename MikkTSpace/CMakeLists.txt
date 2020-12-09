PROJECT(MikkTSpace)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

if(WIN32)
add_compile_options(/W4)
add_compile_options(/wd4456)
add_compile_options(/wd4244)
add_compile_options(/wd4204)
add_compile_options(/wd4201)
else(WIN32)
endif(WIN32)

INCLUDE_DIRECTORIES(inc)
ADD_LIBRARY(MikkTSpace STATIC src/mikktspace.c inc/mikktspace.h)
SET_TARGET_PROPERTIES(MikkTSpace PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

