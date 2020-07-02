
#ifndef __CLOUDSTREAMERSDK_H__
#define __CLOUDSTREAMERSDK_H__

#include "utils/utils.h"
#include "utils/_cunk.h"
#include "utils/MLog.h"
#include "Interfaces/ICloudStreamerCallback.h"
#include "Objects/CloudShareConnection.h"
#include "cloud/core/CameraManager/CameraManagerConfig.h"
#include "cloud/core/CameraManager/CameraManager.h"
#include "cloud/core/CameraManager/ICameraManagerCallback.h"

class CloudStreamerSDK : public CUnk,
						public ICameraManagerCallback
{
//	const char *TAG = "CloudStreamerSDK";
//	const int LOG_LEVEL = 2; //Log.VERBOSE;
	MLog Log;

	ICloudStreamerCallback *mCallback;
	long m_RefConnected;

	bool m_bStarted;

	CloudShareConnection mConnection;

	//camera info
	std::string mToken;
	long long mCamid;
	std::string mSvcpUrl;

	void* snap_handler;

	//streamer CM
	CameraManagerConfig mCameraManagerConfig;
	CameraManager mCameraManager;
	int prepareCM();
	int closeCM();

public:
	CloudStreamerSDK(ICloudStreamerCallback *callback);
	virtual ~CloudStreamerSDK();

	int setSource(std::string &channel);

	int Start();
	int Stop();

	int sendCamEvent(const CameraEvent &camEvent);
	int GetDirectUploadUrl(unsigned long timeUTC, const char* type, const char* category, int size, int duration, int width=0, int height=0);
	int CloudPing();
	time_t CloudPong();
	void* GetCallbackClassPtr();

	//=>ICameraManagerCallback
	void onPrepared();
	void onClosed(int error, std::string reason);
	void onUpdateConfig(CameraManagerConfig &config);
	void onStreamStart();
	void onStreamStop();
	void onCommand(std::string cmd, std::string& retVal);
	void onUpdatePreview(std::string url);
	int  onRawMessage(std::string& data);
	int  ConfirmUpload(std::string url);
	void onRecvUploadUrl(std::string url, int refid);
	void SetUserParam(void* ptr);
	void* GetUserParam();
	void onGetLog(std::string url);
	int SetCamTimeZone(int tz);
	int GetCamTimeZone();
	time_t GetCloudTime();
	void onSetByEventMode(bool bMode);
	void onSetImageParams(image_params_t*);
	void onGetImageParams(image_params_t*);
	void onSetMotionParams(motion_params_t* mpr);
	void onGetMotionParams(motion_params_t* mpr);
	void onSetTimeZone(const char* time_zone_str);
	void onGetTimeZone(char* time_zone_str);
	void onGetPTZParams(ptz_caps_t* ptz);
	void onSendPTZCommand(ptz_command_t* ptz);
	int onSendCameraCommand(cam_command_t* ccmd);
	void onGetOSDParams(osd_settings_t* osd);
	void onSetOSDParams(osd_settings_t* osd);
	void onGetAudioParams(audio_settings_t* audset);
	void onSetAudioParams(audio_settings_t* audset);
	void onSetLogEnable(bool bEnable);
	void onSetActivity(bool bEnable);
	void setVersionOverride(string ver);
	void onTriggerEvent(string evt, string meta);
	void onStartBackward(string url);
	void onStopBackward(string url);
	void onSetPeriodicEvents(const char* name, int period, bool active);
	void onGetEventLimits(time_t* pre, time_t* post);
	void onSetEventLimits(time_t pre, time_t post);
	void onGetWiFiList(wifi_list_t* wifilist);
	void onSetCurrenWiFi(wifi_params* params);
	//<=ICameraManagerCallback
};

#endif //__CLOUDSTREAMERSDK_H__
