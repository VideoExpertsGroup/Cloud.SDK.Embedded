
#ifndef __ICAMERAMANAGERWEBSOCKETCALLBACK_H__
#define __ICAMERAMANAGERWEBSOCKETCALLBACK_H__

#include "CameraManagerConfig.h"

class ICameraManagerCallback
{
public:
	virtual void onPrepared() = 0;
	virtual void onClosed(int error, std::string reason) = 0;
	virtual void onUpdateConfig(CameraManagerConfig &config) = 0;
	virtual void onStreamStart() = 0;
	virtual void onStreamStop() = 0;
	virtual void onCommand(std::string cmd) = 0;
	virtual void onUpdatePreview() = 0;
	virtual int  onRawMessage(std::string& data) = 0;
	virtual void onRecvUploadUrl(std::string url, int refid) = 0;
	virtual void onGetLog(std::string url) = 0;
};

#endif //__ICAMERAMANAGERWEBSOCKETCALLBACK_H__