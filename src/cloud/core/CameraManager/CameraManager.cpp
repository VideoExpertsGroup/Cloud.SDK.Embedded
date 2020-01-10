
#ifdef WIN32
#include <windows.h>
#include <timeapi.h>
#endif

#include "CameraManager.h"
#include "CameraManagerCommands.h"
#include "CameraManagerParams.h"
#include "CameraManagerDoneStatus.h"
#include "CameraManagerByeReasons.h"
#include "../../../Enums/CloudReturnCodes.h"
#include <jansson.h>

#ifdef  __cplusplus
extern "C" {
#endif
//#include "../../../../external_libs/libwebsockets/src/lib/libwebsockets.h"
#ifdef  __cplusplus
}
#endif

void add_string_array(json_t* arr, const char* name, char* str)
{
	json_t *tmp = json_array();
	char* buf = strtok(str, ",");
	while (buf)
	{
		int buflen = strlen(buf);
		int idx = 0;
		while (buf[idx] == ' ' && idx < buflen)idx++; //remove leading spaces
		json_array_append_new(tmp, json_string(&buf[idx]));
		buf = strtok(NULL, ",");
	}
	json_object_set_new(arr, name, tmp);
}


int CameraManager::OnEvent(void* inst, WEBSOCKWRAP_EVENT reason, void *in, size_t len)
{
	CameraManager *p = (CameraManager *)inst;
	if (!p)
		return -1;

	p->Log.d(">=OnEvent");

	switch (reason)
	{
	case WEBSOCKWRAP_CONNECT:
		p->Log.d("WEBSOCKWRAP_CONNECT");
		p->onConnected();
		break;
	case WEBSOCKWRAP_DISCONNECT:
		p->Log.d("WEBSOCKWRAP_DISCONNECT");
		p->onDisconnected();
		break;
	case WEBSOCKWRAP_RECEIVE:
		if (len)
		{
			char* szIn = new char[len + 1];
			memcpy(szIn, in, len);
			szIn[len] = 0;
//			p->Log.d("WEBSOCKWRAP_RECEIVE %s", szIn);
			p->onReceive(szIn);
//			p->Log.d("WEBSOCKWRAP_RECEIVE1");
			delete[] szIn;
//			p->Log.d("WEBSOCKWRAP_RECEIVE end");
		}
		break;
	case WEBSOCKWRAP_ERROR:
		if (p->mCallback) {
			p->mCallback->onClosed(RET_ERROR_NO_CLOUD_CONNECTION, "");
		}
		p->Log.d("WEBSOCKWRAP_ERROR %x",p->mCallback);
		break;
	case WEBSOCKWRAP_LWS_OTHER:
		//p->Log.d("WEBSOCKWRAP_LWS_OTHER");
		break;
	default:
		p->Log.d("Unknown reason 0x%X", reason);
		break;
	}

	p->Log.d("<=OnEvent");


	return 0;
}


CameraManager::CameraManager()
	: Log("CameraManager", 2)
	  ,mWebSocket(OnEvent, this)
{
	messageId = 1;
	misPrepared = false;
	misCamRegistered = false;
	calling_thread_id = 0;
	m_nCamTZ = 0;
	closed = false;
	mByeReason = "";
	mStreamUrl = "";
	mCameraConfig = "";
	m_bHelloDone = false;
	mPubSID = -1;
}

CameraManager::~CameraManager()
{
	Close();
}

int CameraManager::Open(CameraManagerConfig &config, ICameraManagerCallback *callback)
{
	Log.d("=>Open");

	misPrepared = false;
	misCamRegistered = false;
	closed = false;

	mCameraManagerConfig = config;
	mCallback = callback;

	on_update_cam_config();

	std::string uri;
	uri = mCameraManagerConfig.getAddress();

Log.d("Open %s", uri.c_str());

	int ret = mWebSocket.Connect(uri.c_str());
	if (ret < 0) {
		ret = RET_ERROR_NO_CLOUD_CONNECTION;
		Log.d("Open error RET_ERROR_NO_CLOUD_CONNECTION");
		if (mCallback) {
			mCallback->onClosed(RET_ERROR_NO_CLOUD_CONNECTION, "");
		}
	}

	mCameraManagerConfig.setReconnectAddress("");
	config.setReconnectAddress("");

	Log.d("<=Open ret=%d", ret);

	return ret;
}

int CameraManager::Reconnect()
{
	Log.d("=>Reconnect");

	std::string uri;
	uri = mCameraManagerConfig.getAddress();

	misReconnect = true;
	mWebSocket.Disconnect();
	misReconnect = false;
	closed = false;
	m_bHelloDone = false;

Log.d(">>>Reconnect to %s", uri.c_str());

	misCamRegistered = false;

	int ret = mWebSocket.Connect(uri.c_str());
	if (ret < 0) {
		Log.d("Reconnect error RET_ERROR_NO_CLOUD_CONNECTION");
		ret = RET_ERROR_NO_CLOUD_CONNECTION;
		if (mCallback) {
			mCallback->onClosed(RET_ERROR_NO_CLOUD_CONNECTION, "");
		}
	}

	Log.d("<=Reconnect ret=%d", ret);

	return ret;
}

int CameraManager::Close()
{
	Log.d("=>Close");

	closed = true;

	send_cmd_bye();
	Sleep(100);
	mWebSocket.Disconnect();
	m_bHelloDone = false;

	Log.d("<=Close");
	return 0;
}


void CameraManager::onConnected()
{
	Log.d("<=onConnected");
	send_cmd_register();
}

void CameraManager::onDisconnected()
{
	Log.d("<=onDisconnected %d",misReconnect);
	if (misReconnect)
		return;

	if (mCallback)
		mCallback->onClosed(-1, mByeReason);
}

void CameraManager::onReceive(std::string data)
{
	Log.d("=>onReceive Receive message: %s", data.c_str());
//	Log.d("=>onReceive");

	json_error_t  err;
	json_t * jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		Log.e("<=onReceive err=%s", err.text? err.text:"NULL");
		return;
	}

	const char *scmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));

	if (!scmd) {
		Log.e("<=onReceive cmd==NULL");
		json_decref(jdata);
		return;
	}

	if (!strncmp(scmd, CameraManagerCommands::HELLO, strlen(scmd))) {
		recv_cmd_HELLO(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::CAM_HELLO, strlen(scmd))) {
		recv_cmd_CAM_HELLO(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::STREAM_START, strlen(scmd))) {
		recv_cmd_STREAM_START(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::STREAM_STOP, strlen(scmd))) {
		recv_cmd_STREAM_STOP(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::BYE, strlen(scmd))) {
		recv_cmd_BYE(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::CAM_UPDATE_PREVIEW, strlen(scmd))) {
		recv_cmd_CAM_UPDATE_PREVIEW(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::CONFIGURE, strlen(scmd))) {
		recv_cmd_CONFIGURE(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_AUDIO_DETECTION, strlen(scmd))) {
		recv_cmd_GET_AUDIO_DETECTION(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_CAM_AUDIO_CONF, strlen(scmd))) {
		recv_cmd_GET_CAM_AUDIO_CONF(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_CAM_EVENTS, strlen(scmd))) {
		recv_cmd_GET_CAM_EVENTS(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_CAM_STATUS, strlen(scmd))) {
		recv_cmd_GET_CAM_STATUS(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_CAM_VIDEO_CONF, strlen(scmd))) {
		recv_cmd_GET_CAM_VIDEO_CONF(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::DIRECT_UPLOAD_URL, strlen(scmd))) {
		recv_cmd_DIRECT_UPLOAD_URL(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_MOTION_DETECTION, strlen(scmd))) {
		recv_cmd_GET_MOTION_DETECTION(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_STREAM_BY_EVENT, strlen(scmd))) {
		recv_cmd_GET_STREAM_BY_EVENT(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_STREAM_CAPS, strlen(scmd))) {
		recv_cmd_GET_STREAM_CAPS(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_STREAM_CONFIG, strlen(scmd))) {
		recv_cmd_GET_STREAM_CONFIG(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_SUPPORTED_STREAMS, strlen(scmd))) {
		recv_cmd_GET_SUPPORTED_STREAMS(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::SET_CAM_AUDIO_CONF, strlen(scmd))) {
		recv_cmd_SET_CAM_AUDIO_CONF(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::SET_CAM_EVENTS, strlen(scmd))) {
		recv_cmd_SET_CAM_EVENTS(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::SET_CAM_VIDEO_CONF, strlen(scmd))) {
		recv_cmd_SET_CAM_VIDEO_CONF(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::SET_MOTION_DETECTION, strlen(scmd))) {
		recv_cmd_SET_MOTION_DETECTION(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::SET_STREAM_CONFIG, strlen(scmd))) {
		recv_cmd_SET_STREAM_CONFIG(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_PTZ_CONF, strlen(scmd))) {
		recv_cmd_GET_PTZ_CONF(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::CAM_PTZ, strlen(scmd))) {
		recv_cmd_CAM_PTZ(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::RAW_MESSAGE, strlen(scmd))) {
		recv_cmd_RAW_MESSAGE(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::CAM_GET_LOG, strlen(scmd))) {
		recv_cmd_CAM_GET_LOG(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::GET_OSD_CONF, strlen(scmd))) {
		recv_cmd_GET_OSD_CONF(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::SET_OSD_CONF, strlen(scmd))) {
		recv_cmd_SET_OSD_CONF(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::SET_CAM_PARAMETER, strlen(scmd))) {
		recv_cmd_SET_CAM_PARAMETER(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::CAM_TRIGGER_EVENT, strlen(scmd))) {
		recv_cmd_CAM_TRIGGER_EVENT(data);
	}
	else {
		Log.e("<=onReceive unhandled cmd=%s", scmd);
	}

	json_decref(jdata);

