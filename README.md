# HTT Support Utility

A quick and easy utility to change settings on Matrix Orbitals HTT HDMI TFT LCD range of products.

We are happy to release our HTT HDMI TFT Support Utility source code for Windows and Linux!

Windows compiled version:

https://www.matrixorbital.com/software/htt-utilities

----------------------------------------------------------------

 --help
 
    Show help (this message)
    
 --scan
 
    Scan for HTT modules and display their settings

    Returns: Device, Firmware Rev, Driver Type, Screen Rotation, Default Backlight, Touch feedback, Backlight Face, Backlight dimming

 --device [id]
 
    Selects the target for the following commands in setups where multiple HTTs
    are connected. (0 = default)

 --loadcalibration [filename]
 
    Load the calibration data from a file, only available on resistive
    touch screens

 --rotatetouch [degrees]
 
    Sets and saves the rotation for the touch panel (visual output will not change orientation.) 
    Normally the host OS should take care of screen rotation, if the host OS does not support this, 
    this options offers the option to apply the rotation on the device degrees can be [0, 90, 180, 270]

 --savecalibration [filename]
 
    Save the calibration data to a file, only available on resistive touch
    screens

 --sensitivity [level]
 
    Sets the sensitivity of the touch panel
    Attempting set this option to anything besides 'normal' on any other product
    will lead to undefined behavior and is not recommended.

    level for MXT can be [normal,high,extra]
    level for GT9xx can be [normal,high]

**These commands require PCB 1.5+ or 3.0+ of any HTT display**

 --backlight [setting]
 
    set backlight brightness [0-255]
    
 --backlightset [setting]
 
    set and save backlight brightness [0-255]

 --backlightfade [time in ms]
 
    set and save the response time to a backlight brightness change

 --haptic [duration]
 
    set duration for haptic feedback (in 100ms increments)
    for 1 second - use 10

 --piezo [duration]
 
    set duration for piezo tone at 400Hz (in 100ms increments)
    for 1 second - use 10

 --alarm [type] [duration] [flash]
 
    The alarm will continue until either one of the folowwing conditions occurs:
    - The duration of the alarm is reached
    - The screen is touched (touch capable models only)
    - The alarm is canceled by selecting alarm type 0
    
    type: 0-17 alarm type [0 = off], 16 preprogrammed tones
    
    duration: duration for the alarm (in 100ms increments, for 1 second - use 10), use -1 for no timeout

    flash: flashes per second, max = 10, off = 0

 --touchfeedback [setting]
 
    Setting: 0 none, 1 haptic, 2 piezo, 3 haptic and piezo

 --touchdim [time1] [brightness1] [time2] [brightness2] [time3] [brightness3] [time4] [brightness4]
    
    dim the display after [time] seconds of inactivity up to 4 levels.
    
    [time1]       [0-600] time in seconds since last touch
    [brightness1] [0-255] brightness of the display 0 = Off, 255 is full brightness
    [time2]       [0-600] time in seconds since [time1]
    [brightness2] [0-255] brightness of the display 0 = Off, 255 is full brightness
    [time3]       [0-600] time in seconds since [time3]
    [brightness3] [0-255] brightness of the display 0 = Off, 255 is full brightness
    [time4]       [0-600] time in seconds since [time4]
    [brightness4] [0-255] brightness of the display 0 = Off, 255 is full brightness
    
   
   --touchdim 5 150 30 75 60 25 0 0 
   
    example 3 stages, in 5 seconds brightness will be 150, 30 seconds after that 75, 60 seconds after 25
   
   --touchdim 0 0 0 0 0 0 0 0
   
    disable feature
   
   Note: while time is specified in seconds, for convenience time can be postfixed with the letter 'm' for specify minutes ie 5m would automatically convert to 300 seconds. 
    
 --capcalibrate
 
    Capacitive touch calibrate

 --factorydefaults
 
    reset the unit to factory defaults
   
------------------------------------------------------------------

**Hardware Requirements:**

![alt text](https://www.matrixorbital.com/image/cache/catalog/products/HTT50A-TPR_650-300x300.jpg)

**Any HTT HDMI TFT Product with a touch screen**

https://www.matrixorbital.com/communication-protocol/hdmi


**Building**

This example uses [CMake](https://www.cmake.org) as the project system, available in most package managers on linux and available on the [CMake Download page](https://cmake.org/download/) for windows.

***Linux***

The HTT Utility requires the `libudev-dev` package so be sure to install that with your distributions package manager.

After downloading the code using either the green Clone or Download button on this page or cloning from the git command line. Use the following commands in the source folder to build the utility

```bash
mkdir build
cd build
cmake ..
make 
```

***Windows***

***Pre-build binaries***

Prebuild version of the utility for Windows is available on the [Matrix Orbital Support Site](https://www.matrixorbital.com/software/htt-utilities).

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

** SUPPORT **

**Support Forums:**  https://www.lcdforums.com/forums/viewforum.php?f=46



