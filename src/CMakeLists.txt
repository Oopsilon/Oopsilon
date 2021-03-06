cmake_minimum_required (VERSION 3.2)
project (Oopsilon)

include (CheckIncludeFiles)
include (CheckIncludeFileCXX)
include (CheckFunctionExists)
include (GNUInstallDirs)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/External)

check_include_files   (malloc.h HAVE_MALLOC_H)
check_include_files   (unistd.h HAVE_UNISTD_H)
check_include_files   (fcntl.h  HAVE_FCNTL_H)
check_include_file_cxx(cxxabi.h HAVE_CXXABI_H)
check_function_exists (mkstemp HAVE_MKSTEMP)

find_package(FLEX)

if (NOT FLEX_FOUND)
	set(FLEX_FOUND true)
	set(FLEX_EXECUTABLE ${PROJECT_SOURCE_DIR}/cmd/flex/win_flex.exe)
	message(STATUS "Flex for Windows used: " ${FLEX_EXECUTABLE})
endif()

if(MSVC)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w44103 /EHsc")
else (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
                          -std=c++11 -Wno-write-strings")
endif()

function (FlexComp path)
GET_FILENAME_COMPONENT (file ${path} NAME_WE)
add_custom_command(
OUTPUT ${PROJECT_BINARY_DIR}/${file}.yy.h
	${PROJECT_BINARY_DIR}/${file}.yy.cxx
COMMAND ${FLEX_EXECUTABLE}
ARGS -o${PROJECT_BINARY_DIR}/${file}.yy.cxx
    --header-file=${PROJECT_BINARY_DIR}/${file}.yy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/${path}
DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${path}
)
endfunction (FlexComp)

function (LemonComp path)
GET_FILENAME_COMPONENT (file ${path} NAME_WE)
add_custom_command(
OUTPUT ${PROJECT_BINARY_DIR}/${file}.tab.h
    ${PROJECT_BINARY_DIR}/${file}.tab.cxx
	${PROJECT_BINARY_DIR}/${file}.out
COMMAND $<TARGET_FILE:lemon>
ARGS ${CMAKE_CURRENT_SOURCE_DIR}/${path} -n${file}
	-d./ -p -T${CMAKE_SOURCE_DIR}/cmd/lemon/lempar.tpl.cxx
DEPENDS $<TARGET_FILE:lemon>
MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${path}
WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)
endfunction (LemonComp)

function (Prepend var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
endfunction (Prepend)

include_directories ($<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/hdr>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/hdr>
    $<INSTALL_INTERFACE:include>)

add_subdirectory (lib)
add_subdirectory (cmd)

add_dependencies (c-oopsC oopsRt)

#INSTALL(DIRECTORY OopsFoundation DESTINATION ${CMAKE_INSTALL_LIBDIR})
