#!/usr/bin/python
from distutils.core import setup, Extension
from distutils.sysconfig import get_python_lib, get_python_version
import commands
import os

if os.path.isfile("MANIFEST"):
    os.unlink("MANIFEST")

# You may have to change these
PYLIBS = ["python"+get_python_version(), "pthread", "util"]
PYLIBDIR = [get_python_lib(standard_lib=True)+"/config"]
LUALIBS = ["lua5.1"]
LUALIBDIR = []

def pkgconfig(*packages, **kwargs):
    flag_map = {'-I': 'include_dirs', '-L': 'library_dirs', '-l': 'libraries'}

    for token in commands.getoutput("pkg-config --libs --cflags %s" % ' '.join(packages)).split():
        if flag_map.has_key(token[:2]):
            kwargs.setdefault(flag_map.get(token[:2]), []).append(token[2:])
        else: # throw others to extra_link_args
            kwargs.setdefault('extra_link_args', []).append(token)

    for k, v in kwargs.iteritems(): # remove duplicated
        kwargs[k] = list(set(v))

    return kwargs

setup(name="lunatic-python",
      version = "1.0",
      description = "Two-way bridge between Python and Lua",
      author = "Gustavo Niemeyer",
      author_email = "gustavo@niemeyer.net",
      url = "http://labix.org/lunatic-python",
      license = "LGPL",
      long_description =
"""\
Lunatic Python is a two-way bridge between Python and Lua, allowing these
languages to intercommunicate. Being two-way means that it allows Lua inside
Python, Python inside Lua, Lua inside Python inside Lua, Python inside Lua
inside Python, and so on.
""",
      ext_modules = [
                     Extension("lua-python",
                               ["src/pythoninlua.c", "src/luainpython.c"],
                               **pkgconfig('lua')
                     ),
                     Extension("lua",
                               ["src/pythoninlua.c", "src/luainpython.c"],
                               **pkgconfig('lua')
                     )
                    ],
      )
