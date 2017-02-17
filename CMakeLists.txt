cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_BUILD_TYPE_INIT "Release")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${EXECUTABLE_OUTPUT_PATH})


project(Lunatic)

find_package(Lua        5.1 REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)


add_subdirectory(src)

add_library(python MODULE $<TARGET_OBJECTS:src>)
set_target_properties(python PROPERTIES 
                             PREFIX "")

add_library(lua MODULE $<TARGET_OBJECTS:src>)
if (WIN32)
  set_target_properties(lua PROPERTIES 
                            PREFIX ""
                            SUFFIX ".pyd")
else (WIN32)
  set_target_properties(lua PROPERTIES 
                            PREFIX "")
endif (WIN32)

target_link_libraries(lua     ${LUA_LIBRARIES} ${PYTHON_LIBRARIES})
target_link_libraries(python  ${LUA_LIBRARIES} ${PYTHON_LIBRARIES})
