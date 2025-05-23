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
#ifndef LUAINPYTHON_H
#define LUAINPYTHON_H

#if LUA_VERSION_NUM == 501
  #define luaL_len lua_objlen
  #define luaL_setfuncs(L, l, nup) luaL_register(L, NULL, (l))
  #ifdef luaL_newlib // defined in LuaJIT
    #undef luaL_newlib
  #endif
  #define luaL_newlib(L, l) (lua_newtable(L), luaL_register(L, NULL, (l)))
#endif

typedef struct
{
    PyObject_HEAD
    int ref;
    int refiter;
} LuaObject;

extern PyTypeObject LuaObject_Type;

#define LuaObject_Check(op) PyObject_TypeCheck(op, &LuaObject_Type)

PyObject* LuaConvert(lua_State *L, int n);

extern lua_State *LuaState;

#if PY_MAJOR_VERSION < 3
#  define PyInit_lua initlua
#endif
PyMODINIT_FUNC PyInit_lua(void);

#endif
