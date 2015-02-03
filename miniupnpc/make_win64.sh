export PATH=$PWD/../mxe/usr/bin:$PATH
export PATH=$PWD:$PATH
echo $PATH
echo ">>>>>>>>>>>>>>>>>>building miniupnpc.a"
make clean
make -f Makefile.mingw DLLWRAP=x86_64-w64-mingw32.static-dllwrap CC=x86_64-w64-mingw32.static-gcc AR=x86_64-w64-mingw32.static-ar init upnpc-static
mv libminiupnpc.a ../libs/libminiupnpc-x64-win.a
echo '>>>>>>>>>>>>>>>>>>finished'
