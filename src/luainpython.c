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

/* need this to build with Lua 5.2: enables lua_strlen() macro */
#define LUA_COMPAT_ALL

#include <lua.h>
#include <luaconf.h>
#include <lauxlib.h>
#include <lualib.h>

#include "pythoninlua.h"
#include "luainpython.h"

lua_State *LuaState = NULL;

static PyObject *LuaObject_New(int n);

PyObject *LuaConvert(lua_State *L, int n)
{
    
    PyObject *ret = NULL;

    switch (lua_type(L, n)) {

        case LUA_TNIL:
            Py_INCREF(Py_None);
            ret = Py_None;
            break;

        case LUA_TSTRING: {
            const char *s = lua_tostring(L, n);
            int len = lua_strlen(L, n);
            ret = PyString_FromStringAndSize(s, len);
            break;
        }

        case LUA_TNUMBER: {
            lua_Number num = lua_tonumber(L, n);
            if (num != (long)num) {
                ret = PyFloat_FromDouble(
                    (lua_Number)lua_tonumber(L, n));
            } else {
                ret = PyInt_FromLong((long)num);
            }
            break;
        }

        case LUA_TBOOLEAN:
            if (lua_toboolean(L, n)) {
                Py_INCREF(Py_True);
                ret = Py_True;
            } else {
                Py_INCREF(Py_False);
                ret = Py_False;
            }
            break;

        case LUA_TUSERDATA: {
            py_object *obj = (py_object*)
                     luaL_checkudata(L, n, POBJECT);

            if (obj) {
                Py_INCREF(obj->o);
                ret = obj->o;
                break;
            }

            /* Otherwise go on and handle as custom. */
        }

        default:
            ret = LuaObject_New(n);
            break;
    }

    return ret;
}

static PyObject *LuaCall(lua_State *L, PyObject *args)
{
    PyObject *ret = NULL;
    PyObject *arg;
    int nargs, rc, i;

    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError, "tuple expected");
        lua_settop(L, 0);
        return NULL;
    }

    nargs = PyTuple_Size(args);
    for (i = 0; i != nargs; i++) {
        arg = PyTuple_GetItem(args, i);
        if (arg == NULL) {
            PyErr_Format(PyExc_TypeError,
                     "failed to get tuple item #%d", i);
            lua_settop(L, 0);
            return NULL;
        }
        rc = py_convert(L, arg, 0);
        if (!rc) {
            PyErr_Format(PyExc_TypeError,
                     "failed to convert argument #%d", i);
            lua_settop(L, 0);
            return NULL;
        }
    }

    if (lua_pcall(L, nargs, LUA_MULTRET, 0) != 0) {
        PyErr_Format(PyExc_Exception,
                 "error: %s", lua_tostring(L, -1));
        return NULL;
    }

    nargs = lua_gettop(L);
    if (nargs == 1) {
        ret = LuaConvert(L, 1);
        if (!ret) {
            PyErr_SetString(PyExc_TypeError,
                        "failed to convert return");
            lua_settop(L, 0);
            Py_DECREF(ret);
            return NULL;
        }
    } else if (nargs > 1) {
        ret = PyTuple_New(nargs);
        if (!ret) {
            PyErr_SetString(PyExc_RuntimeError,
                    "failed to create return tuple");
            lua_settop(L, 0);
            return NULL;
        }
        for (i = 0; i != nargs; i++) {
            arg = LuaConvert(L, i+1);
            if (!arg) {
                PyErr_Format(PyExc_TypeError,
                         "failed to convert return #%d", i);
                lua_settop(L, 0);
                Py_DECREF(ret);
                return NULL;
            }
            PyTuple_SetItem(ret, i, arg);
        }
    } else {
        Py_INCREF(Py_None);
        ret = Py_None;
    }
    
    lua_settop(L, 0);

    return ret;
}

static PyObject *LuaObject_New(int n)
{
    LuaObject *obj = PyObject_New(LuaObject, &LuaObject_Type);
    if (obj) {
        lua_pushvalue(LuaState, n);
        obj->ref = luaL_ref(LuaState, LUA_REGISTRYINDEX);
        obj->refiter = 0;
    }
    return (PyObject*) obj;
}

static void LuaObject_dealloc(LuaObject *self)
{
    luaL_unref(LuaState, LUA_REGISTRYINDEX, self->ref);
    if (self->refiter)
        luaL_unref(LuaState, LUA_REGISTRYINDEX, self->refiter);
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *LuaObject_getattr(PyObject *obj, PyObject *attr)
{
    lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ((LuaObject*)obj)->ref);
    if (lua_isnil(LuaState, -1)) {
        lua_pop(LuaState, 1);
        PyErr_SetString(PyExc_RuntimeError, "lost reference");
        return NULL;
    }
    
    if (!lua_isstring(LuaState, -1)
        && !lua_istable(LuaState, -1)
        && !lua_isuserdata(LuaState, -1))
    {
        lua_pop(LuaState, 1);
        PyErr_SetString(PyExc_RuntimeError, "not an indexable value");
        return NULL;
    }

    PyObject *ret = NULL;
    int rc = py_convert(LuaState, attr, 0);
    if (rc) {
        lua_gettable(LuaState, -2);
        ret = LuaConvert(LuaState, -1);
    } else {
        PyErr_SetString(PyExc_ValueError, "can't convert attr/key");
    }
    lua_settop(LuaState, 0);
    return ret;
}

