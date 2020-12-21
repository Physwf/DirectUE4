cmake_minimum_required (VERSION 3.4)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project (DirectUE4)
add_subdirectory(DirectUE4)
add_subdirectory(MikkTSpace)
add_subdirectory(mcpp-2.7.2)
add_subdirectory(ForsythTriOO)

add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

MESSAGE(STATUS "operation system is ${CMAKE_SYSTEM}")
