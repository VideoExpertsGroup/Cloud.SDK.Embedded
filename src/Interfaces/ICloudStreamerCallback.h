
#ifndef __ICLOUDSTREAMERCALLBACK_H__
#define __ICLOUDSTREAMERCALLBACK_H__

#include <string>

class ICloudStreamerCallback{
public:
	virtual ~ICloudStreamerCallback() {};

	virtual void onStarted(std::string url_push) = 0; //Cloud gets ready for data, url_push == rtmp://...
	virtual void onStopped() = 0;                //Cloud closed getting the data
	virtual void onError(int error) = 0;
	virtual void onCameraConnected() = 0; // getCamera() to get the camera
	virtual void onCommand(std::string cmd) = 0;
};

#endif