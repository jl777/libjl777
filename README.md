libjl777
========

Build instructions for ubuntu

clang, libcurl must be in the system:

sudo apt-get install clang

sudo apt-get install libcurl4-gnutls-dev

cd libjl777
make onetime
make
make btcd
make install


You might have to update boost:
sudo apt-get remove boost*
sudo add-apt-repository ppa:boost-latest/ppa
sudo apt-get update
aptitude search boost
sudo apt-get install libboost1.55-dev
