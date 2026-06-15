package = 'lunatic-python'
version = 'scm-0'
source = {
	url = 'git+https://github.com/bastibe/lunatic-python'
}
description = {
	summary = 'Two-way bridge between Python and Lua',
	homepage = 'https://github.com/bastibe/lunatic-python',
	license = 'LGPL-2.1'
}
build = {
	type = 'make',
	makefile = 'Makefile.luarocks',
	variables = {
		LIBDIR = '$(LIBDIR)',
		LUA_INCDIR = '$(LUA_INCDIR)',
		LUA_LIBDIR = '$(LUA_LIBDIR)'
	}
}
