#./clean
rm libjl777.so
cd gzip
gcc -fPIC -c *.c
cd ../libtom
gcc -fPIC -c *.c
cd ../picoc
make
cd ..
