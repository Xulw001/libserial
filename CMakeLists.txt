cmake_minimum_required(VERSION 3.0)

project(libserial)
set(lib_name "serial")
set(static_lib_name "serial_static")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-DUNICODE)

include_directories(${PROJECT_SOURCE_DIR}/src)

if(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
elseif(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo"))
elseif(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
else()
SET(CMAKE_BUILD_TYPE "Debug")
endif()

SET(SYSTEM_TYPE "windows")
IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(CMAKE_SYSTEM_PROCESSOR x64)
ELSE()
    SET(CMAKE_SYSTEM_PROCESSOR x86)
ENDIF()

file(GLOB_RECURSE DIR_SRCS "src/*.h" "src/*.cc")

add_library(${lib_name} SHARED ${DIR_SRCS})
add_library(${static_lib_name} STATIC ${DIR_SRCS})
set_target_properties(${static_lib_name} PROPERTIES OUTPUT_NAME ${lib_name})

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
target_link_libraries(${lib_name} pthread)
endif()

add_subdirectory(test)