//	Log.d("<=onReceive Receive message: %s", data.c_str());
//	Log.d("<=onReceive");

}

void CameraManager::on_update_cam_config()
{

	Log.d("%s", __FUNCTION__);

	json_t *jo = json_object();

	if(!mCameraManagerConfig.getUUID().empty())
		json_object_set_new(jo, CameraManagerParams::UUID, json_string(mCameraManagerConfig.getUUID().c_str()));
	if (!mCameraManagerConfig.getSID().empty())
		json_object_set_new(jo, CameraManagerParams::SID, json_string(mCameraManagerConfig.getSID().c_str()));
	if (!mCameraManagerConfig.getPwd().empty())
		json_object_set_new(jo, CameraManagerParams::PWD, json_string(mCameraManagerConfig.getPwd().c_str()));
	if (!mCameraManagerConfig.getConnID().empty())
		json_object_set_new(jo, CameraManagerParams::CONNID, json_string(mCameraManagerConfig.getConnID().c_str()));
	json_object_set_new(jo, CameraManagerParams::CAM_ID, json_integer(mCameraManagerConfig.getCamID()));

	char* jstr = json_dumps(jo, 0);
	mCameraConfig = jstr;
	free(jstr);
	json_decref(jo);

	if (mCallback)
		mCallback->onUpdateConfig(mCameraManagerConfig);
}


//=>Camera Manager comands
int CameraManager::send_cmd_register()
{

	string scmd;

	if (mCallback)
	{
		char tz[256] = { 0 };
		mCallback->onGetTimeZone(tz);
		if (strlen(tz))
			mCameraManagerConfig.setCameraTimezone(tz);
	}

	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::REGISTER));
	json_object_set_new(jcmd, CameraManagerParams::MSGID, json_integer(messageId++));
	json_object_set_new(jcmd, CameraManagerParams::VER, json_string(mCameraManagerConfig.getCMVersion().c_str()));
	json_object_set_new(jcmd, CameraManagerParams::TZ, json_string(mCameraManagerConfig.getCameraTimezone().c_str()));
	json_object_set_new(jcmd, CameraManagerParams::VENDOR, json_string(mCameraManagerConfig.getCameraVendor().c_str()));
	if (!mCameraManagerConfig.getPwd().empty()) {
		json_object_set_new(jcmd, CameraManagerParams::PWD, json_string(mCameraManagerConfig.getPwd().c_str()));
	}
	if (!mCameraManagerConfig.getSID().empty()) {
		json_object_set_new(jcmd, CameraManagerParams::PREV_SID, json_string(mCameraManagerConfig.getSID().c_str()));
	}
	if (!mCameraManagerConfig.getRegToken().isEmpty()) {
		json_object_set_new(jcmd, CameraManagerParams::REG_TOKEN, json_string(mCameraManagerConfig.getRegToken().getToken().c_str()));
	}


        char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	json_decref(jcmd);
	free(jstr);

	Log.d("send_cmd_register()=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());
}

int CameraManager::send_cmd_bye()
{
	Log.d("=>%s", __FUNCTION__);

	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::BYE));
	json_object_set_new(jcmd, CameraManagerParams::MSGID, json_integer(messageId++));
	json_object_set_new(jcmd, CameraManagerParams::REASON, json_string(CameraManagerByeReasons::RECONNECT));

	char* jstr = json_dumps(jcmd, 0);
	string scmd = jstr;
	json_decref(jcmd);
	free(jstr);

	Log.d("%s %s", __FUNCTION__, scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());
}

int CameraManager::send_cmd_cam_register() {


	string scmd;

	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_REGISTER));
	json_object_set_new(jcmd, CameraManagerParams::MSGID, json_integer(messageId++));

	json_object_set_new(jcmd, CameraManagerParams::IP, json_string(mCameraManagerConfig.getCameraIPAddress().c_str()));
	json_object_set_new(jcmd, CameraManagerParams::UUID, json_string(mCameraManagerConfig.getUUID().c_str()));
	json_object_set_new(jcmd, CameraManagerParams::BRAND, json_string(mCameraManagerConfig.getCameraBrand().c_str()));
	json_object_set_new(jcmd, CameraManagerParams::MODEL, json_string(mCameraManagerConfig.getCameraModel().c_str()));
	json_object_set_new(jcmd, CameraManagerParams::SN, json_string(mCameraManagerConfig.getCameraSerialNumber().c_str()));
	json_object_set_new(jcmd, CameraManagerParams::RAW_MESSAGING, json_true());

	if (mCameraManagerConfig.getVersionOverride().empty())
		json_object_set_new(jcmd, CameraManagerParams::VERSION, json_string(mCameraManagerConfig.getCameraVersion().c_str()));
	else
		json_object_set_new(jcmd, CameraManagerParams::VERSION, json_string(mCameraManagerConfig.getVersionOverride().c_str()));


	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(jcmd);

	Log.d("send_cmd_cam_register()=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}


int CameraManager::send_cmd_done(long long cmd_id, std::string cmd, std::string status)
{


//Log.d("=>send_cmd_done");
//Log.d("cmd=%s", cmd.c_str());
//Log.d("status=%s", status.c_str());

	string scmd;

	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::DONE));
	json_object_set_new(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set_new(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd.c_str()));
	json_object_set_new(jcmd, CameraManagerParams::STATUS, json_string(status.c_str()));
	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;

	free(jstr);

	json_decref(jcmd);

	Log.d("send_cmd_done()=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}

static char* GetStringFromTime(time_t ttime)
{
    struct tm* timeinfo;
    static char sztime[32];
    char mstmp[32];
    timeinfo = gmtime(&ttime);
    int msec = timeGetTime()%1000;
    strftime(mstmp, sizeof(mstmp), "%Y%m%dT%H%M%S", timeinfo);
    sprintf(sztime,"%s.%d",mstmp, msec);
    return sztime;
}

int CameraManager::send_cam_event(const CameraEvent &camEvent)
{

	Log.d("%s", __FUNCTION__);

	if (camEvent.event.empty()) {
		Log.e("<=send_cam_event() error camEvent.event.empty()");
		return -1;
	}

	int idx = messageId;
	string scmd;
	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_EVENT));
	json_object_set_new(jcmd, CameraManagerParams::MSGID, json_integer(messageId++));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(mCameraManagerConfig.getCamID()));
	json_object_set_new(jcmd, CameraManagerParams::EVENT, json_string(camEvent.event.c_str()));

	if (camEvent.event != "videoloss")
	{
		json_t *motion_info = json_object();
		json_object_set(jcmd, "motion_info", motion_info);

		json_t *snapshot_info = json_object();
		json_object_set_new(snapshot_info, "image_time", json_string(GetStringFromTime(camEvent.timeUTC)));
		json_object_set_new(snapshot_info, "size", json_integer(camEvent.data_size));
		json_object_set_new(snapshot_info, "width", json_integer(camEvent.snapshot_width));
		json_object_set_new(snapshot_info, "height", json_integer(camEvent.snapshot_height));
		json_object_set(jcmd, "snapshot_info", snapshot_info);
		json_decref(motion_info);
		json_decref(snapshot_info);
	}

	json_object_set_new(jcmd, CameraManagerParams::TIME, json_real(camEvent.timeUTC));

	if (!camEvent.meta.empty())
	{
		json_error_t err;
		json_t* jmeta = json_loads(camEvent.meta.c_str(), 0, &err);
		if (jmeta)
			json_object_set_new(jcmd, CameraManagerParams::META, jmeta);
	}

//	if (!camEvent.status.empty()) json_object_set_new(jcmd, CameraManagerParams::STATUS, json_string(camEvent.status.c_str()));
//	json_object_set_new(jcmd, CameraManagerParams::STATUS, json_string("OK"));

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(jcmd);

//	Log.d("send_cam_event()=%s", scmd.c_str());

	int err = mWebSocket.WriteBack(scmd.c_str());

	if(err<=0)
		return err;

	return idx;
}

