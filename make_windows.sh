echo ">>>>>>>>>>>>>>>>>>Building mxe. This may take a while."
sudo apt-get install autopoint bison bzip2 cmake flex gcc g++ gperf intltool libtool ruby scons wine zlib1g-dev libffi-dev
cd mxe
make pkgconf
make binutils 
make gcc-gmp
make gcc-isl
make gcc-cloog
make gcc-mpfr
make gcc-mpc
make mingw-w64 
make gcc
make gzip2
make libiconv 
make gettext
make pcre
make zlib
make dbus
make glib
make lzo
make icu4c
make apr 
make apr-util 
make curl
make pthreads
make libwebsockets
export PATH=$PWD/usr/bin:$PATH
cp ./usr/i686-w64-mingw32.static/include/winioctl.h ./usr/i686-w64-mingw32.static/include/WinIoCtl.h
cp ./usr/i686-w64-mingw32.static/include/windows.h ./usr/i686-w64-mingw32.static/include/Windows.h
cd ../libuv
sh autogen.sh
./configure --host i686-w64-mingw32.static --disable-shared
echo ">>>>>>>>>>>>>>>>>>building libuv"  
make
cp .libs/libuv.a ../libs/libuv-win.a
cd ..
mkdir db_win
unzip db-6.1.19.zip -d db_win
cd db_win/db-6.1.19
echo ">>>>>>>>>>>>>>>>>>building libdb"
mkdir build_mxe
cd build_mxe
CC=i686-w64-mingw32.static-gcc CXX=i686-w64-mingw32.static-g++ ../dist/configure --enable-mingw --disable-replication --enable-cxx --host i686-w64-mingw32.static
make
cp libdb.a ../../../libs/libdb-win.a
cp db.h ../../../
cd ../../..
rm mxe/usr/i686-w64-mingw32.static/include/objidl.h
cp mxe/objidl.h mxe/usr/i686-w64-mingw32.static/include/objidl.h
cd miniupnpc
sh make_win.sh
echo ">>>>>>>>>>>>>>>>>>building mman-win32"
cd ../mman-win32
./configure --cc=i686-w64-mingw32.static-gcc --enable-static
make
cp libmman.a ../libs/libmman-win.a
cd ..
echo ">>>>>>>>>>>>>>>>>>finished with make winpatch"
