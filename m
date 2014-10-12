#./clean
rm libjl777.a
cd gzip
gcc -Ofast -fPIC -c *.c
cd ../libtom
gcc -Ofast -fPIC -c *.c
cd ../picoc
make
cd ..
./make_shared
