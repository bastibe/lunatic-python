python = require 'python'

assert(nil    == python.eval "None")
assert(true   == python.eval "True")
assert(false  == python.eval "False")

assert(1   == python.eval "1")
assert(1.5 == python.eval "1.5")

assert("foo" == python.eval "b'foo'")
assert("bar" == python.eval "u'bar'")

pyglob = python.globals()
d = {}
pyglob.d = d
-- This crashes if you've compiled the python.so to include another
-- Lua interpreter, i.e., with -llua.
python.execute "d['key'] = 'value'"
assert(d.key == 'value', d.key)

python.execute "d.key = 'newvalue'"
assert(d.key == 'newvalue', d.key)

-- Test operators on py containers
l = python.eval "['hello']"
assert(tostring(l * 3) == "['hello', 'hello', 'hello']")
assert(tostring(l + python.eval "['bye']") == "['hello', 'bye']")

-- Test that Python C module can access Py Runtime symbols
ctypes = python.import 'ctypes'
assert(tostring(ctypes):match "module 'ctypes'")

-- Test multiple Python statement execution
pytype = python.eval "type"
python.execute
[[
foo = 1
bar = 2
]]

assert(python.globals().foo == 1)
assert(python.globals().bar == 2)
