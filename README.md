libjl777
========

Build instructions for ubuntu

clang, libcurl, libnacl and libuv are required.

sudo apt-get install clang

sudo apt-get install libcurl4-gnutls-dev

sudo apt-get install libnacl-dev

git clone https://github.com/joyent/libuv

cd libuv; sh autogen.sh; ./configure; make; make check; sudo make install

To build libnacl.a and randombytes.o:

wget http://hyperelliptic.org/nacl/nacl-20090405.tar.bz2

bzip2 -dc < nacl-20090405.tar.bz2 | tar -xf -

cd nacl-20090405

./do 

This will make it in the build directory
 

Now all the required libraries should be in the system
there is a shell script "m"
./m

That should build libjl777.a
copy it to your project directory

make sure to add the following to the linker line: libjl777.a libuv.a libnacl.a -lcurl -lm -ldl 

libuv.a is from /usr/local/lib


You might have to update boost:
sudo apt-get remove boost*
sudo add-apt-repository ppa:boost-latest/ppa
sudo apt-get update
aptitude search boost
sudo apt-get install libboost1.55-dev
