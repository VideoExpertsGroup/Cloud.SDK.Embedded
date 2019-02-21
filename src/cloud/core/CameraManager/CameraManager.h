
#ifndef __CAMERAMANAGERWEBSOCKET_H__
#define __CAMERAMANAGERWEBSOCKET_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"
#include "CameraManagerConfig.h"
#include "ICameraManagerCallback.h"

#include "../../../utils/wswrap.h"
#include "CameraEvent.h"

class CameraManager : public CUnk
{
//	const char *TAG = "CameraManager";
//	const int LOG_LEVEL = 2; //Log.VERBOSE;

	CameraManagerConfig mCameraManagerConfig;
	ICameraManagerCallback *mCallback;

	CWSClientWrapper mWebSocket;
	int messageId;

	bool misPrepared;
	bool misCamRegistered;
	std::string mCameraConfig;
	std::string mStreamUrl;

	bool misReconnect;
	std::string mByeReason;

	DWORD calling_thread_id;
	
	int m_nCamTZ;

public:
	MLog Log;

	CameraManager();
	virtual ~CameraManager();

	int Open(CameraManagerConfig &config, ICameraManagerCallback *callback);
	int Close();
	int send_cam_event(const CameraEvent &camEvent);

	std::string getCameraConfig() { return mCameraConfig; };
	std::string getStreamUrl() { return mStreamUrl; };

	int confirm_direct_upload(std::string url);

	int set_cam_tz(int tz);
	int get_cam_tz();

private:
	int Reconnect();

	static int OnEvent(void* inst, WEBSOCKWRAP_EVENT reason, void *in, size_t len);

	void onConnected();
	void onDisconnected();
	void onReceive(std::string data);

	void on_update_cam_config();

	//=>send
	int send_cmd_register();	   //send on first websocket connect
	int send_cmd_cam_register();   //send on HELLO cmd
	int send_cmd_done(long long cmd_id, std::string cmd, std::string status);
	//<=send

	//=>recv
	int recv_cmd_HELLO(std::string data);
	int recv_cmd_CAM_HELLO(std::string data);
	int recv_cmd_STREAM_START(std::string data);
	int recv_cmd_STREAM_STOP(std::string data);
	int recv_cmd_BYE(std::string data);
	int recv_cmd_CAM_UPDATE_PREVIEW(std::string data);
	int recv_cmd_CONFIGURE(std::string data);
	int recv_cmd_GET_AUDIO_DETECTION(std::string data);
	int recv_cmd_GET_CAM_AUDIO_CONF(std::string data);
	int recv_cmd_GET_CAM_EVENTS(std::string data);
	int recv_cmd_GET_CAM_STATUS(std::string data);
	int recv_cmd_GET_CAM_VIDEO_CONF(std::string data);
	int recv_cmd_DIRECT_UPLOAD_URL(std::string data);
	int recv_cmd_GET_MOTION_DETECTION(std::string data);
	int recv_cmd_GET_STREAM_BY_EVENT(std::string data);
	int recv_cmd_GET_STREAM_CAPS(std::string data);
	int recv_cmd_GET_STREAM_CONFIG(std::string data);
	int recv_cmd_GET_SUPPORTED_STREAMS(std::string data);
	int recv_cmd_SET_CAM_AUDIO_CONF(std::string data);
	int recv_cmd_SET_CAM_EVENTS(std::string data);
	int recv_cmd_SET_CAM_VIDEO_CONF(std::string data);
	int recv_cmd_SET_MOTION_DETECTION(std::string data);
	int recv_cmd_SET_STREAM_CONFIG(std::string data);
	int recv_cmd_GET_PTZ_CONF(std::string data);

	int recv_cmd_RAW_MESSAGE(std::string data);

	int recv_cmd_CAM_GET_LOG(std::string data);

	//<=recv

};
#endif //__CAMERAMANAGERWEBSOCKET_H__