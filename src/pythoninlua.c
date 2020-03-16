/*

 Lunatic Python
 --------------

 Copyright (c) 2002-2005  Gustavo Niemeyer <gustavo@niemeyer.net>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#include <Python.h>
#if defined(__linux__)
#   include <dlfcn.h>
#endif

/* need this to build with Lua 5.2: defines luaL_register() macro */
#define LUA_COMPAT_MODULE

#include <lua.h>
#include <lauxlib.h>

#include "pythoninlua.h"
#include "luainpython.h"

static int py_asfunc_call(lua_State *);
static int py_eval(lua_State *);

static int py_convert_custom(lua_State *L, PyObject *o, int asindx)
{
    py_object *obj = (py_object*) lua_newuserdata(L, sizeof(py_object));
    if (!obj)
        luaL_error(L, "failed to allocate userdata object");

    Py_INCREF(o);
    obj->o = o;
    obj->asindx = asindx;
    luaL_getmetatable(L, POBJECT);
    lua_setmetatable(L, -2);

    return 1;
}

int py_convert(lua_State *L, PyObject *o)
{
    int ret = 0;
    if (o == Py_None)
    {
        /* Not really needed, but this way we may check
         * for errors with ret == 0. */
        lua_pushnil(L);
        ret = 1;
    } else if (o == Py_True) {
        lua_pushboolean(L, 1);
        ret = 1;
    } else if (o == Py_False) {
        lua_pushboolean(L, 0);
        ret = 1;
    } else if (PyUnicode_Check(o) || PyBytes_Check(o)) {
        PyObject *bstr = PyUnicode_AsEncodedString(o, "utf-8", NULL);
        Py_ssize_t len;
        char *s;

        PyErr_Clear();
        PyBytes_AsStringAndSize(bstr ? bstr : o, &s, &len);
        lua_pushlstring(L, s, len);
        if (bstr) Py_DECREF(bstr);
        ret = 1;
#if PY_MAJOR_VERSION < 3
    } else if (PyInt_Check(o)) {
        lua_pushnumber(L, (lua_Number)PyInt_AsLong(o));
        ret = 1;
#endif
    } else if (PyLong_Check(o)) {
        lua_pushnumber(L, (lua_Number)PyLong_AsLong(o));
        ret = 1;
    } else if (PyFloat_Check(o)) {
        lua_pushnumber(L, (lua_Number)PyFloat_AsDouble(o));
        ret = 1;
    } else if (LuaObject_Check(o)) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ((LuaObject*)o)->ref);
        ret = 1;
    } else {
        int asindx = PyDict_Check(o) || PyList_Check(o) || PyTuple_Check(o);
        ret = py_convert_custom(L, o, asindx);
    }
    return ret;
}

static int py_object_call(lua_State *L)
{
    PyObject *args;
    PyObject *value;
    PyObject *pKywdArgs = NULL;
    int nargs = lua_gettop(L)-1;
    int ret = 0;
    int i;
    py_object *obj = (py_object*) luaL_checkudata(L, 1, POBJECT);
    assert(obj);

    if (!PyCallable_Check(obj->o))
        return luaL_error(L, "object is not callable");

    // passing a single table forces named keyword call style, e.g. plt.plot{x, y, c='red'}
    if (nargs==1 && lua_istable(L, 2)) {
        lua_pushnil(L);  /* first key */
        while (lua_next(L, 2) != 0) {
            if (lua_isnumber(L, -2)) {
                int i = lua_tointeger(L, -2);
                nargs = i < nargs ? nargs : i;
            }
            lua_pop(L, 1);
        }
        args = PyTuple_New(nargs);
        if (!args) {
            PyErr_Print();
            return luaL_error(L, "failed to create arguments tuple");
        }
        pKywdArgs = PyDict_New();    
        if (!pKywdArgs) {
            Py_DECREF(args);
            PyErr_Print();
            return luaL_error(L, "failed to create arguments dictionary");
        }
        lua_pushnil(L);  /* first key */
        while (lua_next(L, 2) != 0) {
            if (lua_isnumber(L, -2)) {
                PyObject *arg = LuaConvert(L, -1);
                if (!arg) {
                    Py_DECREF(args);
                    Py_DECREF(pKywdArgs);
                    return luaL_error(L, "failed to convert argument #%d", lua_tointeger(L, -2));
                }
                PyTuple_SetItem(args, lua_tointeger(L, -2)-1, arg);
            }
            else if (lua_isstring(L, -2)) {
                PyObject *arg = LuaConvert(L, -1);
                if (!arg) {
                    Py_DECREF(args);
                    Py_DECREF(pKywdArgs);
                    return luaL_error(L, "failed to convert argument '%s'", lua_tostring(L, -2));
                }
                PyDict_SetItemString(pKywdArgs, lua_tostring(L, -2), arg);
                Py_DECREF(arg);
            }
            lua_pop(L, 1);
        }
    }
    // regular call style e.g. plt.plot(x, y)
    else {
        args = PyTuple_New(nargs);
        if (!args) {
            PyErr_Print();
            return luaL_error(L, "failed to create arguments tuple");
        }
        for (i = 0; i != nargs; i++) {
            PyObject *arg = LuaConvert(L, i+2);
            if (!arg) {
                Py_DECREF(args);
                return luaL_error(L, "failed to convert argument #%d", i+1);
            }
            PyTuple_SetItem(args, i, arg);
       }
    }

    value = PyObject_Call(obj->o, args, pKywdArgs);
    Py_DECREF(args);
    if (pKywdArgs) Py_DECREF(pKywdArgs);

    if (value) {
        ret = py_convert(L, value);
        Py_DECREF(value);
    } else {
        PyErr_Print();
        luaL_error(L, "error calling python function");
    }

    return ret;
}

