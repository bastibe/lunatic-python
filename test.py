print "----- import lua -----"
import lua
print "----- lg = lua.globals() -----"
lg = lua.globals()
print "----- lg.foo = \"bar\" -----"
lg.foo = 'bar'
print "----- lg.tmp = [] -----"
lg.tmp = []
print "----- print lg.tmp -----"
print lg.tmp
print "----- lua.execute(\"xxx = {1,2,3,foo={4,5}}\") -----"
lua.execute("xxx = {1,2,3,foo={4,5}}")
print "----- print lg.xxx[1] -----"
print lg.xxx[1]
print "----- print lg.xxx[2] -----"
print lg.xxx[2]
print "----- print lg.xxx[3] -----"
print lg.xxx[3]
print "----- print lg.xxx['foo'][1] -----"
print lg.xxx['foo'][1]
