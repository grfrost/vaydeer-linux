# vaydeer-linux

Enable Vaydeer 4 and 9 keypads to be used on Linux

You will need cmake and libudev-dev
```bash
sudo apt install cmake libudev-dev
```
To build 
```bash
clone this repo 
cd vaydeer-linux
mkdir build 
cmake -B build
cmake --build build
```

Make sure your keypad/board is plugged in 
``` 
sudo ./build/vaydeer
```

Whilst his process is running you should be able to use your keypad

Just ctrl c to kill it    
