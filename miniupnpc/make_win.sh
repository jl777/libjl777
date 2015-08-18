export PATH=$PWD/../mxe/usr/bin:$PATH
export PATH=$PWD:$PATH
echo $PATH
echo ">>>>>>>>>>>>>>>>>>building miniupnpc.a"
make clean
make -f Makefile.mingw DLLWRAP=i686-w64-mingw32.static-dllwrap CC=i686-w64-mingw32.static-gcc AR=i686-w64-mingw32.static-ar init upnpc-static
mv libminiupnpc.a ../libs/libminiupnpc-win32.a
echo '>>>>>>>>>>>>>>>>>>finished'

