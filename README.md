# Embedded SDK for VXG Cloud and VXG Server 

Learn more about <a href="https://www.videoexpertsgroup.com">Cloud Video Surveillance</a>

The Cloud.SDK.Embedded is a simple C/C++ reference code for integration of IP cameras, NVRs and other video devices with VXG Cloud and VXG Server. 
<br>
<br>
<br>
Documentation :
https://dashboard.videoexpertsgroup.com/docs/#CameraRefCode/
<br>
<br>
## Build
### Install cmake.
- Ubuntu, RPi  
 $ sudo apt-get install cmake  
 - Windows  
 https://cmake.org/install/  
 ### Prepare external libs (only for Windows)   
 $ cd external_libs  
  //prepare jansson library  
  $ cd jansson-2.11            
  $ mkdir build                  
  $ cd build                       
  $ cmake ..                         
  //prepare libwebsockets library      
  $ cd libwebsockets/src                 
  $ mkdir build                            
  $ cd build                                 
  $ cmake ..                                   
### Make
 - Ubuntu  
 $ cd build.linux   
 $ ./build.sh   
 - RPi  
 $ sudo apt-get install libcap-dev 
 $ cd build.rpi  
 $ ./build.sh   
 - Windows   
 Run Visual Studio, Open solution build.win\CloudSDK.cpp.sln   
### Install ffmpeg (optional)  
 - Ubuntu, RPi  
 sudo apt-get install ffmpeg  
 - Windows  
 https://www.ffmpeg.org/download.html  
 - Check ffmpeg works:  
 $ ffmpeg -version  

## Run
 - Create channel/camera on cloud (make this step once for each camera):
1. Go to Cloud Admin page on VXG Dashboard - https://dashboard.videoexpertsgroup.com/?streaming=  
2. Add new channel => Mobile Camera=> "Enter camera name"  
3. Copy "Access Token" from STREAMING tab.  
4. Click Finish button.  
 - Run application. Go to folder where test app has built:
 (Ubuntu, RPi) ./test_cloudstreamer.exe "IP address of camera" "Access Token"  
 (Windows) test_cloudstreamer.exe "IP address of camera" "Access Token"  
 
