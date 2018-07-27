# Embedded SDK for VXG Cloud and VXG Server 

The Cloud.SDK.Embedded is a simple C/C++ refernce code for integration of IP cameras, NVRs and other video devices with VXG Cloud and VXG Server. 
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
<br>
 - Windows  
 https://cmake.org/install/  
<br>
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
<br>
### Make
 - Ubuntu  
 $ cd build.linux   
 $ ./build.sh   
<br>
 - RPi  
 $ cd build.rpi  
 $ ./build.sh   
<br>
 - Windows   
 Run Visual Studio, Open solution build.win\CloudSDK.cpp.sln   
<br>
### Install ffmpeg (optional)  
 - Ubuntu, RPi  
 sudo apt-get install ffmpeg  
 - Windows  
 https://www.ffmpeg.org/download.html  
<br>
 - Check ffmpeg works:  
 $ ffmpeg -version  

## Run
 - Create channel/camera on cloud (make this step once for the camera)
Register the camera on cloud https://dashboard.videoexpertsgroup.com/?streaming=  
Add new channel => Mobile Camera=> <Enter camera name>  
Copy "Access Token" from STREAMING tab.  
Click Finish button.  
<br>
 - Run application. Go to folder where test app has built  
 (Ubuntu, RPi) ./test_cloudstreamer.exe <IP address of camera> <Access Token>  
 (Windows) test_cloudstreamer.exe <IP address of camera> <Access Token>  
 