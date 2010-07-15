#!/bin/sh
set -e
cd ../dl
make
cp dl.exe ../stage/dl/
cd ../stage
python stage.py
python packer.py 
cd ../wingui
rm -f packed.o
make
upx wingui.exe
cat ../misc/magic.txt >> wingui.exe
cat ../resources/freeze.tar.xz >> wingui.exe
