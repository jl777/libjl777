libjl777
========

Build instructions for ubuntu

libcurl, libnacl and libuv are required.

sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install libnacl-dev

git clone https://github.com/joyent/libuv
cd libuv; sh autogen.sh; ./configure; make; make check; sudo make install

Now all the required libraries should be in the system
there is a shell script "m"
./m

That should build libjl777.so
copy it to /usr/lib or wherever the linker is looking for it and you can link your project against it

make sure to add -ljl777 libuv.a randombytes.o -lnacl -lm -ldl to the linker line

libuv.a is from /usr/local/lib
randombytes.o is from libnacl


You might have to update boost:
sudo apt-get remove boost*
sudo add-apt-repository ppa:boost-latest/ppa
sudo apt-get update
aptitude search boost
sudo apt-get install libboost1.55-dev
