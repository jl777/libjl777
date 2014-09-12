cd gzip
clang -c *.c
cd ../libtom
clang -c *.c
cd ../picoc
make
cd ..
clang -c libjl777.c
ar rcu libjl777.a libjl777.o libs/randombytes.o
cp libjl777.a ~/dark-test-v2/src

