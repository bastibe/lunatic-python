Details
===

(This repo is a fork of https://github.com/bastibe/lunatic-python)

---

This is a fork of Lunatic Python, which can be found on the 'net at http://labix.org/lunatic-python.

Sadly, Lunatic Python is very much outdated and won't work with either a current Python or Lua.

This is an updated version of lunatic-python that works with Python 2.7-3.x and Lua 5.1-5.3.
I tried contacting the original author of Lunatic Python, but got no response.

Installing
---

```
git clone https://github.com/91Act/lunatic-python
cd lunatic-python
cmake .
make
```


Usage
---

Copy `lua.so` to one of the python `sys.path` then:

```python
import lua

lua.execute(r'''
    print('hello world!')
''')
```

