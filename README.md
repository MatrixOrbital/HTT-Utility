# HTT Support utility

A quick and easy utility to change the touch screen settings on Matrix Orbitals HTT range of products.

------------------------------------------------------------------

**Hardware Requirements:**

![alt text](https://www.matrixorbital.com/image/cache/catalog/products/HTT50A-TPR_650-300x300.jpg)

**Any HTT Product with a touch screen**

https://www.matrixorbital.com/communication-protocol/hdmi


**Building**

This example uses [CMake](https://www.cmake.org) as the project system, available in most package managers on linux and available on the [CMake Download page](https://cmake.org/download/) for windows.

***Linux***

The htt utility requires the `libudev-dev` package so be sure to install that with your distributions package manager.

After downloading the code using either the green Clone or Download button on this page or cloning from the git command line. Use the following commands in the source folder to build the utility

```bash
mkdir build
cd build
cmake ..
make 
```

***Windows***

***Pre-build binaries***

Prebuild versions of the utility for windows are available on the [Matrix Orbital Support Site](https://www.matrixorbital.com/software/htt-utility).

***Building from source***

This will require both CMake and Visual Studio to be installed to build the utility.

- Make sure either Visual Studio 2019 or 2017 is installed, also make sure to enable the 'Desktop Development with C++' workload during the installtion. Without it you cannot build the htt utility. 
- Download the installer for your platform from the [CMake Download page](https://cmake.org/download/)
- Install CMake, when asked if you want to add CMake to the path, select the option `Add CMake to the system PATH for all users` 
- Download the source code from this page, by using the Green Clone or Download button in the upper right top of this page.
- If you used a .zip file to download the code, unpack the zip file.
- For simplicity the following steps assume the code was placed at `c:\htt_util` if you used a different path adjust the commands below
- Run the follwing commands from the `Development Command Prompt For VS2019` (or 2017 depending on the Visual studio version you have) a shortcut should be available in the start menu for this.
```
cd c:\htt_util\
mkdir build
cd build
cmake ..
htt_util.sln
```
- Visual Studio will now open 
- From the `Build` pulldown menu, select `Build Solution`



