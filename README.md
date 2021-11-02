# Lunatic-Python

[![Build Status](https://travis-ci.org/bastibe/lunatic-python.svg?branch=master)](https://travis-ci.org/bastibe/lunatic-python)
[![License: LGP-L2.1](https://img.shields.io/badge/license-LGPL%202.1-blue.svg)](https://opensource.org/licenses/LGPL-2.1)

Details
=======

This is a fork of Lunatic Python, which can be found on the 'net at http://labix.org/lunatic-python.

Sadly, Lunatic Python is very much outdated and won't work with either a current Python or Lua.

This is an updated version of lunatic-python that works with Python 2.7-3.x and Lua 5.1-5.3.
I tried contacting the original author of Lunatic Python, but got no response.

Installing
----------

To install, you will need to have the Python and Lua development libraries on your system. If you
do, use the recommended methods (`pip`, `easy-install`, etc) to install lunatic-python.

This version has been modified to compile under Ubuntu. I haven't tested it under other
distributions, your mileage may vary.

Introduction
------------

Lunatic Python is a two-way bridge between Python and Lua, allowing these languages to intercommunicate. Being two-way means that it allows Lua inside Python, Python inside Lua, Lua inside Python inside Lua, Python inside Lua inside Python, and so on. 

Why?

Even though the project was born as an experiment, it's already being used in real world projects to integrate features from both languages. Please, let me know if you use it in real world projects. 

Examples

Lua inside Python
A basic example. 

```
>>> import lua
>>> lg = lua.globals()
>>> lg.string
<Lua table at 0x81c6a10>
>>> lg.string.lower
<Lua function at 0x81c6b30>
>>> lg.string.lower("Hello world!")
'hello world!'
```

Now, let's put a local object into Lua space. 

```
>>> d = {}
>>> lg.d = d
>>> lua.execute("d['key'] = 'value'")
>>> d
{'key': 'value'}
Can we get the reference back from Lua space? 
>>> d2 = lua.eval("d")
>>> d is d2
True
```

Good! 

Is the python interface available inside the Lua interpreter? 

```
>>> lua.eval("python")
<Lua table at 0x81c7540>
```

Yes, it looks so. Let's nest some evaluations and see a local reference passing through. 

```
>>> class MyClass: pass
... 
>>> obj = MyClass()
>>> obj
<__main__.MyClass instance at 0x403ccb4c>
>>> lua.eval(r"python.eval('lua.eval(\"python.eval(\'obj\')\")')")
<__main__.MyClass instance at 0x403ccb4c>
```

Are you still following me?  Good. Then you've probably noticed that the Python interpreter state inside the Lua interpreter state is the same as the outside Python we're running. Let's see that in a more comfortable way. 

```
>>> lua.execute("pg = python.globals()")
>>> lua.eval("pg.obj")
<__main__.MyClass instance at 0x403ccb4c>
```

Things get more interesting when we start to really mix Lua and Python code. 

```
>>> table = lua.eval("table")
>>> def show(key, value):
...   print "key is %s and value is %s" % (`key`, `value`)
... 
>>> t = lua.eval("{a=1, b=2, c=3}")
>>> table.foreach(t, show)
key is 'a' and value is 1
key is 'c' and value is 3
key is 'b' and value is 2
>>>
```

Of course, in this case the same could be achieved easily with Python. 

```
>>> def show(key, value):
...   print "key is %s and value is %s" % (`key`, `value`)
... 
>>> t = lua.eval("{a=1, b=2, c=3}")
>>> for k in t:
...   show(k, t[k])
... 
key is 'a' and value is 1
key is 'c' and value is 3
key is 'b' and value is 2
```

Python inside Lua
-----------------

Now, let's have a look from another perspective. The basic idea is exactly the same. 

```
> require("python")
> python.execute("import string")
> pg = python.globals()
> =pg.string
<module 'string' from '/usr/lib/python2.3/string.pyc'>
> =pg.string.lower("Hello world!")
hello world!
```

As Lua is mainly an embedding language, getting access to the batteries included in Python may be interesting. 

```
> re = python.import("re")
> pattern = re.compile("^Hel(lo) world!")
> match = pattern.match("Hello world!")
> =match.group(1)
lo
```

Just like in the Python example, let's put a local object in Python space. 

```
> d = {}
> pg.d = d
> python.execute("d['key'] = 'value'")
> table.foreach(d, print)
key     value
```

Again, let's grab back the reference from Python space. 

```
> d2 = python.eval("d")
> print(d == d2)
true
```

Is the Lua interface available to Python? 

```
> =python.eval("lua")
<module 'lua' (built-in)>
```

Good. So let's do the nested trick in Lua as well. 

```
> t = {}
> =t
table: 0x80fbdb8
> =python.eval("lua.eval('python.eval(\"lua.eval(\\'t\\')\")')")
table: 0x80fbdb8
> 
```

It means that the Lua interpreter state inside the Python interpreter is the same as the outside Lua interpreter state. Let's show that in a more obvious way. 

```
> python.execute("lg = lua.globals()")
> =python.eval("lg.t")
table: 0x80fbdb8
```

Now for the mixing example. 

```
> function notthree(num)
>>   return (num ~= 3)
>> end
> l = python.eval("[1, 2, 3, 4, 5]")
> filter = python.eval("filter")
> =filter(notthree, l)
[1, 2, 4, 5]
```

Documentation
=============

Theory
------

The bridging mechanism consists of creating the missing interpreter state inside the host interpreter. That is, when you run the bridging system inside Python, a Lua interpreter is created; when you run the system inside Lua, a Python interpreter is created. 
Once both interpreter states are available, these interpreters are provided with the necessary tools to interact freely with each other. The given tools offer not only the ability of executing statements inside the alien interpreter, but also to acquire individual objects and interact with them inside the native state. This magic is done by two special object types, which act bridging native object access to the alien interpreter state.

Almost every object which is passed between Python and Lua is encapsulated in the language specific bridging object type. The only types which are not encapsulated are strings and numbers, which are converted to the native equivalent objects. 
Besides that, the Lua side also has special treatment for encapsulated Python functions and methods. The most obvious way to implement calling of Python objects inside the Lua interpreter is to implement a `__call` function in the bridging object metatable. Unfortunately this mechanism is not supported in certain situations, since some places test if the object type is a function, which is not the case of the bridging object. To overwhelm these problems, Python functions and methods are automatically converted to native Lua function closures, becoming accessible in every Lua context. Callable object instances which are not functions nor methods, on the other hand, will still use the metatable mechanism. Luckily, they may also be converted in a native function closure using the `asfunc()` function, if necessary.

Attribute vs. Subscript object access
-------------------------------------

Accessing an attribute or using the subscript operator in Lua give access to the same information. This behavior is reflected in the Python special object that encapsulates Lua objects, allowing Lua tables to be accessed in a more comfortable way, and also giving access to objects which use protected Python keywords (such as the print function). For example: 

```
>>> string = lua.eval("string")
>>> string.lower
<Lua function at 0x81c6bf8>
>>> string["lower"]
<Lua function at 0x81c6bf8>
```

Using Python from the Lua side requires a little bit more attention, since Python has a more strict syntax than Lua. The later makes no distinction between attribute and subscript access, so we need some way to know what kind of access is desired at a given moment. This control is provided using two functions: `asindx()` and `asattr()`. These functions receive a single Python object as parameter, and return the same object with the given access discipline. Notice that dictionaries and lists use the index discipline by default, while other objects use the attribute discipline. For example: 

```
> dict = python.eval("{}")
> =dict.keys
nil
> dict.keys = 10
> print(dict["keys"])
10
> =dict
{'keys': 10}
> =dict.keys = 10
n.asattr(dict)
> =dict.keys
function: 0x80fa9b8
> =dict.keys()
['keys']
```

Lua inside Python
-----------------

When executing Python as the host language, the Lua functionality is accessed by importing the lua module. When Lua is the host language, the lua module will already be available in the global Python scope. 
Below is a description of the functions available in the lua module. 

`lua.execute(statement)`

This function will execute the given statement inside the Lua interpreter state. 
Examples: 

```
>>> lua.execute("foo = 'bar'")
lua.eval(expression)
```

This function will evaluate the given expression inside the Lua interpreter state, and return the result. It may be used to acquire any object from the Lua interpreter state. 
Examples:

```
>>> lua.eval("'foo'..2")
'foo2'
>>> lua.eval('string')
<Lua table at 0x81c6ae8>
>>> string = lua.eval('string')
>>> string.lower("Hello world!")
'hello world!'
```

`lua.globals()`

Return the Lua global scope from the interpreter state. 

Examples: 

```
>>> lg = lua.globals()
>>> lg.string.lower("Hello world!")
'hello world!'
>>> lg["string"].lower("Hello world!")
'hello world!'
>>> lg["print"]
<Lua function at 0x8154350>
>>> lg["print"]("Hello world!")
Hello world!
```

`lua.require(name)`

Executes the require() Lua function, importing the given module. 

Examples: 
```
>>> lua.require("testmod")
True
>>> lua.execute("func()")
I'm func in testmod!
```

Python inside Lua
-----------------

Unlike Python, Lua has no default path to its modules. Thus, the default path of the real Lua module of Lunatic Python is together with the Python module, and a python.lua stub is provided. This stub must be placed in a path accessible by the Lua require() mechanism, and once imported it will locate the real module and load it. 

Unfortunately, there's a minor inconvenience for our purposes regarding the Lua system which imports external shared objects. The hardcoded behavior of the loadlib() function is to load shared objects without exporting their symbols. This is usually not a problem in the Lua world, but we're going a little beyond their usual requirements here. We're loading the Python interpreter as a shared object, and the Python interpreter may load its own external modules which are compiled as shared objects as well, and these will want to link back to the symbols in the Python interpreter. Luckily, fixing this problem is easier than explaining the problem. It's just a matter of replacing the flag RTLD_NOW in the loadlib.c file of the Lua distribution by the or'ed version RTLD_NOW|RTLD_GLOBAL. This will avoid "undefined symbol" errors which could eventually happen. 
Below is a description of the functions available in the python module. 

`python.execute(statement)`

This function will execute the given statement inside the Python interpreter state. 
Examples: 

```
> python.execute("foo = 'bar'")
```

`python.eval(expression)`

This function will evaluate the given expression inside the Python interpreter state, and return the result. It may be used to acquire any object from the Python interpreter state. 

Examples: 

```
> python.execute("import string")
> =python.eval("string")
<module 'string' from '/usr/lib/python2.3/string.pyc'>
> string = python.eval("string")
> =string.lower("Hello world!")
hello world!
```

`python.globals()`

Return the Python global scope dictionary from the interpreter state. 
Examples: 

```
> python.execute("import string")
> pg = python.globals()
> =pg.string.lower("Hello world!")
hello world!
> =pg["string"].lower("Hello world!")
hello world!
```

`python.locals()`

Return the Python local scope dictionary from the interpreter state. 
Examples: 

```
> function luafunc()
>>   print(python.globals().var)
>>   print(python.locals().var)
>> end
> python.execute("def func():\n var = 'value'\n lua.execute('luafunc()')")
> python.execute("func()")
nil
value
```

`python.builtins()`

Return the Python builtins module dictionary from the interpreter state. 
Examples: 

```
> pb = python.builtins()
> =pb.len("Hello world!")
12
```

`python.import(name)`

Imports and returns the given Python module. 
Examples: 

```
> os = python.import("os")
> =os.getcwd()
/home/niemeyer/src/lunatic-python
```

`python.asattr(pyobj)`

Return a copy of the given Python object with an attribute access discipline. 
Examples: 

```
> dict = python.eval("{}")
> =dict.keys
nil
> dict.keys = 10
> print(dict["keys"])
10
> =dict
{'keys': 10}
> =dict.keys = 10
n.asattr(dict)
> =dict.keys
function: 0x80fa9b8
> =dict.keys()
['keys']
```

`python.asindx(pyobj)`

Return a copy of the given Python object with an index access discipline. 
Examples: 

```
> buffer = python.eval("buffer('foobar')")
> =buffer[0]
stdin:1: unknown attribute in python object
stack traceback:
        [C]: ?
        stdin:1: in main chunk
        [C]: ?
> buffer = python.asindx(buffer)
> =buffer[0]
f
```

`python.asfunc(pyobj)`

Return a copy of the given Python object enclosed in a Lua function closure. This is useful to use Python callable instances in places that require a Lua function. Python methods and functions are automatically converted to Lua functions, and don't require to be explicitly converted. 
Examples: 

```
> python.execute("class Join:\n def __call__(self, *args):\n  return '-'.join(args)")
> join = python.eval("Join()")
> =join
<__main__.Join instance at 0x403a864c>
> =join('foo', 'bar')
foo-bar
> =table.foreach({foo='bar'}, join)
stdin:1: bad argument #2 to `foreach' (function expected, got userdata)
stack traceback:
        [C]: in function `foreach'
        stdin:1: in main chunk
        [C]: ?
> =table.foreach({foo='bar'}, python.asfunc(join))
foo-bar
```

License
-------

Lunatic Python is available under the LGPL license. 

Download
--------
Available files: 
•	lunatic-python-1.0.tar.bz2 
Author
Gustavo Niemeyer <gustavo@niemeyer.net> 
