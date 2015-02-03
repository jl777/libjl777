echo ">>>>>>>>>>>>>>>>>>Building mxe. This may take a while."
sudo apt-get install autopoint bison bzip2 cmake flex gcc g++ gperf intltool libtool ruby scons wine zlib1g-dev libffi-dev
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
cd ../libuv
make clean
sh autogen.sh
./configure --host x86_64-w64-mingw32.static --disable-shared
echo ">>>>>>>>>>>>>>>>>>building libuv"  
make
cp .libs/libuv.a ../libs/libuv-x64-win.a
cd ..
mkdir db_win
unzip db-6.1.19.zip -d db_64_win
cd db_64_win/db-6.1.19
echo ">>>>>>>>>>>>>>>>>>building libdb"
mkdir build_mxe
cd build_mxe
CC=x86_64-w64-mingw32.static-gcc CXX=x86_64-w64-mingw32.static-g++ ../dist/configure --enable-mingw --disable-replication --enable-cxx --host x86_64-w64-mingw32.static
make
cp libdb.a ../../../libs/libdb-x64-win.a
cp db.h ../../../
cd ../../..
rm mxe/usr/x86_64-w64-mingw32.static/include/objidl.h
cp mxe/objidl.h mxe/usr/x86_64-w64-mingw32.static/include/objidl.h
cd miniupnpc
sh make_win64.sh
echo ">>>>>>>>>>>>>>>>>>building mman-win32"
cd ../mman-win32
make clean
./configure --cc=x86_64-w64-mingw32.static-gcc --enable-static
make
cp libmman.a ../libs/libmman-x64-win.a
cd ..
echo ">>>>>>>>>>>>>>>>>>finished with make winpatch"