static int LuaObject_setattr(PyObject *obj, PyObject *attr, PyObject *value)
{
    int ret = -1;
    int rc;
    lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ((LuaObject*)obj)->ref);
    if (lua_isnil(LuaState, -1)) {
        lua_pop(LuaState, 1);
        PyErr_SetString(PyExc_RuntimeError, "lost reference");
        return -1;
    }
    if (!lua_istable(LuaState, -1)) {
        lua_pop(LuaState, -1);
        PyErr_SetString(PyExc_TypeError, "Lua object is not a table");
        return -1;
    }
    rc = py_convert(LuaState, attr, 0);
    if (rc) {
        if (NULL == value) {
            lua_pushnil(LuaState);
            rc = 1;
        } else {
            rc = py_convert(LuaState, value, 0);
        }

        if (rc) {
            lua_settable(LuaState, -3);
            ret = 0;
        } else {
            PyErr_SetString(PyExc_ValueError,
                    "can't convert value");
        }
    } else {
        PyErr_SetString(PyExc_ValueError, "can't convert key/attr");
    }
    lua_settop(LuaState, 0);
    return ret;
}

static PyObject *LuaObject_str(PyObject *obj)
{
    PyObject *ret = NULL;
    const char *s;
    lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ((LuaObject*)obj)->ref);
    if (luaL_callmeta(LuaState, -1, "__tostring")) {
        s = lua_tostring(LuaState, -1);
        lua_pop(LuaState, 1);
        if (s) ret = PyString_FromString(s);
    }
    if (!ret) {
        int type = lua_type(LuaState, -1);
        switch (type) {
            case LUA_TTABLE:
            case LUA_TFUNCTION:
                ret = PyString_FromFormat("<Lua %s at %p>",
                    lua_typename(LuaState, type),
                    lua_topointer(LuaState, -1));
                break;
            
            case LUA_TUSERDATA:
            case LUA_TLIGHTUSERDATA:
                ret = PyString_FromFormat("<Lua %s at %p>",
                    lua_typename(LuaState, type),
                    lua_touserdata(LuaState, -1));
                break;

            case LUA_TTHREAD:
                ret = PyString_FromFormat("<Lua %s at %p>",
                    lua_typename(LuaState, type),
                    (void*)lua_tothread(LuaState, -1));
                break;

            default:
                ret = PyString_FromFormat("<Lua %s>",
                    lua_typename(LuaState, type));
                break;

        }
    }
    lua_pop(LuaState, 1);
    return ret;
}

static PyObject *LuaObject_call(PyObject *obj, PyObject *args)
{
    lua_settop(LuaState, 0);
    lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ((LuaObject*)obj)->ref);
    return LuaCall(LuaState, args);
}

static PyObject *LuaObject_iternext(LuaObject *obj)
{
    PyObject *ret = NULL;

    lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ((LuaObject*)obj)->ref);

    if (obj->refiter == 0)
        lua_pushnil(LuaState);
    else
        lua_rawgeti(LuaState, LUA_REGISTRYINDEX, obj->refiter);

    if (lua_next(LuaState, -2) != 0) {
        /* Remove value. */
        lua_pop(LuaState, 1);
        ret = LuaConvert(LuaState, -1);
        /* Save key for next iteration. */
        if (!obj->refiter)
            obj->refiter = luaL_ref(LuaState, LUA_REGISTRYINDEX);
        else
            lua_rawseti(LuaState, LUA_REGISTRYINDEX, obj->refiter);
    } else if (obj->refiter) {
        luaL_unref(LuaState, LUA_REGISTRYINDEX, obj->refiter);
        obj->refiter = 0;
    }

    return ret;
}

static int LuaObject_length(LuaObject *obj)
{
    int len;
    lua_rawgeti(LuaState, LUA_REGISTRYINDEX, ((LuaObject*)obj)->ref);
    len = lua_objlen(LuaState, -1);
    lua_settop(LuaState, 0);
    return len;
}

static PyObject *LuaObject_subscript(PyObject *obj, PyObject *key)
{
    return LuaObject_getattr(obj, key);
}

static int LuaObject_ass_subscript(PyObject *obj,
                   PyObject *key, PyObject *value)
{
    return LuaObject_setattr(obj, key, value);
}

static PyMappingMethods LuaObject_as_mapping = {
#if PY_VERSION_HEX >= 0x02050000
    (lenfunc)LuaObject_length,    /*mp_length*/
#else
    (inquiry)LuaObject_length,    /*mp_length*/
#endif
    (binaryfunc)LuaObject_subscript,/*mp_subscript*/
    (objobjargproc)LuaObject_ass_subscript,/*mp_ass_subscript*/
};

