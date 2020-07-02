
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
	virtual void onCommand(std::string cmd, std::string &retVal) = 0;
	virtual void onUpdatePreview(std::string url) = 0;
	virtual int  onRawMessage(std::string& data) = 0;
	virtual void onRecvUploadUrl(std::string url, int refid) = 0;
	virtual void onGetLog(std::string url) = 0;
	virtual time_t GetCloudTime() = 0;
	virtual void onSetByEventMode(bool bMode) = 0;
	virtual void onSetImageParams(image_params_t*) = 0;
	virtual void onGetImageParams(image_params_t*) = 0;
	virtual void onSetMotionParams(motion_params_t* mpr) = 0;
	virtual void onGetMotionParams(motion_params_t* mpr) = 0;
	virtual void onSetTimeZone(const char* str) = 0;
	virtual void onGetTimeZone(char* str) = 0;
	virtual void onGetPTZParams(ptz_caps_t* ptz) = 0;
	virtual void onSendPTZCommand(ptz_command_t* ptz) = 0;
	virtual int onSendCameraCommand(cam_command_t* ccmd) = 0;
	virtual void onGetOSDParams(osd_settings_t* osd) = 0;
	virtual void onSetOSDParams(osd_settings_t* osd) = 0;
	virtual void onGetAudioParams(audio_settings_t* audset) = 0;
	virtual void onSetAudioParams(audio_settings_t* audset) = 0;
	virtual void onSetLogEnable(bool bEnable) = 0;
	virtual void onSetActivity(bool bEnable) = 0;
	virtual void onTriggerEvent(string evt, string meta) = 0;
	virtual void onStartBackward(string url) = 0;
	virtual void onStopBackward(string url) = 0;
	virtual void onSetPeriodicEvents(const char* name, int period, bool active) = 0;
	virtual void onGetEventLimits(time_t* pre, time_t* post) = 0;
	virtual void onSetEventLimits(time_t pre, time_t post) = 0;
	virtual void onGetWiFiList(wifi_list_t* wifilist) = 0;
	virtual void onSetCurrenWiFi(wifi_params* params) = 0;
};

#endif //__ICAMERAMANAGERWEBSOCKETCALLBACK_H__