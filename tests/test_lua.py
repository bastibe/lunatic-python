"""
>>> lg = lua.globals()
>>> lg == lg._G
True
>>> lg._G == lg._G
True
>>> lg._G == lg['_G']
True

>>> lg.foo = 'bar'
>>> lg.foo == u'bar'
True

>>> lg.tmp = []
>>> lg.tmp
[]

>>> lua.execute("x = {1, 2, 3, foo = {4, 5}}")
>>> lg.x[1], lg.x[2], lg.x[3]
(1..., 2..., 3...)
>>> lg.x['foo'][1], lg.x['foo'][2]
(4..., 5...)

>>> lua.require
<built-in function require>

>>> lg.string
<Lua table at 0x...>
>>> lg.string.lower
<Lua function at 0x...>
>>> lg.string.lower("Hello world!") == u'hello world!'
True

>>> d = {}
>>> lg.d = d
>>> lua.execute("d['key'] = 'value'")
>>> d
{...'key': ...'value'}

>>> d2 = lua.eval("d")
>>> d is d2
True

>>> lua.execute("python = require 'python'")
>>> lua.eval("python")
<Lua table at 0x...>

>>> obj
<MyClass>

>>> lua.eval("python.eval 'obj'")
<MyClass>

>>> lua.eval(\"\"\"python.eval([[lua.eval('python.eval("obj")')]])\"\"\")
<MyClass>

>>> lua.execute("pg = python.globals()")
>>> lua.eval("pg.obj")
<MyClass>

>>> def show(key, value):
...   print("key is %s and value is %s" % (repr(key), repr(value)))
... 
>>> asfunc = lua.eval("python.asfunc")
>>> asfunc
<Lua function at 0x...>

>>> l = ['a', 'b', 'c']
>>> t = lua.eval("{a=1, b=2, c=3}")
>>> for k in l:
...   show(k, t[k])
key is 'a' and value is 1...
key is 'b' and value is 2...
key is 'c' and value is 3...

"""

import sys, os
sys.path.append(os.getcwd())


class MyClass:
    def __repr__(self): return '<MyClass>'

obj = MyClass()


if __name__ == '__main__':
    import lua
    import doctest
    doctest.testmod(optionflags=doctest.ELLIPSIS)