static int _p_object_newindex_set(lua_State *L, py_object *obj,
                  int keyn, int valuen)
{
    PyObject *value;
    PyObject *key = LuaConvert(L, keyn);

    if (!key) {
        return luaL_argerror(L, 1, "failed to convert key");
    }

    if (!lua_isnil(L, valuen)) {
        value = LuaConvert(L, valuen);
        if (!value) {
            Py_DECREF(key);
            return luaL_argerror(L, 1, "failed to convert value");
        }

        if (PyObject_SetItem(obj->o, key, value) == -1) {
            PyErr_Print();
            luaL_error(L, "failed to set item");
        }

        Py_DECREF(value);
    } else {
        if (PyObject_DelItem(obj->o, key) == -1) {
            PyErr_Print();
            luaL_error(L, "failed to delete item");
        }
    }

    Py_DECREF(key);
    return 0;
}

static int py_object_newindex_set(lua_State *L)
{
    py_object *obj = (py_object*) luaL_checkudata(L, lua_upvalueindex(1), POBJECT);
    if (lua_gettop(L) != 2)
        return luaL_error(L, "invalid arguments");

    return _p_object_newindex_set(L, obj, 1, 2);
}

static int py_object_newindex(lua_State *L)
{
    PyObject *value;
    const char *attr;
    py_object *obj = (py_object*) luaL_checkudata(L, 1, POBJECT);
    assert(obj);

    if (obj->asindx || lua_type(L, 2) != LUA_TSTRING)
        return _p_object_newindex_set(L, obj, 2, 3);

    attr = luaL_checkstring(L, 2);
    assert(attr);

    value = LuaConvert(L, 3);
    if (!value) {
        return luaL_argerror(L, 1, "failed to convert value");
    }

    if (PyObject_SetAttrString(obj->o, attr, value) == -1) {
        Py_DECREF(value);
        PyErr_Print();
        return luaL_error(L, "failed to set value");
    }

    Py_DECREF(value);
    return 0;
}

static int _p_object_index_get(lua_State *L, py_object *obj, int keyn)
{
    PyObject *key = LuaConvert(L, keyn);
    PyObject *item;
    int ret = 0;

    if (!key) {
        return luaL_argerror(L, 1, "failed to convert key");
    }

    item = PyObject_GetItem(obj->o, key);

    Py_DECREF(key);

    if (item) {
        ret = py_convert(L, item);
        Py_DECREF(item);
    } else {
        PyErr_Clear();
        if (lua_gettop(L) > keyn) {
            lua_pushvalue(L, keyn+1);
            ret = 1;
        }
    }

    return ret;
}

static int py_object_index_get(lua_State *L)
{
    py_object *obj = (py_object*) luaL_checkudata(L, lua_upvalueindex(1), POBJECT);
    int top = lua_gettop(L);
    if (top < 1 || top > 2)
        return luaL_error(L, "invalid arguments");

    return _p_object_index_get(L, obj, 1);
}

static int py_object_index(lua_State *L)
{
    int ret = 0;
    PyObject *value;
    const char *attr;
    py_object *obj = (py_object*) luaL_checkudata(L, 1, POBJECT);
    assert(obj);

    if (obj->asindx || lua_type(L, 2) != LUA_TSTRING)
        return _p_object_index_get(L, obj, 2);

    attr = luaL_checkstring(L, 2);
    assert(attr);

    if (strcmp(attr, "__get") == 0) {
        lua_pushvalue(L, 1);
        lua_pushcclosure(L, py_object_index_get, 1);
        return 1;
    } else if (strcmp(attr, "__set") == 0) {
        lua_pushvalue(L, 1);
        lua_pushcclosure(L, py_object_newindex_set, 1);
        return 1;
    }


    value = PyObject_GetAttrString(obj->o, attr);
    if (value) {
        ret = py_convert(L, value);
        Py_DECREF(value);
    } else {
        PyErr_Clear();
        lua_pushnil(L);
        ret = 1;
    }

    return ret;
}

