#!/bin/bash

# Setup repository
cd ${HOME}
if [ ! -d btcd/libjl777 ]; then
    echo "BTCD doesn't seem to be present in your home directory, downloading it from GitHub..."
    git clone https://github.com/jl777/btcd
fi
cd btcd/libjl777

# Build software
echo "Building application from sources, compilation takes a few minutes, please wait..."
make onetime
echo "Start make SuperNET" && make SuperNET
echo "Start make btcd" && make btcd

# Setup software
echo "All compilation tasks finished, configuring application..."
./BitcoinDarkd
sleep 5
./BitcoinDarkd dumpwallet wallet.txt
./BitcoinDarkd stop
pkill SuperNET
sleep 5
[ `ps aux | grep -i BitcoinDarkd | grep -v grep | wc -l` -ne 0 ] && pkill BitcoinDarkd

# Setup configuration file
rpcpassword=`grep rpcpassword ${HOME}/.BitcoinDark/BitcoinDark.conf | cut -d"=" -f2`
if [ ! -z "${rpcpassword}" ]; then
    if [ -f "wallet.txt" ]; then
        if [ ! -f "SuperNET.conf" ]; then
            adr1=`cat wallet.txt | grep 'addr'| cut -d" " -f5 | cut -d"=" -f2 | head -1 | tail -1`
            adr2=`cat wallet.txt | grep 'addr'| cut -d" " -f5 | cut -d"=" -f2 | head -2 | tail -1`
            adr3=`cat wallet.txt | grep 'addr'| cut -d" " -f5 | cut -d"=" -f2 | head -3 | tail -1`
            adr4=`cat wallet.txt | grep 'addr'| cut -d" " -f5 | cut -d"=" -f2 | head -4 | tail -1`
            adr5=`cat wallet.txt | grep 'addr'| cut -d" " -f5 | cut -d"=" -f2 | head -5 | tail -1`
            cp SuperNET.conf.default SuperNET.conf
            sed -i "s/BTCDADDRESS-1/${adr1}/g" SuperNET.conf
            sed -i "s/BTCDADDRESS-2/${adr2}/g" SuperNET.conf
            sed -i "s/BTCDADDRESS-3/${adr3}/g" SuperNET.conf
            sed -i "s/BTCDADDRESS-4/${adr4}/g" SuperNET.conf
            sed -i "s/BTCDADDRESS-5/${adr5}/g" SuperNET.conf
            sed -i "s/HOME-FOLDER/${HOME}/g" SuperNET.conf
        else
            echo "Error: Configuration file SuperNET.conf already exists and this script is designed to install a completely new SuperNET node"
            exit 3
        fi
    else
        echo "Error: Missing BitcoinDark addresses in wallet.txt from execution of ./BitcoinDarkd dumpwallet wallet.txt"
        exit 5
    fi
else
    echo "Error: Missing rpcpassword in ${HOME}/.BitcoinDark/BitcoinDark.conf"
    exit 7
fi