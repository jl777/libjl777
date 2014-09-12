git pull
rm libjl777.a
cd gzip
clang -c *.c
cd ../libtom
clang -c *.c
cd ../picoc
make
cd ..
cp libjl777.a ~/dark-test-v2/src

