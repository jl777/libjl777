#./clean
rm libjl777.so
cd gzip
gcc -fPIC -mcmodel=large -c *.c
cd ../libtom
gcc -fPIC -mcmodel=large -c *.c
cd ../picoc
make
cd ..
