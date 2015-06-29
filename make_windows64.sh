echo ">>>>>>>>>>>>>>>>>>Building mxe. This may take a while."
cd mxe
make pkgconf MXE_TARGETS='x86_64-w64-mingw32.static'
make binutils MXE_TARGETS='x86_64-w64-mingw32.static'
make gcc-gmp MXE_TARGETS='x86_64-w64-mingw32.static'
make gcc-isl MXE_TARGETS='x86_64-w64-mingw32.static'
make gcc-cloog MXE_TARGETS='x86_64-w64-mingw32.static'
make gcc-mpfr MXE_TARGETS='x86_64-w64-mingw32.static'
make gcc-mpc MXE_TARGETS='x86_64-w64-mingw32.static'
make mingw-w64 MXE_TARGETS='x86_64-w64-mingw32.static'
make gcc MXE_TARGETS='x86_64-w64-mingw32.static'
make gzip2 MXE_TARGETS='x86_64-w64-mingw32.static'
make libiconv MXE_TARGETS='x86_64-w64-mingw32.static' 
make gettext MXE_TARGETS='x86_64-w64-mingw32.static'
make pcre MXE_TARGETS='x86_64-w64-mingw32.static'
make zlib MXE_TARGETS='x86_64-w64-mingw32.static'
make dbus MXE_TARGETS='x86_64-w64-mingw32.static'
make glib MXE_TARGETS='x86_64-w64-mingw32.static'
make lzo MXE_TARGETS='x86_64-w64-mingw32.static'
make icu4c MXE_TARGETS='x86_64-w64-mingw32.static'
make apr  MXE_TARGETS='x86_64-w64-mingw32.static'
make apr-util  MXE_TARGETS='x86_64-w64-mingw32.static'
make curl MXE_TARGETS='x86_64-w64-mingw32.static'
make pthreads MXE_TARGETS='x86_64-w64-mingw32.static'
make libwebsockets MXE_TARGETS='x86_64-w64-mingw32.static'
export PATH=$PWD/usr/bin:$PATH
cp ./usr/x86_64-w64-mingw32.static/include/winioctl.h ./usr/x86_64-w64-mingw32.static/include/WinIoCtl.h
cp ./usr/x86_64-w64-mingw32.static/include/windows.h ./usr/x86_64-w64-mingw32.static/include/Windows.h
cd ..
cd nanomsg
echo ">>>>>>>>>>>>>>>>>>building nanomsg"
sh ./autogen.sh
CC=x86_64-w64-mingw32.static-gcc CXX=x86_64-w64-mingw32.static-g++ ./configure --disable-replication --enable-cxx --host x86_64-w64-mingw32.static
make
cp .libs/libnanomsg.a ../libs/libnanomsg-x64-win.a
cd ../
rm mxe/usr/x86_64-w64-mingw32.static/include/objidl.h
cp mxe/objidl.h mxe/usr/x86_64-w64-mingw32.static/include/objidl.h
cd miniupnpc
sh make_win64.sh
echo ">>>>>>>>>>>>>>>>>>building mman-win32"
cd ../mman-win32
make clean
./configure --cc=x86_64-w64-mingw32.static-gcc --enable-static
make
cp libmman.a ../libs/libmman.a
cd ..
echo ">>>>>>>>>>>>>>>>>>finished with make winpatch"