int CameraManager::recv_cmd_HELLO(std::string data)
{
	Log.d("recv_cmd_HELLO %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));

	const char *p = json_string_value(json_object_get(jdata, CameraManagerParams::CA));
	if (p) {
		mCameraManagerConfig.setCA(p);
	}
	p = json_string_value(json_object_get(jdata, CameraManagerParams::SID));
	if (p) {
		mCameraManagerConfig.setSID(p);
	}
	p = json_string_value(json_object_get(jdata, CameraManagerParams::UPLOAD_URL));
	if (p) {
		mCameraManagerConfig.setUploadURL(p);
	}
	p = json_string_value(json_object_get(jdata, CameraManagerParams::MEDIA_SERVER));
	if (p) {
		mCameraManagerConfig.setMediaServer(p);
	}
	p = json_string_value(json_object_get(jdata, CameraManagerParams::CONNID));
	if (p) {
		mCameraManagerConfig.setConnID(p);
	}

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	if(!misCamRegistered){
		if (send_cmd_cam_register() >= 0)
			misCamRegistered = true;
	}

	on_update_cam_config();

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_CAM_HELLO(std::string data)
{
	Log.d("recv_cmd_CAM_HELLO %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));

	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));
	mCameraManagerConfig.setCamID(cam_id);

	const char *p = json_string_value(json_object_get(jdata, CameraManagerParams::MEDIA_URL));
	if (p) {
		mCameraManagerConfig.setCamPath(p);
	}
	p = json_string_value(json_object_get(jdata, CameraManagerParams::PATH));
	if (p) {
		mCameraManagerConfig.setCamPath(p);
	}
	p = json_string_value(json_object_get(jdata, CameraManagerParams::MEDIA_URI));
	if (p) {
		mCameraManagerConfig.setMediaServer(p);
	}
	p = json_string_value(json_object_get(jdata, CameraManagerParams::ACTIVITY));
	if (p) {
		mCameraManagerConfig.setCameraActivity( json_boolean_value(json_object_get(jdata, CameraManagerParams::ACTIVITY)));
	}

	on_update_cam_config();

	if (!mCameraManagerConfig.getMediaServer().empty()) {
		string mediaServerURL = mCameraManagerConfig.getMediaServer();
		string cam_path = mCameraManagerConfig.getCamPath();
		string stream_id = "Main";//mVXGCloudCamera.getStreamID();
		string sid = mCameraManagerConfig.getSID();

		mStreamUrl = "rtmp://" + mediaServerURL + "/" + cam_path + stream_id + "?sid=" + sid;
		misPrepared = true;

		if (false == m_bHelloDone)
		{
			if (mCallback)
				mCallback->onPrepared();
		}
	}

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	m_bHelloDone = true;

	return 0;
}

int CameraManager::recv_cmd_STREAM_START(std::string data)
{

	Log.d("=>recv_cmd_STREAM_START %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	json_t* jps = json_object_get(jdata, CameraManagerParams::PUB_SID);
	if (jps)
		setPublishSID(json_integer_value(jps));

	const char *reason = NULL;
	json_t* rj = json_object_get(jdata, CameraManagerParams::REASON);
	if (rj)
		reason = json_string_value(rj);

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);

		json_decref(jdata);
		return -2;
	}

	Log.d("recv_cmd_STREAM_START cb1");

	if (reason && 0 == strcmp(reason, "record_by_event"))
	{
		Log.d("%s reason record_by_event",__FUNCTION__);
		if (mCallback)
			mCallback->onSetByEventMode(true);
	}
	else
	{
		if (mCallback)
			mCallback->onStreamStart();
	}

	Log.d("recv_cmd_STREAM_START cb2");

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	Log.d("recv_cmd_STREAM_START cb3");

	json_decref(jdata);

	Log.d("<=recv_cmd_STREAM_START");

	return 0;
}

int CameraManager::recv_cmd_STREAM_STOP(std::string data)
{
	Log.d("=>recv_cmd_STREAM_STOP %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));
	const char *reason = NULL;
	json_t* rj = json_object_get(jdata, CameraManagerParams::REASON);
	if (rj)
		reason = json_string_value(rj);

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);

		json_decref(jdata);
		return -2;
	}

	DWORD th_id = GetCurrentThreadId();

	if (calling_thread_id && (calling_thread_id == th_id))
	{
		Log.d("recv_cmd_STREAM_STOP same thread");
		//return 0;
	}

	calling_thread_id = th_id;

	if (reason && 0 == strcmp(reason, "record_by_event"))
	{
		Log.d("%s reason record_by_event", __FUNCTION__);
		if (mCallback)
			mCallback->onSetByEventMode(false);
	}
	else
	{
		if (mCallback)
			mCallback->onStreamStop();
	}

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	Log.d("<=recv_cmd_STREAM_STOP");

	return 0;
}

