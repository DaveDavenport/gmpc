#!/bin/bash

mkdir temp;
cd temp
wget "$1"
tar -zxf launchpad-export.tar.gz
rm launchpad-export.tar.gz
cd ../

for a in *.po; do
    mv temp/gmpc/gmpc-$a ./$a;
done