PyTypeObject LuaObject_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                        /*ob_size*/
    "lua.custom",             /*tp_name*/
    sizeof(LuaObject),        /*tp_basicsize*/
    0,                        /*tp_itemsize*/
    (destructor)LuaObject_dealloc, /*tp_dealloc*/
    0,                        /*tp_print*/
    0,                        /*tp_getattr*/
    0,                        /*tp_setattr*/
    0,                        /*tp_compare*/
    LuaObject_str,            /*tp_repr*/
    0,                        /*tp_as_number*/
    0,                        /*tp_as_sequence*/
    &LuaObject_as_mapping,    /*tp_as_mapping*/
    0,                        /*tp_hash*/
    (ternaryfunc)LuaObject_call,     /*tp_call*/
    LuaObject_str,            /*tp_str*/
    LuaObject_getattr,       /*tp_getattro*/
    LuaObject_setattr,        /*tp_setattro*/
    0,                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "custom lua object",      /*tp_doc*/
    0,                        /*tp_traverse*/
    0,                        /*tp_clear*/
    0,                        /*tp_richcompare*/
    0,                        /*tp_weaklistoffset*/
    PyObject_SelfIter,        /*tp_iter*/
    (iternextfunc)LuaObject_iternext, /*tp_iternext*/
    0,                        /*tp_methods*/
    0,                        /*tp_members*/
    0,                        /*tp_getset*/
    0,                        /*tp_base*/
    0,                        /*tp_dict*/
    0,                        /*tp_descr_get*/
    0,                        /*tp_descr_set*/
    0,                        /*tp_dictoffset*/
    0,                        /*tp_init*/
    PyType_GenericAlloc,      /*tp_alloc*/
    PyType_GenericNew,        /*tp_new*/
    _PyObject_Del,            /*tp_free*/
    0,                        /*tp_is_gc*/
};


PyObject *Lua_run(PyObject *args, int eval)
{
    PyObject *ret;
    char *buf = NULL;
    char *s;
    int len;

    if (!PyArg_ParseTuple(args, "s#", &s, &len))
        return NULL;

    if (eval) {
        buf = (char *) malloc(strlen("return ")+len+1);
        strcpy(buf, "return ");
        strncat(buf, s, len);
        s = buf;
        len = strlen("return ")+len;
    }

    if (luaL_loadbuffer(LuaState, s, len, "<python>") != 0) {
        PyErr_Format(PyExc_RuntimeError,
                 "error loading code: %s",
                 lua_tostring(LuaState, -1));
        return NULL;
    }

    free(buf);
    
    if (lua_pcall(LuaState, 0, 1, 0) != 0) {
        PyErr_Format(PyExc_RuntimeError,
                 "error executing code: %s",
                 lua_tostring(LuaState, -1));
        return NULL;
    }

    ret = LuaConvert(LuaState, -1);
    lua_settop(LuaState, 0);
    return ret;
}

PyObject *Lua_execute(PyObject *self, PyObject *args)
{
    return Lua_run(args, 0);
}

PyObject *Lua_eval(PyObject *self, PyObject *args)
{
    return Lua_run(args, 1);
}

PyObject *Lua_globals(PyObject *self, PyObject *args)
{
    PyObject *ret = NULL;
    lua_getglobal(LuaState, "_G");
    if (lua_isnil(LuaState, -1)) {
        PyErr_SetString(PyExc_RuntimeError,
                "lost globals reference");
        lua_pop(LuaState, 1);
        return NULL;
    }
    ret = LuaConvert(LuaState, -1);
    if (!ret)
        PyErr_Format(PyExc_TypeError,
                 "failed to convert globals table");
    lua_settop(LuaState, 0);
    return ret;
}

static PyObject *Lua_require(PyObject *self, PyObject *args)
{
    lua_getglobal(LuaState, "require");
    if (lua_isnil(LuaState, -1)) {
        lua_pop(LuaState, 1);
        PyErr_SetString(PyExc_RuntimeError, "require is not defined");
        return NULL;
    }
    return LuaCall(LuaState, args);
}

static PyMethodDef lua_methods[] = {
    {"execute",    Lua_execute,    METH_VARARGS,        NULL},
    {"eval",       Lua_eval,       METH_VARARGS,        NULL},
    {"globals",    Lua_globals,    METH_NOARGS,         NULL},
    {"require",    Lua_require,    METH_VARARGS,        NULL},
    {NULL,         NULL}
};

PyMODINIT_FUNC
initlua(void)
{
    PyObject *m;

    if (PyType_Ready(&LuaObject_Type) < 0)
        return;

    m = Py_InitModule3("lua", lua_methods,
                      "Lunatic-Python Python-Lua bridge");

    if (m == NULL)
        return;

    Py_INCREF(&LuaObject_Type);

    if (!LuaState) {
        LuaState = luaL_newstate();
        luaL_openlibs(LuaState);
        luaopen_python(LuaState);
        lua_settop(LuaState, 0);
    }
}
