#!/usr/bin/python

import sys
import os
import commands

from distutils.core import setup, Extension
from distutils.sysconfig import get_python_lib, get_python_version

if os.path.isfile("MANIFEST"):
    os.unlink("MANIFEST")

# You may have to change these
LUAVERSION = "5.2"
PYLIBS = ["python" + get_python_version(), "pthread", "util"]
PYLIBDIR = [get_python_lib(standard_lib=True) + "/config"]
LUALIBS = ["lua" + LUAVERSION]
LUALIBDIR = []


def pkgconfig(*packages):
    # map pkg-config output to kwargs for distutils.core.Extension
    flag_map = {'-I': 'include_dirs', '-L': 'library_dirs', '-l': 'libraries'}

    for package in packages:
        (pcstatus, pcoutput) = commands.getstatusoutput(
            "pkg-config --libs --cflags %s" % package)
        if pcstatus == 0:
            break
    else:
        sys.exit("pkg-config failed for %s; "
                 "most recent output was:\n%s" %
                 (", ".join(packages), pcoutput))

    kwargs = {}
    for token in pcoutput.split():
        if token[:2] in flag_map:
            kwargs.setdefault(flag_map.get(token[:2]), []).append(token[2:])
        else:                           # throw others to extra_link_args
            kwargs.setdefault('extra_link_args', []).append(token)

    for k, v in kwargs.iteritems():     # remove duplicated
        kwargs[k] = list(set(v))

    return kwargs

lua_pkgconfig = pkgconfig('lua', 'lua' + LUAVERSION)

setup(name="lunatic-python",
      version="1.0",
      description="Two-way bridge between Python and Lua",
      author="Gustavo Niemeyer",
      author_email="gustavo@niemeyer.net",
      url="http://labix.org/lunatic-python",
      license="LGPL",
      long_description="""\
Lunatic Python is a two-way bridge between Python and Lua, allowing these
languages to intercommunicate. Being two-way means that it allows Lua inside
Python, Python inside Lua, Lua inside Python inside Lua, Python inside Lua
inside Python, and so on.
""",
      ext_modules=[
        Extension("lua-python",
                  ["src/pythoninlua.c", "src/luainpython.c"],
                  **lua_pkgconfig),
        Extension("lua",
                  ["src/pythoninlua.c", "src/luainpython.c"],
                  **lua_pkgconfig),
        ],
      )
