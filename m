#./clean
rm libjl777.a
cd gzip
gcc -O3 -fPIC -c *.c
cd ../libtom
gcc -O3 -fPIC -c *.c
cd ../picoc
make
cd ..
./make_shared
