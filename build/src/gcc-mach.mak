CXX_cygwin = g++
CXX_mingw =  g++
CXX_js =     em++
CXX_wasm =   em++
CXX_linux =  g++
CXX_freebsd =g++
CXX_darwin = g++

EXEEXT_cygwin = .exe
EXEEXT_mingw =  .exe
EXEEXT_js =     .js
EXEEXT_wasm =   .js
EXEEXT_linux =
EXEEXT_freebsd =
EXEEXT_darwin =

DYNLIBPREFIX_cygwin = 
DYNLIBPREFIX_mingw =
DYNLIBPREFIX_js =
DYNLIBPREFIX_wasm =
DYNLIBPREFIX_linux =   lib
DYNLIBPREFIX_freebsd = lib
DYNLIBPREFIX_darwin =  lib

DYNLIBEXT_cygwin =  .dll
DYNLIBEXT_mingw =   .dll
DYNLIBEXT_js =      .js
DYNLIBEXT_wasm =    .js
DYNLIBEXT_linux =   .so
DYNLIBEXT_freebsd = .so
DYNLIBEXT_darwin =  .bundle

LIBEXT_cygwin = .lib
LIBEXT_mingw = .lib
LIBEXT_js =    .js
LIBEXT_wasm =  .js
LIBEXT_linux = .l
LIBEXT_freebsd = .l
LIBEXT_darwin = .l

CFLAGS_cygwin =  -DUSEKDB -DUNICODE -D_UNICODE -DWINVER=0x0500 -D_WIN32_WINNT=0x0500
CFLAGS_mingw =   -DUSEKDB -DUNICODE -D_UNICODE -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -static
CFLAGS_js =      -UWIN32 -U_WIN32 -U_Windows -U__CYGWIN__ -s ALLOW_MEMORY_GROWTH=1
CFLAGS_wasm = -s WASM=1 $(CFLAGS_js)
CFLAGS_linux =   -fPIC
CFLAGS_freebsd = -fPIC
CFLAGS_darwin =  

LDFLAGS_cygwin = -Wl,--enable-auto-import -Wl,--enable-stdcall-fixup
LDFLAGS_mingw = -Wl,--enable-auto-import -Wl,--enable-stdcall-fixup -static
EXPORTED_FUNCTIONS = "['_load','_loadu','_unload','_request','_multi_load','_multi_loadu','_multi_unload','_multi_request','_so_library_init','_so_library_cleanup','_so_getmoduleversion','_so_create','_so_createu','_so_dispose','_so_request','_so_free']"
LDFLAGS_js = -Wl,--enable-auto-import -Wl,--enable-stdcall-fixup -Os --memory-init-file 0 --pre-js em-pre.js --post-js em-post.js -s EXPORTED_FUNCTIONS=$(EXPORTED_FUNCTIONS) -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall','cwrap','intArrayFromString','intArrayToString']" -s ALLOW_MEMORY_GROWTH=1
LDFLAGS_wasm = -Wl,--enable-auto-import -Wl,--enable-stdcall-fixup -O1 --memory-init-file 0 --pre-js em-pre.js --post-js em-post.js -s EXPORTED_FUNCTIONS=$(EXPORTED_FUNCTIONS) -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall','cwrap','intArrayFromString','intArrayToString']" -s ALLOW_MEMORY_GROWTH=1 -s WASM=1
LDFLAGS_linux = 
LDFLAGS_freebsd = 
LDFLAGS_darwin = 

SHARED_cygwin = -shared
SHARED_mingw = -shared
SHARED_js = -shared
SHARED_wasm = -shared
SHARED_linux = -shared
SHARED_freebsd = -shared
SHARED_darwin = -bundle

LIBS_cygwin = -lshell32
LIBS_mingw = -lshell32
LIBS_js =
LIBS_wasm =
LIBS_linux = -ldl -lrt -lpthread
LIBS_freebsd = -liconv
LIBS_darwin = -lstdc++ -liconv
