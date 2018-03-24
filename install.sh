#!/bin/bash

if [ ! -d ~/3rdparty/ipc ]; then mkdir -p ~/3rdparty/ipc; fi
if [ ! -d ~/3rdparty/include ]; then mkdir -p ~/3rdparty/include; fi
if [ ! -d ~/3rdparty/lib ]; then mkdir -p ~/3rdparty/lib; fi

if [ ! -d ~/ipc_driver ]
then
	git clone https://github.com/ZJUZQ/ipcam.git
fi

cd ~/ipc_driver
if [ -d build ]; then rm -r build; fi
mkdir build
cd build

cmake -DCMAKE_INSTALL_PREFIX=${HOME}/3rdparty CMAKE_UILD_TYPE=Release ..
make -j 6
make install

echo "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:${HOME}/3rdparty/lib" >> ~/.bashrc
echo "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:${HOME}/3rdparty/ipc/lib" >> ~/.bashrc
echo "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:${HOME}/3rdparty/ipc/lib/dh_lib" >> ~/.bashrc
echo "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:${HOME}/3rdparty/ipc/lib/hc_lib" >> ~/.bashrc
echo "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:${HOME}/3rdparty/ipc/lib/hc_lib/HCNetSDKCom" >> ~/.bashrc

source ~/.bashrc

