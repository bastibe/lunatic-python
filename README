Details
=======

This is a fork of Lunatic Python, which can be found on the 'net at http://labix.org/lunatic-python.

Sadly, Lunatic Python is very much outdated and won't work with either a current Python or Lua.

This is an updated version of lunatic-python that works with Python 2.7 and Lua 5.1.
I tried contacting the original author of Lunatic Python, but got no response.

Installing
----------

To install, you will need to have the Python and Lua development libraries on your system. If you
do, use the recommended methods (```pip```, ```easy-install```, etc) to install lunatic-python.

On some versions of Ubuntu, installation might fail with error messages. If it does, make sure
```lualib5.1-0-dev``` and ```python-dev``` are installed, and change the two lines from:

    extra_compile_args=["-rdynamic"],

to:


    extra_compile_args=["-rdynamic", "-I/usr/include/lua5.1"],

If it complains about a missing ```lua``` or ``lualib```, change the line:

    LUALIBS = ["lua"]

to:

    LUALIBS = ["lua5.1"]

and it should compile cleanly.