static int py_object_gc(lua_State *L)
{
    py_object *obj = (py_object*) luaL_checkudata(L, 1, POBJECT);
    assert(obj);

    Py_DECREF(obj->o);
    return 0;
}

static int py_object_tostring(lua_State *L)
{
    py_object *obj = (py_object*) luaL_checkudata(L, 1, POBJECT);
    assert(obj);

    PyObject *repr = PyObject_Str(obj->o);
    if (!repr)
    {
        char buf[256];
        snprintf(buf, 256, "python object: %p", obj->o);
        lua_pushstring(L, buf);
        PyErr_Clear();
    }
    else
    {
        py_convert(L, repr);
        assert(lua_type(L, -1) == LUA_TSTRING);
        Py_DECREF(repr);
    }
    return 1;
}

static int py_operator_lambda(lua_State *L,  const char *op)
{
  static char script_buff[] = "lambda a, b: a    b";
  static size_t len = sizeof(script_buff) / sizeof(script_buff[0]);
  snprintf(script_buff, len, "lambda a, b: a %s b", op);

  lua_pushcfunction(L, py_eval);
  lua_pushlstring(L, script_buff, len);
  lua_call(L, 1, 1);
  return luaL_ref(L, LUA_REGISTRYINDEX);
}

#define make_pyoperator(opname, opliteral) \
  static int py_object_ ## opname(lua_State *L) \
  { \
    static int op_ref = LUA_REFNIL; \
    if (op_ref == LUA_REFNIL) op_ref = py_operator_lambda(L, opliteral); \
    \
    lua_rawgeti(L, LUA_REGISTRYINDEX, op_ref); \
    lua_insert(L, 1); \
    return py_object_call(L); \
  } \
  struct opname ## __LINE__ // force semi

make_pyoperator(_pow, "**");
make_pyoperator(_mul, "*");
make_pyoperator(_div, "/");
make_pyoperator(_add, "+");
make_pyoperator(_sub, "-");

static const luaL_Reg py_object_mt[] =
{
    {"__call",  py_object_call},
    {"__index", py_object_index},
    {"__newindex",  py_object_newindex},
    {"__gc",    py_object_gc},
    {"__tostring",  py_object_tostring},
    {"__pow",   py_object__pow},
    {"__mul",   py_object__mul},
    {"__div",   py_object__div},
    {"__add",   py_object__add},
    {"__sub",   py_object__sub},
    {NULL, NULL}
};

static int py_run(lua_State *L, int eval)
{
    const char *s = luaL_checkstring(L, 1);
    PyObject *m, *d, *o;
    int ret = 0;

    lua_settop(L, 1);

    if (!eval)
    {
        lua_pushliteral(L, "\n");
        lua_concat(L, 2);

        s = luaL_checkstring(L, 1);
    }

    m = PyImport_AddModule("__main__");
    if (!m)
        return luaL_error(L, "Can't get __main__ module");

    d = PyModule_GetDict(m);

    o = PyRun_String(s, eval ? Py_eval_input : Py_file_input,
                     d, d);
    if (!o)
    {
        PyErr_Print();
        return 0;
    }

    if (py_convert(L, o))
        ret = 1;

    Py_DECREF(o);
    
#if PY_MAJOR_VERSION < 3
    if (Py_FlushLine())
#endif
        PyErr_Clear();

    return ret;
}

static int py_execute(lua_State *L)
{
    return py_run(L, 0);
}

static int py_eval(lua_State *L)
{
    return py_run(L, 1);
}

static int py_asindx(lua_State *L)
{
    py_object *obj = (py_object*) luaL_checkudata(L, 1, POBJECT);
    assert(obj);

    return py_convert_custom(L, obj->o, 1);
}

static int py_asattr(lua_State *L)
{
    py_object *obj = (py_object*) luaL_checkudata(L, 1, POBJECT);
    assert(obj);

    return py_convert_custom(L, obj->o, 0);
}

static int py_asfunc_call(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_insert(L, 1);
    return py_object_call(L);
}

static int py_asfunc(lua_State *L)
{
    py_object *obj = luaL_checkudata(L, 1, POBJECT);
    if (!PyCallable_Check(obj->o))
        return luaL_error(L, "object is not callable");

    lua_settop(L, 1);
    lua_pushcclosure(L, py_asfunc_call, 1);

    return 1;
}