int CameraManager::recv_cmd_BYE(std::string data)
{
	Log.d("recv_cmd_BYE %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *reason = json_string_value(json_object_get(jdata, CameraManagerParams::REASON));
	long long retry = json_integer_value(json_object_get(jdata, CameraManagerParams::RETRY));

	if (reason == NULL){
		json_decref(jdata);
		return -2;
	}

	mByeReason = reason;
	m_bHelloDone = false;

	Log.d("recv_cmd_BYE retry=%d", retry);

	if(retry>0)
	{
		Log.d("sleeping %dsec", retry);
		for(int x=0; x < 10*retry; x++)
		{
			Sleep(100);
			if(closed)
				break;
		}
	}

	if (!strncmp(reason, CameraManagerByeReasons::RECONNECT, strlen(reason)))
	{
		Reconnect();
	}
	else //INVALID_USER, AUTH_FAILURE, CONN_CONFLICT etc
	{
		if(retry<=0)
		{
			Log.d("sleep 10000");
			for(int x=0; x < 100; x++)
			{
				Sleep(100);
				if(closed)
					break;
			}
		}
	}

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_CAM_UPDATE_PREVIEW(std::string data)
{
	Log.d("recv_cmd_CAM_UPDATE_PREVIEW %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);

		json_decref(jdata);
		return -2;
	}

	if (mCallback)
		mCallback->onUpdatePreview();

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_CONFIGURE(std::string data)
{
	Log.d("recv_cmd_CONFIGURE %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));

	// TODO check with reconnect
	bool bConfigChanged = false;
	bool bForceReconnect = false;
	const char *p;
	p = json_string_value(json_object_get(jdata, CameraManagerParams::PWD));
	if (p) {
		mCameraManagerConfig.setPwd(p);
		bConfigChanged = true;
	}

	p = json_string_value(json_object_get(jdata, CameraManagerParams::UUID));
	if (p) {
		mCameraManagerConfig.setUUID(p);
		bConfigChanged = true;
	}

	p = json_string_value(json_object_get(jdata, CameraManagerParams::CONNID));
	if (p) {
		mCameraManagerConfig.setConnID(p);
		bConfigChanged = true;
	}

	p = json_string_value(json_object_get(jdata, CameraManagerParams::SERVER));
	if (p) {
		mCameraManagerConfig.setReconnectAddress(p);
		Log.d("onConfigureReceive() newAddress=%s", p);
		bConfigChanged = true;
		bForceReconnect = true;
	}

	p = json_string_value(json_object_get(jdata, CameraManagerParams::TZ));
	if (p) {
		mCameraManagerConfig.setCameraTimezone(p);
		Log.d("onConfigureReceive() setCameraTimezone=%s", p);
		if (mCallback)
			mCallback->onSetTimeZone(p);
	}

	if (bConfigChanged) {
		on_update_cam_config();
	}

	if (bForceReconnect) {

	}
	else {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);
	}

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_GET_AUDIO_DETECTION(std::string data)
{
	Log.d("recv_cmd_GET_AUDIO_DETECTION %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);

		json_decref(jdata);
		return -2;
	}

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_GET_CAM_AUDIO_CONF(std::string data)
{

	Log.d("recv_cmd_GET_CAM_AUDIO_CONF %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));

	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);

		json_decref(jdata);
		return -2;
	}

	if (!mCallback) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	audio_settings_t audset;
	SetDefaultAudioParams(&audset);
	mCallback->onGetAudioParams(&audset);

	string scmd;

	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_AUDIO_CONF));
	json_object_set_new(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set_new(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	if(audset.mic_gain!=PARAMETER_NOT_SET)
		json_object_set_new(jcmd, CameraManagerParams::MIC_GAIN, json_integer(audset.mic_gain));
	if (audset.mic_mute != PARAMETER_NOT_SET)
		json_object_set_new(jcmd, CameraManagerParams::MIC_MUTE, json_boolean((bool)audset.mic_mute));
	if (audset.spkr_vol != PARAMETER_NOT_SET)
		json_object_set_new(jcmd, CameraManagerParams::SPKR_VOL, json_integer(audset.spkr_vol));
	if (audset.spkr_mute != PARAMETER_NOT_SET)
		json_object_set_new(jcmd, CameraManagerParams::SPKR_MUTE, json_boolean((bool)audset.spkr_mute));
	if (strlen(audset.echo_cancel))
		json_object_set_new(jcmd, CameraManagerParams::ECHO_CANCEL, json_string(audset.echo_cancel));

	//audio caps
	json_t *aud_caps = json_object();
	if (audset.caps.mic != PARAMETER_NOT_SET)
		json_object_set_new(aud_caps, CameraManagerParams::MIC, json_boolean((bool)audset.caps.mic));
	else
		json_object_set_new(aud_caps, CameraManagerParams::MIC, json_false());

	if (audset.caps.spkr != PARAMETER_NOT_SET)
		json_object_set_new(aud_caps, CameraManagerParams::SPKR, json_boolean((bool)audset.caps.spkr));
	else
		json_object_set_new(aud_caps, CameraManagerParams::SPKR, json_false());

	if (audset.caps.backward != PARAMETER_NOT_SET)
		json_object_set_new(aud_caps, CameraManagerParams::BACKWARD, json_boolean((bool)audset.caps.backward));
	else
		json_object_set_new(aud_caps, CameraManagerParams::BACKWARD, json_false());

	if (strlen(audset.caps.backward_formats))
		add_string_array(aud_caps, CameraManagerParams::BACKWARD_FMTS, audset.caps.backward_formats);

	json_object_set(jcmd, CameraManagerParams::CAPS, aud_caps);

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(aud_caps);
	json_decref(jcmd);
	json_decref(jdata);

	Log.d("recv_cmd_GET_CAM_AUDIO_CONF send=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}

int CameraManager::recv_cmd_GET_CAM_EVENTS(std::string data)
{
	//{"cmd":"cam_events_conf", "refid" : 12, "orig_cmd" : "get_cam_events", "cam_id" : 164614, "enabled" : true, "events" : [{"caps":{"stream":true, "snapshot" : false}, "event" : "record", "active" : true, "stream" : true, "snapshot" : false}, { "caps":{"stream":true,"snapshot" : false},"event" : "memorycard","active" : true,"stream" : true,"snapshot" : false }]}

	Log.d("recv_cmd_GET_CAM_EVENTS %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));

	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));


	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);

		json_decref(jdata);
		return -2;
	}

	string scmd;

	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_EVENTS_CONF));
	json_object_set_new(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set_new(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	//events status
	json_object_set_new(jcmd, CameraManagerParams::ENABLED, json_true());

	//events array
	json_t *events = json_array();

	/*
	Available Events:
	� �motion�  for motion detection events
	� �sound� for audio detection
	� �net� for the camera network status change
	� �record� CM informs server about necessity of changing of recording state
	� �memorycard� camera's memory-card status change
	� �wifi� status of camera's currently used Wi-Fi
	*/

	//actual set
	std::string sEvents[] = { "motion", "sound", "record" , "facedetection", "alarm", "sensor", "device", "time" };

	std::string *se = sEvents;
	int arr_length = sizeof(sEvents) / sizeof(*sEvents);

	for (int i =0; i< arr_length; i++,se++) {

		//event
		json_t *jevent = json_object();

		//caps
		json_t *event_caps = json_object();
		json_object_set_new(event_caps, CameraManagerParams::STREAM, json_true());
		json_object_set_new(event_caps, CameraManagerParams::SNAPSHOT, json_true());//json_false());
		json_object_set_new(event_caps, CameraManagerParams::TRIGGER, json_boolean((i > 3)));//external events

		json_object_set_new(jevent, CameraManagerParams::CAPS, event_caps);
		json_object_set_new(jevent, CameraManagerParams::ACTIVE, json_true());
		json_object_set_new(jevent, CameraManagerParams::STREAM, json_true());
		json_object_set_new(jevent, CameraManagerParams::SNAPSHOT, json_true());//json_false());
		//name
		json_object_set_new(jevent, CameraManagerParams::EVENT, json_string(se->c_str()));


		//add event to events array
		json_array_append(events, jevent);

		json_decref(jevent);
	}

	json_object_set_new(jcmd, CameraManagerParams::EVENTS, events);

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(events);
	json_decref(jcmd);
	json_decref(jdata);

	Log.d("recv_cmd_GET_CAM_EVENTS send=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}

int CameraManager::recv_cmd_GET_CAM_STATUS(std::string data)
{
	Log.d("recv_cmd_GET_CAM_STATUS %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));

	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);

		json_decref(jdata);
		return -2;
	}

	string scmd;

	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_STATUS));
	json_object_set_new(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set_new(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	//camera status
	json_object_set_new(jcmd, CameraManagerParams::IP, json_string(mCameraManagerConfig.getCameraIPAddress().c_str()));
	json_object_set_new(jcmd, CameraManagerParams::ACTIVITY, json_true() /*json_boolean(mCameraManagerConfig.getCameraActivity())*/);
	json_object_set_new(jcmd, CameraManagerParams::STREAMING, json_boolean(mCameraManagerConfig.getCameraStreaming()));

	//json_object_set_new(jcmd, CameraManagerParams::STATUS_LED, json_boolean(mCameraManagerConfig.getCameraStatusLed()));
	json_object_set_new(jcmd, CameraManagerParams::STATUS_LED, json_boolean(MLog::globalLevel != LOGLEVEL_ERROR));

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(jcmd);
	json_decref(jdata);

	Log.d("recv_cmd_GET_CAM_STATUS send=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}

int CameraManager::recv_cmd_GET_CAM_VIDEO_CONF(std::string data)
{
	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -2;
	}

	if (!mCallback){
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	string scmd="";
	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_VIDEO_CONF));
	json_object_set_new(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set_new(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	json_t *caps = json_object();

	image_params_t img;
	SetDefaultImageParams(&img);
	mCallback->onGetImageParams(&img);
	char szValue[256] = { 0 };

	if (img.brightness != PARAMETER_NOT_SET)
	{
		json_object_set_new(jcmd, "brightness", json_integer(img.brightness));
		json_object_set(caps, "brightness", json_boolean(1));
	}
	else 
		json_object_set(caps, "brightness", json_boolean(0));

	if (img.contrast != PARAMETER_NOT_SET)
	{
		json_object_set_new(jcmd, "contrast", json_integer(img.contrast));
		json_object_set(caps, "contrast", json_boolean(1));
	}
	else
		json_object_set(caps, "contrast", json_boolean(0));

	if (img.saturation != PARAMETER_NOT_SET)
	{
		json_object_set_new(jcmd, "saturation", json_integer(img.saturation));
		json_object_set(caps, "saturation", json_boolean(1));
	}
	else
		json_object_set(caps, "saturation", json_boolean(0));
//
//	if (img.hue != PARAMETER_NOT_SET)
//	{
//		json_object_set_new(jcmd, "hue", json_integer(img.hue));
//		json_object_set(caps, "hue", json_boolean(1));
//	}
//	else
//		json_object_set(caps, "hue", json_boolean(0));

	if (img.sharpness != PARAMETER_NOT_SET)
	{
		json_object_set_new(jcmd, "sharpness", json_integer(img.sharpness));
		json_object_set(caps, "sharpness", json_boolean(1));
	}
	else
		json_object_set(caps, "sharpness", json_boolean(0));

	if (img.power_freq_60hz != PARAMETER_NOT_SET)
	{
		if(img.power_freq_60hz)
			json_object_set_new(jcmd, "pwr_frequency", json_integer(60));
		else
			json_object_set_new(jcmd, "pwr_frequency", json_integer(50));
		json_object_set(caps, "pwr_frequency", json_boolean(1));
	}
	else
		json_object_set(caps, "pwr_frequency", json_boolean(0));

	if (img.white_balance[0])
	{
		json_object_set_new(jcmd, "wb_type", json_string(img.white_balance));

		if (img.white_balance_types != (char**)PARAMETER_NOT_SET)
		{
			json_t *val_caps = json_array();
			int n = 0;
			while (img.white_balance_types[n])
			{
				json_array_append_new(val_caps, json_string(img.white_balance_types[n]));
				n++;
			}
			json_object_set(caps, "wb_type", val_caps);
			json_decref(val_caps);
		}
	}
	else
	{
		json_t *val_caps = json_array();
		json_object_set(caps, "wb_type", val_caps);
		json_decref(val_caps);
	}

	if (img.noise_reduction_level != PARAMETER_NOT_SET)
	{
		json_object_set_new(jcmd, "nr_level", json_integer(img.noise_reduction_level));
		json_object_set(caps, "nr_level", json_boolean(1));
	}
	else
		json_object_set(caps, "nr_level", json_boolean(0));

	if (img.noise_reduction[0])
	{
		json_object_set_new(jcmd, "nr_type", json_string(img.noise_reduction));

		if (img.noise_reduction_types != (char**)PARAMETER_NOT_SET)
		{
			json_t *val_caps = json_array();
			int n = 0;
			while (img.noise_reduction_types[n])
			{
				json_array_append_new(val_caps, json_string(img.noise_reduction_types[n]));
				n++;
			}
			json_object_set(caps, "nr_type", val_caps);
			json_decref(val_caps);
		}
	}
	else
	{
		json_t *val_caps = json_array();
		json_object_set(caps, "nr_type", val_caps);
		json_decref(val_caps);
	}

	if (img.vert_flip != PARAMETER_NOT_SET)
	{
		img.vert_flip == 1 ? json_object_set_new(jcmd, "vert_flip", json_string("on")): json_object_set_new(jcmd, "vert_flip", json_string("off"));
		json_t *val_caps = json_array();
		json_array_append_new(val_caps, json_string("off"));
		json_array_append_new(val_caps, json_string("on"));
		json_object_set(caps, "vert_flip", val_caps);
		json_decref(val_caps);
	}
	else
	{
		json_t *val_caps = json_array();
		json_object_set(caps, "vert_flip", val_caps);
		json_decref(val_caps);
	}

	if (img.horz_flip != PARAMETER_NOT_SET)
	{
		img.horz_flip == 1 ? json_object_set_new(jcmd, "horz_flip", json_string("on")) : json_object_set_new(jcmd, "horz_flip", json_string("off"));
		json_t *val_caps = json_array();
		json_array_append_new(val_caps, json_string("off"));
		json_array_append_new(val_caps, json_string("on"));
		json_object_set(caps, "horz_flip", val_caps);
		json_decref(val_caps);
	}
	else
	{
		json_t *val_caps = json_array();
		json_object_set(caps, "horz_flip", val_caps);
		json_decref(val_caps);
	}

	if (img.ir_light != PARAMETER_NOT_SET)
	{
		if (img.ir_light == 0)
			json_object_set_new(jcmd, "ir_light", json_string("off"));
		else
			if (img.ir_light == 1)
				json_object_set_new(jcmd, "ir_light", json_string("on"));
			else
				if (img.ir_light == 2)
					json_object_set_new(jcmd, "ir_light", json_string("auto"));

		json_t *val_caps = json_array();
		json_array_append_new(val_caps, json_string("off"));
		json_array_append_new(val_caps, json_string("on"));
		json_array_append_new(val_caps, json_string("auto"));
		json_object_set(caps, "ir_light", val_caps);
		json_decref(val_caps);
	}
	else
	{
		json_t *val_caps = json_array();
		json_object_set(caps, "ir_light", val_caps);
		json_decref(val_caps);
	}

	if (img.tdn != PARAMETER_NOT_SET)
	{
		if(img.tdn==0)
			json_object_set_new(jcmd, "tdn", json_string("night"));
		else
			if (img.tdn == 1)
				json_object_set_new(jcmd, "tdn", json_string("day"));
			else
				if (img.tdn == 2)
					json_object_set_new(jcmd, "tdn", json_string("auto"));

		json_t *val_caps = json_array();
		json_array_append_new(val_caps, json_string("night"));
		json_array_append_new(val_caps, json_string("day"));
		json_array_append_new(val_caps, json_string("auto"));
		json_object_set(caps, "tdn", val_caps);
		json_decref(val_caps);
	}
	else
	{
		json_t *val_caps = json_array();
		json_object_set(caps, "tdn", val_caps);
		json_decref(val_caps);
	}


	json_object_set(jcmd, "caps", caps);
	json_decref(caps);

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);
	
	json_decref(jcmd);
	json_decref(jdata);

	//Log.d("%s send=%s", __FUNCTION__, scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}

int CameraManager::recv_cmd_DIRECT_UPLOAD_URL(std::string data)
{

	Log.d("%s",__FUNCTION__);

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		Log.d("%s json error, data=%s", __FUNCTION__, data.c_str());
		return -1;
	}

	const char *status  = json_string_value(json_object_get(jdata, "status"));
	if(!(status && !strcmp(status,"OK")))
	{
		json_decref(jdata);
		Log.d("%s status!=OK, data=%s", __FUNCTION__, data.c_str());
		return -2;
	}

	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));
	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd  = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -2;
	}

	std::string url  = json_string_value(json_object_get(jdata, "url"));
	int refid = json_integer_value(json_object_get(jdata, "refid"));
	Log.d("cmd=%s, url=%s, refid=%d", cmd, url.c_str(), refid);

	if (mCallback)
		mCallback->onRecvUploadUrl(url, refid);

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_GET_MOTION_DETECTION(std::string data)
{

	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -2;
	}

	if (!mCallback) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	string scmd = "";
	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::MOTION_DETECTION_CONF));
	json_object_set_new(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set_new(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	motion_params_t mpr;
	SetDefaultMotionParams(&mpr);
	mCallback->onGetMotionParams(&mpr);
	char szValue[256] = { 0 };
	
	if (mpr.columns != PARAMETER_NOT_SET)
		json_object_set_new(jcmd, "columns", json_integer(mpr.columns));

	if (mpr.rows != PARAMETER_NOT_SET)
		json_object_set_new(jcmd, "rows", json_integer(mpr.rows));

#define MAX_REGIONS (1)

	json_t *caps = json_object();
	json_object_set_new(caps, "sensitivity", json_string("frame"));
	json_object_set_new(caps, "max_regions", json_integer(MAX_REGIONS));
	json_object_set_new(caps, "region_shape", json_string("any"));
	json_object_set(jcmd, "caps", caps);
	json_decref(caps);


	json_t *regions = json_array();
	for (int n = 0; n < MAX_REGIONS; n++)
	{
		json_t *singlereg = json_object();
		if (mpr.sensitivity != PARAMETER_NOT_SET)
			json_object_set_new(singlereg, "sensitivity", json_integer(mpr.sensitivity));

		if (mpr.enabled != PARAMETER_NOT_SET)
			json_object_set_new(singlereg, "enabled", json_boolean(mpr.enabled));

		if (mpr.gridmap)
		{
			json_object_set_new(singlereg, "map", json_string(mpr.gridmap));
			free(mpr.gridmap);
		}

		char name[64]; sprintf(name, "region_%d", n);
		json_object_set_new(singlereg, "region", json_string(name));
		json_object_set_new(singlereg, "enabled", json_boolean(true));
		json_array_append_new(regions, singlereg);
	}

	json_object_set(jcmd, "regions", regions);
	json_decref(regions);

	char* jstr = json_dumps(jcmd, 0);
	Log.d("%s jstr=%s", __FUNCTION__, jstr);

	scmd = jstr;
	free(jstr);

	json_decref(jcmd);
	json_decref(jdata);

	return mWebSocket.WriteBack(scmd.c_str());
}

int CameraManager::recv_cmd_GET_STREAM_BY_EVENT(std::string data)
{
	return -1;
}

int CameraManager::recv_cmd_GET_STREAM_CAPS(std::string data)
{
	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	std::string streamCaps;
	long long cmd_id;
	const char *cmd;
	long long cam_id;

	Log.d("get_stream_caps: %s", data.c_str());

	if (!jdata) {
		return -1;
	}

	cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -2;
	}

	streamCaps.clear();
	if (mCallback)
		mCallback->onCommand(data, streamCaps);
//{"cam_id": 5, "cmd": "stream_caps", "msgid": 15, "caps_video": [{"brt": [0, 0, 1], "vbr": false, "streams": ["video1f726d4041355b119704b63a69232f7d"], "fps": [0], "formats": ["H264"], "resolutions": [[800, 600]], "quality": [0, 0], "gop": [0, 0, 1]}]}
//{"cam_id": 3, "cmd": "stream_caps", "msgid": 16, "caps_video": [{"streams" : ["Vid"], "formats" : ["H264"], "resolutions":[ [1280, 720], [1920, 1080] ],"fps" : [30], "gop" : [30], "brt" : [1024, 2048, 128], "vbr" : false, "quality" : [0,0]}],
//"caps_audio": [    {        "streams" : ["Aud"],        "formats" : ["AAC"],        "brt" : [64, 128, 64],        "srt" : [32.0, 44.1, 48.0]}]}

	streamCaps = "{\"cam_id\":" + std::to_string(cam_id) + ", " +
					  "\"cmd\": " + "\"" + "stream_caps"  + "\", "  +
					  "\"msgid\": " + std::to_string(cmd_id + 1) + ","  +
					   streamCaps + "}";

	return mWebSocket.WriteBack(streamCaps.c_str());;
}

int CameraManager::recv_cmd_GET_STREAM_CONFIG(std::string data)
{
	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	long long cmd_id;
	const char *cmd;
	long long cam_id;
	string stream_config;
	const char* video_stream_name = NULL;
	const char* audio_stream_name = NULL;
	string video_config_str;
	string audio_config_str;
	string retVal;

	Log.d("get_stream_config: %s", data.c_str());

	if (!jdata) {
		return -1;
	}

	cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));
	json_unpack(json_object_get(jdata, CameraManagerParams::VIDEO_ES), "[s]", &video_stream_name);
	json_unpack(json_object_get(jdata, CameraManagerParams::AUDIO_ES), "[s]", &audio_stream_name);

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -2;
	}

	stream_config =  "{\"msgid\":" + std::to_string(cmd_id + 1);
	stream_config += ",\"cam_id\":" + std::to_string(cam_id);
	stream_config += ",\"cmd\": \"";
	stream_config += CameraManagerCommands::STREAM_CONFIG;
	stream_config += "\"";

	retVal.clear();
	if (mCallback)
		mCallback->onCommand(data, retVal);

	if (!retVal.empty()) {
		mCameraManagerConfig.setStreamConfig(new StreamConfig(retVal));
	}


	video_config_str = "\"video\": [";
	if (video_stream_name && strlen(video_stream_name)) {
		StreamConfigVideo video_config = mCameraManagerConfig.getStreamConfig()->video_streams[video_stream_name];

		char szFPS[32];
		sprintf(szFPS, "%f", video_config.getFps());
		string fff = szFPS;
		Log.d("fps=%f, szFPS = %s", video_config.getFps(), szFPS);

		video_config_str += "{\"vert\": ";
		video_config_str += std::to_string(video_config.getVert());
		video_config_str += ",\"horz\": " + std::to_string(video_config.getHorz());
		video_config_str += ",\"fps\":" + fff;// std::to_string(video_config.getFps());
		video_config_str += ",\"quality\": " + std::to_string(video_config.getQuality());
		video_config_str += ",\"stream\": \"" + video_config.getStream() + "\"";
		video_config_str += ",\"gop\": " + std::to_string(video_config.getGop());
		video_config_str += ", \"format\": \"H264\", \"vbr\": true, \"brt\": 2000";
		video_config_str += "}";
	}
	video_config_str += "]";

	audio_config_str = "\"audio\": [";
	if (audio_stream_name && strlen(audio_stream_name)) {
		audio_config_str += "";
	}
	audio_config_str += "]";

	stream_config += "," + video_config_str;
	stream_config += ", "+ audio_config_str;
	stream_config += "}";

	return mWebSocket.WriteBack(stream_config.c_str());;
}

