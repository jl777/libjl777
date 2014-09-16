#./clean
rm libjl777.a
cd gzip
gcc -fPIC -c *.c
cd ../libtom
gcc -fPIC -c *.c
cd ../picoc
make
cd ..
