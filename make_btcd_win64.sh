cd mxe
make qt5 MXE_TARGETS='x86_64-w64-mingw32'
make qttools MXE_TARGETS='x86_64-w64-mingw32'
cd ..
export PATH=$PWD/mxe/usr/bin:$PATH
echo ">>>>>>>>>building BitcoinDark-qt.exe"
cp mxe/winheaders/* mxe/usr/x86_64-w64-mingw32.static/include
cd ../src/leveldb
make clean
TARGET_OS=OS_WINDOWS_CROSSCOMPILE make libleveldb.a libmemenv.a CC=x86_64-w64-mingw32.static-gcc CXX=x86_64-w64-mingw32.static-g++
cd ..
rm glue.o
cd ..
x86_64-w64-mingw32.static-qmake-qt5 "USE_UPNP=-" "USE_IPV6=0" "USE_QRENCODE=0" BitcoinDark-qt.pro
make
strip release/BitcoinDark-qt.exe
cp release/BitcoinDark-qt.exe libjl777/BitcoinDark-qt.exe
echo ">>>>>>>>>finished"