int CameraManager::recv_cmd_GET_SUPPORTED_STREAMS(std::string data)
{
	Log.d("recv_cmd_GET_SUPPORTED_STREAMS %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));

	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);

		json_decref(jdata);
		return -2;
	}


	string scmd;

	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::SUPPORTED_STREAMS));
	json_object_set_new(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set_new(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	//audio
	json_t *video = json_array();
	json_array_append_new(video, json_string("Aud"));
	json_object_set(jcmd, CameraManagerParams::AUDIO_ES, video);

	//video
	json_t *audio = json_array();
	json_array_append_new(audio, json_string("Vid"));
	json_object_set(jcmd, CameraManagerParams::VIDEO_ES, audio);

	//stream
	json_t *stream1 = json_object();
	json_object_set_new(stream1, CameraManagerParams::ID, json_string("Main")); // TODO hardcoded
	json_object_set_new(stream1, CameraManagerParams::VIDEO, json_string("Vid")); // TODO hardcoded
	json_object_set_new(stream1, CameraManagerParams::AUDIO, json_string("Aud")); // TODO hardcoded
	json_t *stream = json_array();
	json_array_append(stream, stream1);

	json_object_set(jcmd, CameraManagerParams::STREAMS, stream);

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(video);
	json_decref(audio);
	json_decref(stream1);
	json_decref(stream);
	json_decref(jcmd);
	json_decref(jdata);

	Log.d("recv_cmd_GET_SUPPORTED_STREAMS send=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}

int CameraManager::recv_cmd_SET_CAM_AUDIO_CONF(std::string data)
{

	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -2;
	}

	if (!mCallback) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	audio_settings_t audset;
	SetDefaultAudioParams(&audset);
	json_t* tmp = NULL;

	tmp = json_object_get(jdata, CameraManagerParams::MIC_GAIN);
	if (tmp)
		audset.mic_gain = json_integer_value(tmp);
	tmp = json_object_get(jdata, CameraManagerParams::MIC_MUTE);
	if (tmp)
		audset.mic_mute = json_boolean_value(tmp);
	tmp = json_object_get(jdata, CameraManagerParams::SPKR_VOL);
	if (tmp)
		audset.spkr_vol = json_integer_value(tmp);
	tmp = json_object_get(jdata, CameraManagerParams::SPKR_MUTE);
	if (tmp)
		audset.spkr_mute = json_boolean_value(tmp);
	tmp = json_object_get(jdata, CameraManagerParams::ECHO_CANCEL);
	if (tmp)
	{
		const char* szval = json_string_value(tmp);
		if(szval)
			strcpy(audset.echo_cancel, szval);
	}

	mCallback->onSetAudioParams(&audset);

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);
	json_decref(jdata);
	return 0;
}

int CameraManager::recv_cmd_SET_CAM_EVENTS(std::string data)
{
	Log.d("recv_cmd_SET_CAM_EVENTS %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);
	return 0;

	//TODO add memory card info
	/*response.put(CameraManagerParameterNames.CMD, CameraManagerCommandNames.CAM_EVENT);
	response.put(CameraManagerParameterNames.EVENT, "memorycard");
	response.put(CameraManagerParameterNames.CAM_ID, mCameraID);
	Double time = Double.valueOf(System.currentTimeMillis());
	time = time / 1000;
	response.put(CameraManagerParameterNames.TIME, time);

	JSONObject memorycardinfo = new JSONObject();
	memorycardinfo.put(CameraManagerParameterNames.STATUS, CameraManagerMemoryCardStatus.NORMAL);
	memorycardinfo.put(CameraManagerParameterNames.SIZE, mSize); // in MB
	memorycardinfo.put(CameraManagerParameterNames.FREE, mFree); // in MB

	response.put(CameraManagerParameterNames.MEMORYCARD_INFO, memorycardinfo);*/
}

int CameraManager::recv_cmd_SET_CAM_PARAMETER(std::string data)
{
	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);

		json_decref(jdata);
		return -2;
	}

	json_t* tmp = NULL;
	tmp = json_object_get(jdata, "status_led");
	if (tmp)
	{
		bool bLogEnabled = json_boolean_value(tmp);
		if (mCallback)
			mCallback->onSetLogEnable(bLogEnabled);
		mCameraManagerConfig.setCameraStatusLed(bLogEnabled);
	}

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	return 0;

}

