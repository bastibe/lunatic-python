add_definitions(-DLUA_BUILD_AS_DLL -DLUA_LIB)
add_library(src OBJECT luainpython.c pythoninlua.c)
