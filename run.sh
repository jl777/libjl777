#!/bin/sh
while(true)
do
  [ `ps aux | grep -i BitcoinDarkd | grep -v grep | wc -l` -eq 0 ] && ./BitcoinDarkd
  sleep 10
done