int CameraManager::recv_cmd_SET_CAM_VIDEO_CONF(std::string data)
{
	Log.d("recv_cmd_SET_CAM_VIDEO_CONF %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);

		json_decref(jdata);
		return -2;
	}

	image_params_t img;
	SetDefaultImageParams(&img);

	json_t* tmp = NULL;
	tmp = json_object_get(jdata, "brightness");
	if (tmp)
	{
		if (json_is_integer(tmp))
		{
			img.brightness = json_integer_value(tmp);
		}
		else if (json_is_string(tmp))
		{
			img.brightness = atoi(json_string_value(tmp));
		}
	}

	tmp = json_object_get(jdata, "saturation");
	if (tmp)
	{
		if (json_is_integer(tmp))
		{
			img.saturation = json_integer_value(tmp);
		}
		else if (json_is_string(tmp))
		{
			img.saturation = atoi(json_string_value(tmp));
		}
	}

	tmp = json_object_get(jdata, "contrast");
	if (tmp)
	{
		if (json_is_integer(tmp))
		{
			img.contrast = json_integer_value(tmp);
		}
		else if (json_is_string(tmp))
		{
			img.contrast = atoi(json_string_value(tmp));
		}
	}
/*
	tmp = json_object_get(jdata, "hue");
	if (tmp)
	{
		if (json_is_integer(tmp))
		{
			img.hue = json_integer_value(tmp);
		}
		else if (json_is_string(tmp))
		{
			img.hue = atoi(json_string_value(tmp));
		}
	}
*/
	tmp = json_object_get(jdata, "sharpness");
	if (tmp)
	{
		if (json_is_integer(tmp))
		{
			img.sharpness = json_integer_value(tmp);
		}
		else if (json_is_string(tmp))
		{
			img.sharpness = atoi(json_string_value(tmp));
		}
	}

	tmp = json_object_get(jdata, "pwr_frequency");
	if (tmp)
	{
		if (json_is_integer(tmp))
		{
			if(60==json_integer_value(tmp))
				img.power_freq_60hz = 1;
			else
				img.power_freq_60hz = 0;
		}
	}

	tmp = json_object_get(jdata, "wb_type");
	if (tmp)
	{
		//strcpy_s(img.white_balance, PARAMETER_CHAR_MAX_LENGTH, json_string_value(tmp));
		const char* szTmp = json_string_value(tmp);
		if (strlen(szTmp) < PARAMETER_CHAR_MAX_LENGTH)
			strcpy(img.white_balance, szTmp);

	}

	tmp = json_object_get(jdata, "nr_type");
	if (tmp)
	{
		//strcpy_s(img.noise_reduction, PARAMETER_CHAR_MAX_LENGTH, json_string_value(tmp));
		const char* szTmp = json_string_value(tmp);
		if (strlen(szTmp) < PARAMETER_CHAR_MAX_LENGTH)
			strcpy(img.noise_reduction, szTmp);
	}

	tmp = json_object_get(jdata, "nr_level");
	if (tmp)
	{
		img.noise_reduction_level = json_integer_value(tmp);
	}

	

	tmp = json_object_get(jdata, "vert_flip");
	if (tmp)
	{
		img.vert_flip = !strcmp(json_string_value(tmp), "on");
	}

	tmp = json_object_get(jdata, "horz_flip");
	if (tmp)
	{
		img.horz_flip = !strcmp(json_string_value(tmp), "on");
	}

	tmp = json_object_get(jdata, "ir_light");
	if (tmp)
	{
		if(!strcmp(json_string_value(tmp), "off"))
			img.ir_light = 0;
		else 
			if (!strcmp(json_string_value(tmp), "on"))
				img.ir_light = 1;
			else
				if (!strcmp(json_string_value(tmp), "auto"))
					img.ir_light = 2;
	}

	tmp = json_object_get(jdata, "tdn");
	if (tmp)
	{
		if (!strcmp(json_string_value(tmp), "night"))
			img.tdn = 0;
		else
			if (!strcmp(json_string_value(tmp), "day"))
				img.tdn = 1;
			else
				if (!strcmp(json_string_value(tmp), "auto"))
					img.tdn = 2;
	}

	if (mCallback)
		mCallback->onSetImageParams(&img);

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_SET_MOTION_DETECTION(std::string data)
{
	Log.d("recv_cmd_SET_MOTION_DETECTION %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		Log.e("recv_cmd_SET_MOTION_DETECTION cam_id!=getCamID");
		return -2;
	}

	if (!mCallback) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	motion_params_t mpr;
	SetDefaultMotionParams(&mpr);

	//{"regions": [{"map": "6zEBMDDrMQEwMOsxATAw6zEBMDDrMQEwMOsxATAw6zEBMDDrMQEwMOsxATAw6zEBMDDrMQEwMOsxATAw6zEBMDDrMQEwMOsxATAw6zEBMDDrMQEwMOsxATAw", "sensitivity": 70, "enabled": true, "region": "region_0"}], "msgid": 23, "cmd": "set_motion_detection", "cam_id": 197897}

	json_t* regions = json_object_get(jdata, "regions");
	if (regions)
	{
		json_t* singlereg = json_array_get(regions, 0);
		if (singlereg)
		{
			char* test = json_dumps(singlereg, 0);
			Log.d("test %s", test);
			//{"map": "6zEBMDDrMQEwMOsxATAw6zEBMDDrMQEwMOsxATAw6zEBMDDrMQEwMOsxATAw6zEBMDDrMQEwMOsxATAw6zEBMDDrMQEwMOsxATAw6zEBMDDrMQEwMOsxATAw", "sensitivity": 70, "enabled": true, "region": "region_0"}
			
			json_t* tmp = json_object_get(singlereg, "sensitivity");
			if (tmp)
				mpr.sensitivity = json_integer_value(tmp);

			tmp = json_object_get(singlereg, "enabled");
			if (tmp)
				mpr.enabled = json_boolean_value(tmp);

			tmp = json_object_get(singlereg, "map");
			if (tmp)
			{
				string strmap = json_string_value(tmp);
				mpr.gridmap = (char*)malloc(strmap.length()+1);
				strcpy(mpr.gridmap, strmap.c_str());
			}
		}
	}

	mCallback->onSetMotionParams(&mpr);

	if (mpr.gridmap)
		free(mpr.gridmap);

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);
	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_SET_STREAM_CONFIG(std::string data)
{
	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));

	Log.d("recv_cmd_SET_STREAM_CONFIG: %s", data.c_str());

	mCameraManagerConfig.setStreamConfig(new StreamConfig(data));
	if (mCallback)
		mCallback->onUpdateConfig(mCameraManagerConfig);

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);
	return 0;
}

