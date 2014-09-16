libjl777
========

Build instructions for ubuntu

clang, libcurl must be in the system:

sudo apt-get install clang

sudo apt-get install libcurl4-gnutls-dev

libuv and nacl sources are included and onetime build steps as follows:
./onetime

Now all the required libraries should be in the system

there is a shell script "m" to make, make sure to copy randombytes.o to the libs dir

./m

That should build libjl777.a
copy it to your project directory along with libnacl.a and libuv.a which is in libuv/.libs

make sure to add the following to the linker line: libjl777.a libuv.a libnacl.a -lcurl -lm -ldl 


You might have to update boost:
sudo apt-get remove boost*
sudo add-apt-repository ppa:boost-latest/ppa
sudo apt-get update
aptitude search boost
sudo apt-get install libboost1.55-dev
