cd gzip
clang -c *.c
cd ../libtom
clang -c *.c
cd ../picoc
make
cd ..
clang -c libjl777.c
ar rcu libjl777.a libjl777.o libothers.a libs/libwebsockets.a libs/libnacl.a libs/libuv.a libs/randombytes.o 

