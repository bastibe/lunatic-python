add_library(src OBJECT luainpython.c pythoninlua.c)
set_target_properties(src PROPERTIES
                          POSITION_INDEPENDENT_CODE TRUE)

target_include_directories(src PRIVATE ${LUA_INCLUDE_DIR} ${Python_INCLUDE_DIRS})

target_compile_definitions(src PRIVATE LUA_LIB)
if (WIN32)
  target_compile_definitions(src PRIVATE LUA_BUILD_AS_DLL)
endif (WIN32)

if (UNIX)
  get_filename_component(PYTHON_LIBRT ${Python_LIBRARIES} NAME)
  target_compile_definitions(src PRIVATE PYTHON_LIBRT=${PYTHON_LIBRT})
endif (UNIX)

if (CMAKE_COMPILER_IS_GNUCC)
  target_compile_options(src PUBLIC -Wall -pedantic -std=c99)
endif (CMAKE_COMPILER_IS_GNUCC)