int CameraManager::recv_cmd_GET_PTZ_CONF(std::string data)
{
	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));
	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -2;
	}

	if (!mCallback) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	ptz_caps_t ptzpr;
	SetDefaultPTZParams(&ptzpr);
	mCallback->onGetPTZParams(&ptzpr);

	int bPtzSupported = (ptzpr.left || ptzpr.right || ptzpr.bottom || ptzpr.top || ptzpr.zoom_in || ptzpr.zoom_out || ptzpr.stop);

	string scmd = "";
	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set_new(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	if (!bPtzSupported)
	{
		FreePTZParams(&ptzpr);
		json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::DONE));
		json_object_set_new(jcmd, CameraManagerParams::STATUS, json_string("NOT_SUPPORTED"));
		char* jstr = json_dumps(jcmd, 0);
		scmd = jstr;
		free(jstr);
		json_decref(jcmd);
		Log.d("send done with status NOT_SUPPORTED");
		return mWebSocket.WriteBack(scmd.c_str());
	}

	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_PTZ_CONF));

	if (ptzpr.max_presets != PARAMETER_NOT_SET)
	{
		if (ptzpr.max_presets > MAX_PTZ_PRESETS)
			ptzpr.max_presets = MAX_PTZ_PRESETS;
		json_object_set_new(jcmd, "maximum_number_of_presets", json_integer(ptzpr.max_presets));
	}

	json_t *actions = json_array();
	if (ptzpr.left)json_array_append_new(actions, json_string("left"));
	if (ptzpr.right)json_array_append_new(actions, json_string("right"));
	if (ptzpr.bottom)json_array_append_new(actions, json_string("bottom"));
	if (ptzpr.top)json_array_append_new(actions, json_string("top"));
	if (ptzpr.zoom_in)json_array_append_new(actions, json_string("zoom_in"));
	if (ptzpr.zoom_out)json_array_append_new(actions, json_string("zoom_out"));
	if (ptzpr.stop)json_array_append_new(actions, json_string("stop"));

	json_t* presets = json_array();

	if (ptzpr.max_presets != PARAMETER_NOT_SET)
	{
		for (int n = 0; n < ptzpr.max_presets; n++)
		{
			if (ptzpr.preset[n].token || ptzpr.preset[n].name)
			{
				json_t *singlepreset = json_object();
				int needadd = 0;
				if (ptzpr.preset[n].token && strlen(ptzpr.preset[n].token))
				{
					json_object_set_new(singlepreset, "token", json_string(ptzpr.preset[n].token));
					needadd++;
				}

				if (ptzpr.preset[n].name && strlen(ptzpr.preset[n].name))
				{
					json_object_set_new(singlepreset, "name", json_string(ptzpr.preset[n].name));
					needadd++;
				}

				if (needadd > 0)
					json_array_append(presets, singlepreset);

				char* aastr = json_dumps(singlepreset, 0);
				Log.d("%s singlepreset=%s", __FUNCTION__, aastr);
				char* bbstr = json_dumps(presets, 0);
				Log.d("%s presets=%s", __FUNCTION__, bbstr);

				json_decref(singlepreset);
			}
		}
	}

	json_object_set(jcmd, "actions", actions);
	json_object_set(jcmd, "presets", presets);
	json_decref(actions);
	json_decref(presets);

	char* jstr = json_dumps(jcmd, 0);
	Log.d("%s jstr=%s", __FUNCTION__, jstr);

	scmd = jstr;
	free(jstr);
	json_decref(jcmd);
	json_decref(jdata);

	FreePTZParams(&ptzpr);

	return mWebSocket.WriteBack(scmd.c_str());
}

int CameraManager::recv_cmd_CAM_PTZ(std::string data)
{
	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		Log.e("cam_id!=getCamID");
		return -2;
	}

	if (!mCallback) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	const char *action = json_string_value(json_object_get(jdata, "action"));
	if (action)
	{
		ptz_command_t pcmd;
		pcmd.command = (char*)action;
		mCallback->onSendPTZCommand(&pcmd);
	}

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);
	json_decref(jdata);
	Log.d("%s: %s", __FUNCTION__, data.c_str());

	return 0;
}

int CameraManager::recv_cmd_CAM_COMMAND(std::string data)
{
	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		Log.e("cam_id!=getCamID");
		return -2;
	}

	if (!mCallback) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	cam_command_t ccmd;
	ccmd.cmd_id = CAM_CMD_REBOOT;
	ccmd.cmd_param = NULL;
	int ret = mCallback->onSendCameraCommand(&ccmd);

	ret ? send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR) : send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);
	Log.d("%s: %s", __FUNCTION__, data.c_str());

	return 0;
}

int CameraManager::recv_cmd_GET_OSD_CONF(std::string data)
{
	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		Log.e("recv_cmd_RAW_MESSAGE json_loads failed");
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		Log.e("recv_cmd_RAW_MESSAGE cam_id!=getCamID");
		return -2;
	}

	if (!mCallback) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	osd_settings_t osdset;
	SetDefaultOSDSettings(&osdset);
	mCallback->onGetOSDParams(&osdset);

	int bOSDSupported = (1);

	string scmd = "";
	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set_new(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	if (!bOSDSupported)
	{
		FreeOSDSettings(&osdset);
		json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::DONE));
		json_object_set_new(jcmd, CameraManagerParams::STATUS, json_string("NOT_SUPPORTED"));
		char* jstr = json_dumps(jcmd, 0);
		scmd = jstr;
		free(jstr);
		json_decref(jcmd);
		Log.d("send done with status NOT_SUPPORTED");
		return mWebSocket.WriteBack(scmd.c_str());
	}

//parameters
	osd_params_t* osdparam = &osdset.params;

	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::OSD_CONF));

	if (osdparam->system_id != PARAMETER_NOT_SET)
		json_object_set_new(jcmd, "system_id", json_boolean(osdparam->system_id));

	json_object_set_new(jcmd, "system_id_text", json_string(osdparam->system_id_text));

	if (osdparam->time != PARAMETER_NOT_SET)
		json_object_set_new(jcmd, "time", json_boolean(osdparam->time));

	if (osdparam->date != PARAMETER_NOT_SET)
		json_object_set_new(jcmd, "date", json_boolean(osdparam->date));

	if (strlen(osdparam->time_format))
		json_object_set_new(jcmd, "time_format", json_string(osdparam->time_format));

	if (strlen(osdparam->date_format))
		json_object_set_new(jcmd, "date_format", json_string(osdparam->date_format));

	if (strlen(osdparam->font_size))
		json_object_set_new(jcmd, "font_size", json_string(osdparam->font_size));

	if (strlen(osdparam->font_color))
		json_object_set_new(jcmd, "font_color", json_string(osdparam->font_color));

	if (strlen(osdparam->bkg_color))
		json_object_set_new(jcmd, "bkg_color", json_string(osdparam->bkg_color));

	if (osdparam->bkg_transp != PARAMETER_NOT_SET)
		json_object_set_new(jcmd, "bkg_transp", json_boolean(osdparam->bkg_transp));

	if (strlen(osdparam->alignment))
		json_object_set_new(jcmd, "alignment", json_string(osdparam->alignment));

