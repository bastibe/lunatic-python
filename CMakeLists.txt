cmake_minimum_required(VERSION 2.8.8)

set(CMAKE_BUILD_TYPE_INIT "Release")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH})

project(Lunatic)

set(PY_PATH "UNDEFINED_PY_PATH" CACHE PATH "root Python install path")
set(PY_IMPORT "" CACHE PATH "Python's import library")
set(LUA_PATH "UNDEFINED_LUA_PATH" CACHE PATH "root Lua install path")
set(LUA_IMPORT "liblua.a" CACHE PATH "Lua's import library")
include_directories("${PY_PATH}/include"
                    "${LUA_PATH}/include")
link_directories("${PY_PATH}/libs" "${LUA_PATH}/lib")

add_subdirectory(src)

add_library(python MODULE $<TARGET_OBJECTS:src>)
set_target_properties(python PROPERTIES 
                             PREFIX "")

add_library(lua MODULE $<TARGET_OBJECTS:src>)
set_target_properties(lua PROPERTIES 
                          PREFIX ""
                          SUFFIX ".pyd")

target_link_libraries(lua "${LUA_IMPORT}" "${PY_IMPORT}")
target_link_libraries(python "${LUA_IMPORT}" "${PY_IMPORT}")
