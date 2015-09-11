echo ">>>>>>>>>>>>>>>>>>Building mxe. This may take a while."
cp -n mxepatch/nanomsg.mk mxe/src/nanomsg.mk
cp -n mxepatch/nanomsg-1.patch mxe/src/nanomsg-1.patch
cp -n mxepatch/nanomsg-2.patch mxe/src/nanomsg-2.patch
cp -n mxepatch/nanomsg-3-global.patch mxe/src/nanomsg-3-global.patch
cp -n mxepatch/nanomsg-4-options.patch mxe/src/nanomsg-4-options.patch
cp -n mxepatch/picocoin.mk mxe/src/picocoin.mk
cp -n mxepatch/picocoin-1.patch mxe/src/picocoin-1.patch
cp -n mxepatch/picocoin-2-endian.patch mxe/src/picocoin-2-endian.patch
cd mxe
patch -N -s --reject-file=- < ../mxepatch/mxe.patch
cd src
patch -N -s --reject-file=- < ../../mxepatch/curl.mk.patch
cd ..
make pkgconf MXE_TARGETS='i686-w64-mingw32.static'
make binutils MXE_TARGETS='i686-w64-mingw32.static'
make gcc-gmp MXE_TARGETS='i686-w64-mingw32.static'
make gcc-isl MXE_TARGETS='i686-w64-mingw32.static'
#make gcc-cloog MXE_TARGETS='i686-w64-mingw32.static'
make gcc-mpfr MXE_TARGETS='i686-w64-mingw32.static'
make gcc-mpc MXE_TARGETS='i686-w64-mingw32.static'
make mingw-w64 MXE_TARGETS='i686-w64-mingw32.static'
make gcc MXE_TARGETS='i686-w64-mingw32.static'
#make gzip2 MXE_TARGETS='i686-w64-mingw32.static'
make libiconv MXE_TARGETS='i686-w64-mingw32.static' 
make gettext MXE_TARGETS='i686-w64-mingw32.static'
make pcre MXE_TARGETS='i686-w64-mingw32.static'
make zlib MXE_TARGETS='i686-w64-mingw32.static'
make dbus MXE_TARGETS='i686-w64-mingw32.static'
make glib MXE_TARGETS='i686-w64-mingw32.static'
make lzo MXE_TARGETS='i686-w64-mingw32.static'
make icu4c MXE_TARGETS='i686-w64-mingw32.static'
make apr  MXE_TARGETS='i686-w64-mingw32.static'
make apr-util  MXE_TARGETS='i686-w64-mingw32.static'
make curl MXE_TARGETS='i686-w64-mingw32.static'
make pthreads MXE_TARGETS='i686-w64-mingw32.static'
make libwebsockets MXE_TARGETS='i686-w64-mingw32.static'
make boost MXE_TARGETS='i686-w64-mingw32.static'
make db MXE_TARGETS='i686-w64-mingw32.static'
make nanomsg MXE_TARGETS='i686-w64-mingw32.static'
make mman-win32 MXE_TARGETS='i686-w64-mingw32.static'
make picocoin MXE_TARGETS='i686-w64-mingw32.static'
export PATH=$PWD/usr/bin:$PATH
cp ./usr/i686-w64-mingw32.static/include/winioctl.h ./usr/i686-w64-mingw32.static/include/WinIoCtl.h
cp ./usr/i686-w64-mingw32.static/include/windows.h ./usr/i686-w64-mingw32.static/include/Windows.h
cd ..
#cd nanomsg
#echo ">>>>>>>>>>>>>>>>>>building nanomsg"
#make clean
#sh ./autogen.sh
#CC=i686-w64-mingw32.static-gcc CXX=i686-w64-mingw32.static-g++ ./configure --disable-replication --enable-cxx --host i686-w64-mingw32.static
#make CFLAGS='-g -O2 -w -DNN_HAVE_WINDOWS -DNN_HAVE_MINGW -D_WIN32_WINNT=0x0600'
#cp .libs/libnanomsg.a ../libs/libnanomsg-win32.a
#cd ../
rm mxe/usr/i686-w64-mingw32.static/include/objidl.h
cp mxepatch/winheaders/objidl.h mxe/usr/i686-w64-mingw32.static/include/objidl.h
cd miniupnpc
sh make_win.sh
#echo ">>>>>>>>>>>>>>>>>>building mman-win32"
#cd ../mman-win32
#make clean
#./configure --cc=i686-w64-mingw32.static-gcc --enable-static
#make
#cp libmman.a ../libs/libmman.a
#cd ..
echo ">>>>>>>>>>>>>>>>>>finished with make winpatch"

