
#ifndef __ICLOUDSTREAMERCALLBACK_H__
#define __ICLOUDSTREAMERCALLBACK_H__

#include <string>

#include "../cloud/core/CameraManager/CameraManagerConfig.h"

class ICloudStreamerCallback{
public:
	virtual ~ICloudStreamerCallback() {};

	virtual void onStarted(std::string url_push) = 0; //Cloud gets ready for data, url_push == rtmp://...
	virtual void onStopped() = 0;                //Cloud closed getting the data
	virtual void onError(int error) = 0;
	virtual void onCameraConnected() = 0; // getCamera() to get the camera
	virtual void onCommand(std::string cmd, std::string &retVal) = 0;
	virtual int  onRawMsg(std::string& data) = 0;
	virtual void onUploadUrl(void* inst, std::string url_push, int refid) = 0;//Got upload url for snapshot
	virtual void onCamGetLog(std::string url) = 0;
	virtual time_t onGetCloudTime() = 0;
	virtual void onSetRecByEventsMode(bool bEventsMode) = 0;
	virtual void GetImageParams(image_params_t*) = 0;
	virtual void setStreamConfig(CameraManagerConfig &config) = 0;
	virtual void SetImageParams(image_params_t*) = 0;
	virtual void GetMotionParams(motion_params_t* mpr) = 0;
	virtual void SetMotionParams(motion_params_t* mpr) = 0;
	virtual void GetTimeZone(char* time_zone_str) = 0;
	virtual void SetTimeZone(const char* time_zone_str) = 0;
	virtual void GetPTZParams(ptz_caps_t* ptz) = 0;
	virtual void SendPTZCommand(ptz_command_t* ptz) = 0;
	virtual int SendCameraCommand(cam_command_t* ccmd) = 0;
	virtual void GetOSDParams(osd_settings_t* osd) = 0;
	virtual void SetOSDParams(osd_settings_t* osd) = 0;
	virtual void GetAudioParams(audio_settings_t* audset) = 0;
	virtual void SetAudioParams(audio_settings_t* audset) = 0;
	virtual void SetLogEnable(bool bEnable) = 0;
	virtual void TriggerEvent(void* inst, string evt, string meta) = 0;
	void* parent;
};

#endif