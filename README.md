IPC Driver
===
The IP cameras' driver. Now it support DaHua and Haikang's camera

### Tips
Add the `3rdparty/hc/lib/HCNetSDKCom` into LD_LIBRARY_PATH

### Build

```
git clone --recursive http://gitlab.alibaba-inc.com/ShopAI/ipc_driver.git 
cd ipc_driver
mkdir build; cd build; 
cmake -DCMAKE_UILD_TYPE=Release ..; make -j 6
```
