libjl777
========

Build instructions for ubuntu

libcurl, libnacl and libuv are required.

make sure you have curl installed so -lcurl works, should already be in the system

sudo apt-get install libnacl-dev

git clone https://github.com/joyent/libuv

cd libuv; sh autogen.sh; ./configure; make; make check

sudo make install

Now all the required libraries should be in the system
there is a shell script "m"
./m

That should build libjl777.so
copy it to /usr/lib or wherever the linker is looking for it and you can link your project against it

make sure to add -lnacl randombytes.o from the nacl project to your project


