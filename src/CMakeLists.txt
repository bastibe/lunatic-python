if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_definitions(-DLUA_BUILD_AS_DLL)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_definitions(-DLUA_LIB)

add_library(src OBJECT luainpython.c pythoninlua.c)
