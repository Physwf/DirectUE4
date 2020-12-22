set(DIR_PROJECT "${CMAKE_CURRENT_SOURCE_DIR}/")
file(GLOB_RECURSE DIR_SRC
    "./*.cpp"
    "./*.h"
    "./*.inl"
	"./*.dush"
    "./*.dusf"
)
if(WIN32)
add_compile_options(/wd4244)
else(WIN32)
endif(WIN32)
foreach(_source IN ITEMS ${DIR_SRC})
    get_filename_component(_source_path "${_source}" PATH)
	file(RELATIVE_PATH _source_path_rel "${DIR_PROJECT}" "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()
set(CMAKE_CXX_STANDARD 17)
add_executable(DirectUE4 ${DIR_SRC})
#add_executable(DirectUE4 ${DIR_SRC})
#set(DirectXTexDir "D:/DirectXTex/")
MESSAGE(STATUS "FBX_SDK_HOME=$ENV{FBX_SDK_HOME}")
MESSAGE(STATUS "FBX_SDK_HOME=$ENV{FBX_SDK_HOME}")
target_include_directories(DirectUE4 PRIVATE "")
target_include_directories(DirectUE4 PRIVATE "Animation")
target_include_directories(DirectUE4 PRIVATE "CompositionLighting")
target_include_directories(DirectUE4 PRIVATE "Light")
target_include_directories(DirectUE4 PRIVATE "Material")
target_include_directories(DirectUE4 PRIVATE "Texture")
target_include_directories(DirectUE4 PRIVATE "Math")
target_include_directories(DirectUE4 PRIVATE "Templates")
target_include_directories(DirectUE4 PRIVATE "Mesh")
target_include_directories(DirectUE4 PRIVATE "PostProcess")
target_include_directories(DirectUE4 PRIVATE "Renderer")
target_include_directories(DirectUE4 PRIVATE "RHI")
target_include_directories(DirectUE4 PRIVATE "Scene")
target_include_directories(DirectUE4 PRIVATE "Utilities")
target_include_directories(DirectUE4 PRIVATE "../MikkTSpace/inc")
target_include_directories(DirectUE4 PRIVATE "../MikkTSpace/inc")
target_include_directories(DirectUE4 PRIVATE "../mcpp-2.7.2/inc")
target_include_directories(DirectUE4 PRIVATE "../ForsythTriOO/Src")
target_include_directories(DirectUE4 PRIVATE $ENV{FBX_SDK_HOME}/include)
find_library(FBXSDK_LIBRARY libfbxsdk HINTS $ENV{FBX_SDK_HOME}/lib/vs2015/x86/release)
target_include_directories(DirectUE4 PRIVATE $ENV{DirectXTexDir}DirectXTex)
find_library(DirectXTexLib DirectXTex HINTS $ENV{DirectXTexDir}DirectXTex/Bin/Desktop_2017_Win10/Win32/Debug/)

#https://www.jianshu.com/p/66df9650a9e2
ADD_CUSTOM_COMMAND(TARGET DirectUE4
        POST_BUILD
        COMMENT "Copy Shaders"
        COMMAND @if exist $(TargetDir)\\Shaders\\ (@echo "No Need mkdir") else ( @mkdir  $(TargetDir)\\Shaders\\ )
        COMMAND @copy $(SolutionDir)..\\DirectUE4\\Shaders\\*.dusf  $(TargetDir)\\Shaders\\
)

add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")

#configure_file(${DIR_PROJECT}/Triangle.dusf ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
#copy $(SolutionDir)..\DirectUE4\Shaders\*.dusf  $(TargetDir)
SET(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})

MESSAGE(STATUS "operation system is ${CMAKE_SYSTEM}")

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
	MESSAGE(STATUS "current platform: Linux ")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
	MESSAGE(STATUS "current platform: Windows")
	add_definitions(-DPLATFORM_WINDOWS)
	target_link_libraries(DirectUE4 MikkTSpace)
	target_link_libraries (DirectUE4 "MCPP")
	target_link_libraries (DirectUE4 "ForsythTriOptimizer")
	target_link_libraries (DirectUE4 "d3d11.lib")
	target_link_libraries (DirectUE4 "d3d9.lib")
	target_link_libraries (DirectUE4 "dxgi.lib")
	target_link_libraries (DirectUE4 "D3DCompiler.lib")
	target_link_libraries (DirectUE4 ${FBXSDK_LIBRARY})
	target_link_libraries (DirectUE4 ${DirectXTexLib})
	target_link_libraries (DirectUE4 dxguid.lib)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
	MESSAGE(STATUS "current platform: FreeBSD")
ELSE ()
	MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

MESSAGE(STATUS "###################################")

IF (WIN32)
	MESSAGE(STATUS "Now is windows")
ELSEIF (APPLE)
	MESSAGE(STATUS "Now is Apple systens.")
ELSEIF (UNIX)
	MESSAGE(STATUS "Now is UNIX-like OS's. Including aPPLE os x  and CygWin")
ENDIF ()
