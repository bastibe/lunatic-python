python = require 'python'

assert(nil    == python.eval "None")
assert(true   == python.eval "True")
assert(false  == python.eval "False")

assert(1   == python.eval "1")
assert(1.5 == python.eval "1.5")

pyglob = python.globals()
d = {}
pyglob.d = d
-- This crashes if you've compiled the python.so to include another
-- Lua interpreter, i.e., with -llua.
python.execute "d['key'] = 'value'"