//caps

	json_t* caps = json_object();

	osdparam = &osdset.caps;

	json_object_set_new(caps, CameraManagerParams::CMD, json_string(CameraManagerCommands::OSD_CONF));

	if (osdparam->system_id != PARAMETER_NOT_SET)
		json_object_set_new(caps, "system_id", json_boolean(osdparam->system_id));

	json_object_set_new(caps, "system_id_text", json_boolean(strlen(osdparam->system_id_text)>0));

	if (osdparam->time != PARAMETER_NOT_SET)
		json_object_set_new(caps, "time", json_boolean(osdparam->time));

	if (osdparam->date != PARAMETER_NOT_SET)
		json_object_set_new(caps, "date", json_boolean(osdparam->date));

	if (osdparam->bkg_transp != PARAMETER_NOT_SET)
		json_object_set_new(caps, "bkg_transp", json_boolean(osdparam->bkg_transp));

	add_string_array(caps, "time_format", osdparam->time_format);
	add_string_array(caps, "date_format", osdparam->date_format);
	add_string_array(caps, "font_size", osdparam->font_size);
	add_string_array(caps, "font_color", osdparam->font_color);
	add_string_array(caps, "bkg_color", osdparam->bkg_color);
	add_string_array(caps, "alignment", osdparam->alignment);

	json_object_set(jcmd, "caps", caps);
	json_decref(caps);

	char* jstr = json_dumps(jcmd, 0);
	Log.d("%s jstr=%s", __FUNCTION__, jstr);

	scmd = jstr;
	free(jstr);
	json_decref(jcmd);
	json_decref(jdata);

	FreeOSDSettings(&osdset);

	return mWebSocket.WriteBack(scmd.c_str());
}

int CameraManager::recv_cmd_CAM_TRIGGER_EVENT(std::string data)
{
	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t* jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char* cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		Log.e("cam_id!=getCamID");
		return -2;
	}

	if (!mCallback) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	//{"msgid": 21, "cmd": "cam_trigger_event", "meta": {"TEST_ALARM_1": ""}, "event": "alarm", "cam_id": 9052}

	string strevent;
	string strmeta;
	json_t* jevent = json_object_get(jdata, "event");
	if(jevent)
		strevent = json_string_value(jevent);
	json_t* jmeta = json_object_get(jdata, "meta");
	if (jmeta)
	{
		char* meta = json_dumps(jmeta, 0);
		strmeta = meta;
		free(meta);
	}
	
	if (!strevent.empty())
	{
		Log.d("event=%s, meta=%s", strevent.c_str(), strmeta.c_str());
		mCallback->onTriggerEvent(strevent, strmeta);
	}

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);
	json_decref(jdata);
	return 0;
}


void add_osd_value(json_t* jdata, const char* name, char* dst)
{
	json_t* jtmp = json_object_get(jdata, name);
	if (jtmp)
	{
		const char *tmp = json_string_value(jtmp);
		if (tmp && strlen(tmp) < MAX_OSD_LENGTH)
			strcpy(dst, tmp);
	}
}
void add_osd_value(json_t* jdata, const char* name, int* dst)
{
	json_t* jtmp = json_object_get(jdata, name);
	if (jtmp)
		*dst = json_boolean_value(jtmp);
}

int CameraManager::recv_cmd_SET_OSD_CONF(std::string data)
{
	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		Log.e("recv_cmd_RAW_MESSAGE json_loads failed");
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		Log.e("recv_cmd_RAW_MESSAGE cam_id!=getCamID");
		return -2;
	}

	if (!mCallback) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -3;
	}

	osd_settings_t osdset;
	osd_params_t* osdptr = &osdset.params;
	const char *tmp = NULL;
	SetDefaultOSDSettings(&osdset);


	add_osd_value(jdata, "system_id_text", osdptr->system_id_text);
	add_osd_value(jdata, "time_format", osdptr->time_format);
	add_osd_value(jdata, "date_format", osdptr->date_format);
	add_osd_value(jdata, "font_size", osdptr->font_size);
	add_osd_value(jdata, "font_color", osdptr->font_color);
	add_osd_value(jdata, "bkg_color", osdptr->bkg_color);
	add_osd_value(jdata, "alignment", osdptr->alignment);
	add_osd_value(jdata, "system_id", &osdptr->system_id);
	add_osd_value(jdata, "time", &osdptr->time);
	add_osd_value(jdata, "date", &osdptr->date);
	add_osd_value(jdata, "bkg_transp", &osdptr->bkg_transp);

	mCallback->onSetOSDParams(&osdset);

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);
	json_decref(jdata);
	Log.d("%s: %s", __FUNCTION__, data.c_str());

	return 0;
}

int CameraManager::recv_cmd_RAW_MESSAGE(std::string data)
{
	Log.e("recv_cmd_RAW_MESSAGE %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		Log.e("recv_cmd_RAW_MESSAGE json_loads failed");
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		Log.e("recv_cmd_RAW_MESSAGE cam_id!=getCamID");
		return -2;
	}

	int raw_ret = -1;
	if (mCallback)
		raw_ret = mCallback->onRawMessage(data);

	Log.e("recv_cmd_RAW_MESSAGE raw_ret=%d", raw_ret);

	Log.e("2 data = %s", data.c_str());

	if(raw_ret>=0)
		mWebSocket.WriteBack(data.c_str());

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	return raw_ret;
}

int CameraManager::confirm_direct_upload(std::string url)
{
	if(url.empty())
		return -1;

	Log.d("%s", __FUNCTION__);

	string scmd;
	json_t *jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string("confirm_direct_upload"));
	json_object_set_new(jcmd, CameraManagerParams::MSGID, json_integer(messageId++));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(mCameraManagerConfig.getCamID()));
	json_object_set_new(jcmd, "url", json_string(url.c_str()));

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(jcmd);
//	Log.d("confirm_direct_upload()=%s", scmd.c_str());
	return mWebSocket.WriteBack(scmd.c_str());
}

int CameraManager::recv_cmd_CAM_GET_LOG(std::string data)
{

	Log.d("%s %s", __FUNCTION__, data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *cmd = json_string_value(json_object_get(jdata, CameraManagerParams::CMD));
	long long cam_id = json_integer_value(json_object_get(jdata, CameraManagerParams::CAM_ID));

	if (cam_id != mCameraManagerConfig.getCamID()) {
		send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::CM_ERROR);
		json_decref(jdata);
		return -2;
	}

	time_t tutc = time(NULL) + m_nCamTZ;

	if (mCallback)
	{
		time_t ttmp = mCallback->GetCloudTime();
		if (ttmp)
			tutc = ttmp;
	}

	std::string time_utc = GetStringFromTime(tutc);
	std::string url = "http://" + mCameraManagerConfig.getUploadURL() + "/" + mCameraManagerConfig.getCamPath() + "?sid=" + mCameraManagerConfig.getSID() + "&cat=log&type=txt&start=" + time_utc;

	Log.d("recv_cmd_CAM_GET_LOG url=%s", url.c_str());

	if (mCallback)
		mCallback->onGetLog(url);

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);
	json_decref(jdata);

	return 0;

}

int CameraManager::set_cam_tz(int tz)
{
	m_nCamTZ = tz;
	return m_nCamTZ;
}

int CameraManager::get_cam_tz()
{
	return m_nCamTZ;
}

int CameraManager::get_direct_upload_url(unsigned long timeUTC, const char* type, const char* category, int size, int duration)
{
	Log.d("%s", __FUNCTION__);

	int idx = messageId;
	string scmd;
	json_t* jcmd = json_object();
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::GET_DIRECT_UPLOAD_URL));
	json_object_set_new(jcmd, CameraManagerParams::MSGID, json_integer(messageId++));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(mCameraManagerConfig.getCamID()));
	json_object_set_new(jcmd, "file_time", json_string(GetStringFromTime(timeUTC)));
	json_object_set_new(jcmd, "size", json_integer(size));
	json_object_set_new(jcmd, "duration", json_integer(duration));
	json_object_set_new(jcmd, "stream_id", json_integer(0));
	json_object_set_new(jcmd, CameraManagerParams::TIME, json_real(timeUTC));

	if(category)
		json_object_set_new(jcmd, "category", json_string(category));
	if(type)
		json_object_set_new(jcmd, "type", json_string(type));

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);
	json_decref(jcmd);

	Log.d("%s %s", __FUNCTION__, scmd.c_str());

	int err = mWebSocket.WriteBack(scmd.c_str());
	if (err <= 0)return err;

	return idx;

}

int CameraManager::CloudPing()
{
	return mWebSocket.Ping();
}

time_t CameraManager::CloudPong()
{
	return mWebSocket.m_tLastPongTime;
}

//<=Camera Manager comands