static int py_globals(lua_State *L)
{
    PyObject *globals;

    if (lua_gettop(L) != 0) {
        return luaL_error(L, "invalid arguments");
    }

    globals = PyEval_GetGlobals();
    if (!globals) {
        PyObject *module = PyImport_AddModule("__main__");
        if (!module) {
            return luaL_error(L, "Can't get __main__ module");
        }
        globals = PyModule_GetDict(module);
    }

    if (!globals) {
        PyErr_Print();
        return luaL_error(L, "can't get globals");
    }

    return py_convert_custom(L, globals, 1);
}

static int py_locals(lua_State *L)
{
    PyObject *locals;

    if (lua_gettop(L) != 0) {
        return luaL_error(L, "invalid arguments");
    }

    locals = PyEval_GetLocals();
    if (!locals)
        return py_globals(L);

    return py_convert_custom(L, locals, 1);
}

static int py_builtins(lua_State *L)
{
    PyObject *builtins;

    if (lua_gettop(L) != 0) {
        return luaL_error(L, "invalid arguments");
    }

    builtins = PyEval_GetBuiltins();
    if (!builtins) {
        PyErr_Print();
        return luaL_error(L, "failed to get builtins");
    }

    return py_convert_custom(L, builtins, 1);
}

static int py_import(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    PyObject *module = PyImport_ImportModule((char*)name);
    int ret;

    if (!module)
    {
        PyErr_Print();
        return luaL_error(L, "failed importing '%s'", name);
    }

    ret = py_convert_custom(L, module, 0);
    Py_DECREF(module);
    return ret;
}

py_object* luaPy_to_pobject(lua_State *L, int n)
{
    if(!lua_getmetatable(L, n)) return NULL;
    luaL_getmetatable(L, POBJECT);
    int is_pobject = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);

    return is_pobject ? (py_object *) lua_touserdata(L, n) : NULL;
}

static const luaL_Reg py_lib[] =
{
    {"execute", py_execute},
    {"eval",    py_eval},
    {"asindx",  py_asindx},
    {"asattr",  py_asattr},
    {"asfunc",  py_asfunc},
    {"locals",  py_locals},
    {"globals", py_globals},
    {"builtins",    py_builtins},
    {"import",  py_import},
    {NULL, NULL}
};

LUA_API int luaopen_python(lua_State *L)
{
    int rc;

    /* Register module */
    luaL_newlib(L, py_lib);

    /* Register python object metatable */
    luaL_newmetatable(L, POBJECT);
    luaL_setfuncs(L, py_object_mt, 0);
    lua_pop(L, 1);

    /* Initialize Lua state in Python territory */
    if (!LuaState) LuaState = L;

    /* Initialize Python interpreter */
    if (!Py_IsInitialized())
    {
        PyObject *luam, *mainm, *maind;
#if PY_MAJOR_VERSION >= 3
        wchar_t *argv[] = {L"<lua>", 0};
#else
        char *argv[] = {"<lua>", 0};
#endif
        Py_SetProgramName(argv[0]);
        PyImport_AppendInittab("lua", PyInit_lua);

        /* Loading python library symbols so that dynamic extensions don't throw symbol not found error.           
           Ref Link: http://stackoverflow.com/questions/29880931/importerror-and-pyexc-systemerror-while-embedding-python-script-within-c-for-pam
        */
#if defined(__linux__)
#   define STR(s) #s
#if PY_MAJOR_VERSION < 3
#   define PYLIB_STR(major, minor) "libpython" STR(major) "." STR(minor) ".so"
#else
#   define PYLIB_STR(major, minor) "libpython" STR(major) "." STR(minor) "m.so"
#endif
        dlopen(PYLIB_STR(PY_MAJOR_VERSION, PY_MINOR_VERSION), RTLD_NOW | RTLD_GLOBAL);
#endif

        Py_Initialize();
        PySys_SetArgv(1, argv);
        /* Import 'lua' automatically. */
        luam = PyImport_ImportModule("lua");
        if (!luam)
          return luaL_error(L, "Can't import lua module");

        mainm = PyImport_AddModule("__main__");
        if (!mainm)
        {
          Py_DECREF(luam);
          return luaL_error(L, "Can't get __main__ module");
        }

        maind = PyModule_GetDict(mainm);
        PyDict_SetItemString(maind, "lua", luam);
        Py_DECREF(luam);
    }

    /* Register 'none' */
    rc = py_convert_custom(L, Py_None, 0);
    if (!rc)
      return luaL_error(L, "failed to convert none object");

    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "Py_None"); /* registry.Py_None */

    lua_setfield(L, -2, "none"); /* python.none */

    return 1;
}
