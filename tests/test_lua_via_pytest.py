import re
import __main__

import pytest

import lua

# TODO: Remove this skip marker and fix the segmentation fault issues.
skip_segfault = pytest.mark.skip(
    reason="Segmentation fault in LuaJIT when accessing table elements"
)


class MyClass:
    def __repr__(self):
        return "<MyClass>"


obj = MyClass()
__main__.obj = obj


def _assert_repr(value, pattern):
    assert re.fullmatch(pattern, repr(value))


def test_globals_reference():
    lg = lua.globals()
    assert lg == lg._G
    assert lg._G == lg._G
    assert lg._G == lg["_G"]


def test_assign_python_values():
    lg = lua.globals()

    lg.foo = "bar"
    assert lg.foo == "bar"

    lg.tmp = []
    assert lg.tmp == []


def test_lua_module_and_string():
    lg = lua.globals()

    assert lua.require.__name__ == "require"

    _assert_repr(lg.string, r"<Lua table at 0x[0-9a-fA-F]+>")
    _assert_repr(lg.string.lower, r"<Lua function at 0x[0-9a-fA-F]+>")
    assert lg.string.lower("Hello world!") == "hello world!"


@skip_segfault
def test_lua_table_access():
    lg = lua.globals()

    # TODO: Fatal Python error: Segmentation fault
    lua.execute("x = {1, 2, 3, foo = {4, 5}}")
    assert (lg.x[1], lg.x[2], lg.x[3]) == (1, 2, 3)
    assert (lg.x["foo"][1], lg.x["foo"][2]) == (4, 5)


@skip_segfault
def test_python_dict_round_trip():
    lg = lua.globals()

    d = {}
    lg.d = d
    lua.execute("d['key'] = 'value'")  # TODO: Fatal Python error: Segmentation fault
    assert d["key"] == "value"

    d2 = lua.eval("d")
    assert d is d2


@skip_segfault
def test_python_interface_access():
    __main__.lua = lua
    # TODO: Fatal Python error: Segmentation fault
    lua.execute("python = require 'python'")

    _assert_repr(lua.eval("python"), r"<Lua table at 0x[0-9a-fA-F]+>")
    assert lua.eval("python.eval 'obj'") is obj
    assert lua.eval("""python.eval([[lua.eval('python.eval(\"obj\")')]])""") is obj

    lua.execute("pg = python.globals()")
    assert lua.eval("pg.obj") is obj


@skip_segfault
def test_asfunc_and_table_iteration():
    observed = []

    def show(key, value):
        observed.append((key, value))

    asfunc = lua.eval("python.asfunc")  # TODO: Fatal Python error: Segmentation fault
    _assert_repr(asfunc, r"<Lua function at 0x[0-9a-fA-F]+>")

    lst = ["a", "b", "c"]
    t = lua.eval("{a=1, b=2, c=3}")

    for k in lst:
        show(k, t[k])

    assert observed == [("a", 1), ("b", 2), ("c", 3)]
