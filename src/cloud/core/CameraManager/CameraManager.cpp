
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

int CameraManager::OnEvent(void* inst, WEBSOCKWRAP_EVENT reason, void *in, size_t len)
{
	CameraManager *p = (CameraManager *)inst;
	if (!p)
		return -1;

	switch (reason)
	{
	case WEBSOCKWRAP_CONNECT:
		p->Log.v("WEBSOCKWRAP_CONNECT");
		p->onConnected();
		break;
	case WEBSOCKWRAP_DISCONNECT:
		p->Log.v("WEBSOCKWRAP_DISCONNECT");
		p->onDisconnected();
		break;
	case WEBSOCKWRAP_RECEIVE:
		if (len)
		{
			char* szIn = new char[len + 1];
			memcpy(szIn, in, len);
			szIn[len] = 0;
//			p->Log.v("WEBSOCKWRAP_RECEIVE %s", szIn);
			p->onReceive(szIn);
			delete[] szIn;
		}
		break;
	case WEBSOCKWRAP_ERROR:
		if (p->mCallback) {
			p->mCallback->onClosed(RET_ERROR_NO_CLOUD_CONNECTION, "");
		}
		p->Log.v("WEBSOCKWRAP_ERROR %x",p->mCallback);
		break;
	case WEBSOCKWRAP_LWS_OTHER:
		//p->Log.v("WEBSOCKWRAP_LWS_OTHER");
		break;
	default:
		p->Log.v("Unknown reason 0x%X", reason);
		break;
	}
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
	mByeReason = "";
}

CameraManager::~CameraManager()
{

}

int CameraManager::Open(CameraManagerConfig &config, ICameraManagerCallback *callback)
{
	Log.v("=>Open");

	misPrepared = false;
	misCamRegistered = false;

	mCameraManagerConfig = config;
	mCallback = callback;

	on_update_cam_config();

	std::string uri;
	uri = mCameraManagerConfig.getAddress();

	int ret = mWebSocket.Connect(uri.c_str());
	if (ret < 0) {
		ret = RET_ERROR_NO_CLOUD_CONNECTION;
		Log.v("Open error RET_ERROR_NO_CLOUD_CONNECTION");
		if (mCallback) {
			mCallback->onClosed(RET_ERROR_NO_CLOUD_CONNECTION, "");
		}
	}

	Log.v("<=Open ret=%d", ret);

	return ret;
}

int CameraManager::Reconnect()
{
	Log.v("=>Reconnect");

	std::string uri;
	uri = mCameraManagerConfig.getAddress();

	misReconnect = true;
	mWebSocket.Disconnect();
	misReconnect = false;

	int ret = mWebSocket.Connect(uri.c_str());
	if (ret < 0) {
		Log.v("Reconnect error RET_ERROR_NO_CLOUD_CONNECTION");
		ret = RET_ERROR_NO_CLOUD_CONNECTION;
		if (mCallback) {
			mCallback->onClosed(RET_ERROR_NO_CLOUD_CONNECTION, "");
		}
	}

	Log.v("<=Reconnect ret=%d", ret);

	return ret;
}

int CameraManager::Close()
{
	Log.v("=>Close");

	mWebSocket.Disconnect();

	Log.v("<=Close");
	return 0;
}


void CameraManager::onConnected()
{
	Log.v("<=onConnected");
	send_cmd_register();
}

void CameraManager::onDisconnected()
{
	Log.v("<=onDisconnected %d",misReconnect);
	if (misReconnect)
		return;

	if (mCallback)
		mCallback->onClosed(-1, mByeReason);
}

void CameraManager::onReceive(std::string data)
{
//	Log.i("=>onReceive Receive message: %s", data.c_str());
//	Log.i("=>onReceive");

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
	else if (!strncmp(scmd, CameraManagerCommands::RAW_MESSAGE, strlen(scmd))) {
		recv_cmd_RAW_MESSAGE(data);
	}
	else if (!strncmp(scmd, CameraManagerCommands::CAM_GET_LOG, strlen(scmd))) {
		recv_cmd_CAM_GET_LOG(data);
	}
	else {
		Log.e("<=onReceive unhandled cmd=%s", scmd);
	}

	json_decref(jdata);

//	Log.i("<=onReceive Receive message: %s", data.c_str());
//	Log.i("<=onReceive");

}

void CameraManager::on_update_cam_config()
{

	Log.v("%s", __FUNCTION__);

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

	Log.v("send_cmd_register()=%s", scmd.c_str());

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
	json_object_set_new(jcmd, CameraManagerParams::VERSION, json_string(mCameraManagerConfig.getCameraVersion().c_str()));
	json_object_set_new(jcmd, CameraManagerParams::RAW_MESSAGING, json_true());


	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(jcmd);

	Log.v("send_cmd_cam_register()=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}


int CameraManager::send_cmd_done(long long cmd_id, std::string cmd, std::string status)
{

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

	Log.v("send_cmd_done()=%s", scmd.c_str());

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

	Log.v("%s", __FUNCTION__);

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


	if(camEvent.event == "motion")
	{
		json_t *motion_info = json_object();
		json_object_set(jcmd, "motion_info", motion_info);

		json_t *snapshot_info = json_object();
		json_object_set_new(snapshot_info, "image_time", json_string(GetStringFromTime(camEvent.timeUTC)));
		json_object_set_new(snapshot_info, "size", json_integer(camEvent.snapshot_size));
		json_object_set_new(snapshot_info, "width", json_integer(camEvent.snapshot_width));
		json_object_set_new(snapshot_info, "height", json_integer(camEvent.snapshot_height));
		json_object_set(jcmd, "snapshot_info", snapshot_info);
		json_decref(motion_info);
		json_decref(snapshot_info);
	}

	json_object_set_new(jcmd, CameraManagerParams::TIME, json_real(camEvent.timeUTC));

//	json_object_set_new(jcmd, CameraManagerParams::TIME, json_integer(camEvent.timeUTC));
//	if (!camEvent.status.empty()) json_object_set_new(jcmd, CameraManagerParams::STATUS, json_string(camEvent.status.c_str()));
//	json_object_set_new(jcmd, CameraManagerParams::STATUS, json_string("OK"));

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(jcmd);

//	Log.v("send_cam_event()=%s", scmd.c_str());

	int err = mWebSocket.WriteBack(scmd.c_str());

	if(err<=0)
		return err;

	return idx;	
}

int CameraManager::recv_cmd_HELLO(std::string data)
{
	Log.v("recv_cmd_HELLO %s", data.c_str());

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
	Log.v("recv_cmd_CAM_HELLO %s", data.c_str());

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
		if(mCallback)
			mCallback->onPrepared();
	}

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_STREAM_START(std::string data)
{

	Log.v("recv_cmd_STREAM_START %s", data.c_str());

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
		mCallback->onStreamStart();

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_STREAM_STOP(std::string data)
{
	Log.v("recv_cmd_STREAM_STOP %s", data.c_str());

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

	DWORD th_id = GetCurrentThreadId();

	if (calling_thread_id && (calling_thread_id == th_id))
	{
		Log.v("recv_cmd_STREAM_STOP same thread");
		//return 0;
	}

	calling_thread_id = th_id;

	if (mCallback)
		mCallback->onStreamStop();

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_BYE(std::string data)
{
	Log.v("recv_cmd_BYE %s", data.c_str());

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	long long cmd_id = json_integer_value(json_object_get(jdata, CameraManagerParams::MSGID));
	const char *reason = json_string_value(json_object_get(jdata, CameraManagerParams::REASON));

	if (reason == NULL){
		json_decref(jdata);
		return -2;
	}

	mByeReason = reason;

	if (!strncmp(reason, CameraManagerByeReasons::RECONNECT, strlen(reason))) {
		Reconnect();
	}

	json_decref(jdata);

	return 0;
}

int CameraManager::recv_cmd_CAM_UPDATE_PREVIEW(std::string data)
{
	Log.v("recv_cmd_CAM_UPDATE_PREVIEW %s", data.c_str());

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
	Log.v("recv_cmd_CONFIGURE %s", data.c_str());

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
	Log.v("recv_cmd_GET_AUDIO_DETECTION %s", data.c_str());

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

	Log.v("recv_cmd_GET_CAM_AUDIO_CONF %s", data.c_str());

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
	json_object_set_new(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_AUDIO_CONF));
	json_object_set_new(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set_new(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set_new(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	//audio caps
	json_t *aud_caps = json_object();
	json_object_set_new(aud_caps, CameraManagerParams::MIC, json_false());
	json_object_set_new(aud_caps, CameraManagerParams::SPKR, json_false());
	json_object_set_new(aud_caps, CameraManagerParams::BACKWARD, json_false());

	json_object_set(jcmd, CameraManagerParams::CAPS, aud_caps);

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(aud_caps);
	json_decref(jcmd);
	json_decref(jdata);

	Log.v("recv_cmd_GET_CAM_AUDIO_CONF send=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}

int CameraManager::recv_cmd_GET_CAM_EVENTS(std::string data)
{
	//{"cmd":"cam_events_conf", "refid" : 12, "orig_cmd" : "get_cam_events", "cam_id" : 164614, "enabled" : true, "events" : [{"caps":{"stream":true, "snapshot" : false}, "event" : "record", "active" : true, "stream" : true, "snapshot" : false}, { "caps":{"stream":true,"snapshot" : false},"event" : "memorycard","active" : true,"stream" : true,"snapshot" : false }]}

	Log.v("recv_cmd_GET_CAM_EVENTS %s", data.c_str());

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
	– “motion”  for motion detection events
	– “sound” for audio detection
	– “net” for the camera network status change
	– “record” CM informs server about necessity of changing of recording state
	– “memorycard” camera's memory-card status change
	– “wifi” status of camera's currently used Wi-Fi
	*/

	//actual set
	std::string sEvents[] = { "motion", "sound", "record" };

	std::string *se = sEvents;

	for (int i =0;i<3;i++,se++) { 

		//event
		json_t *jevent = json_object();

		//caps
		json_t *event_caps = json_object();
		json_object_set_new(event_caps, CameraManagerParams::STREAM, json_true());
		json_object_set_new(event_caps, CameraManagerParams::SNAPSHOT, json_true());//json_false());

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

	Log.v("recv_cmd_GET_CAM_EVENTS send=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}

int CameraManager::recv_cmd_GET_CAM_STATUS(std::string data)
{
	Log.v("recv_cmd_GET_CAM_STATUS %s", data.c_str());

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
	json_object_set_new(jcmd, CameraManagerParams::STATUS_LED, json_boolean(mCameraManagerConfig.getCameraStatusLed()));

	char* jstr = json_dumps(jcmd, 0);
	scmd = jstr;
	free(jstr);

	json_decref(jcmd);
	json_decref(jdata);

	Log.v("recv_cmd_GET_CAM_STATUS send=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}

int CameraManager::recv_cmd_GET_CAM_VIDEO_CONF(std::string data)
{
	return -1;
}

int CameraManager::recv_cmd_DIRECT_UPLOAD_URL(std::string data)
{

//{"status": "OK", "url": "http://skyvr-av-auth2-rel.s3.amazonaws.com/u17/m176002/c175585/20190114/160855_u.jpg?Signature=U2oI%2Bj8K9jXzZilvB0O8nZWX6Qs%3D&Expires=1547483935&AWSAccessKeyId=ASIAWEOIJQJ53DRT63Z6&x-amz-security-token=FQoGZXIvYXdzELH//////////wEaDBvPZS9wCIhAmboNnSK3A6x5hWS/rDCUigMfUS/rDlihobgVN%2BVNhzuiPlBSyXOJrDyqMIq97/AmQX1aguFrdd4FH2RJv%2Bvjevl82zQK7ARIYP6DaVC/Hju1zlHq2NullwNJflGFHhrYXHRvs47A2fW00ihXS92K5WN7xSNWuOfBNRZ3pX/5YeNZMyN5H/L1WFPqcV3zNUeVUnrquTA0fxh6abMhuE0zInXLWc1ZkeMiIZCS5rlue72SvdHTIHo3gPR%2BYqRw7PXAWryWyQ%2BHCltE%2Bg2Q92QOzTKR4DY/XgOIAyWJde5rGQ6fZBqwQYq1roP0QarFw/7qIrAcQ37y9RLHKGNfTbqqYptDilUeLl5gUyTej4KfMTIlQ75T%2BqW5L6S9e1ExnhUP/1SMsQUaiU7mOSH7TVjpMdy7JNqSVLyASKWpcEVHrnQkiN7erEjrMkMt5mxVrF8QVdNNpb1fPyzBxMRPNAD9hQ%2BKHnROqa99yfeFsHMiGkiFDH6tkuKuCLp3Hfh86LGVjLMPmhIHQTLKGPljb%2BDDzn3Ft8QizHAOFmyAT7O5VAmiy/iwfbqLpKnxLiYTLpQOrzD9nh8wm6EQZKhAoe0ou8zy4QU%3D", 
//"msgid": 19, "cmd": "direct_upload_url", 
//"headers": {"Content-Length": "65535", "Content-Type": "image/jpeg"}, 
//"expire": "20190114T163855.961", "cam_id": 175585, "refid": 4}

//	Log.v("recv_cmd_DIRECT_UPLOAD_URL %s", data.c_str());
	Log.v("%s",__FUNCTION__);

	json_error_t err;
	json_t *jdata = json_loads(data.c_str(), 0, &err);
	if (!jdata) {
		return -1;
	}

	const char *status  = json_string_value(json_object_get(jdata, "status"));
	if(!(status && !strcmp(status,"OK")))
	{
		json_decref(jdata);
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
//	Log.v("url=%s, refid=%d", url.c_str(), refid);

	if (mCallback)
		mCallback->onRecvUploadUrl(url, refid);

	json_decref(jdata);

	send_cmd_done(cmd_id, cmd, CameraManagerDoneStatus::OK);
	return 0;
}

int CameraManager::recv_cmd_GET_MOTION_DETECTION(std::string data)
{
	return -1;
}

int CameraManager::recv_cmd_GET_STREAM_BY_EVENT(std::string data)
{
	return -1;
}

int CameraManager::recv_cmd_GET_STREAM_CAPS(std::string data)
{
	return -1;
}

int CameraManager::recv_cmd_GET_STREAM_CONFIG(std::string data)
{
	return -1;
}

int CameraManager::recv_cmd_GET_SUPPORTED_STREAMS(std::string data)
{
	Log.v("recv_cmd_GET_SUPPORTED_STREAMS %s", data.c_str());

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

	//video
	json_t *video = json_array();
	json_array_append_new(video, json_string("Aud"));
	json_object_set(jcmd, CameraManagerParams::AUDIO_ES, video);

	//audio
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

	Log.v("recv_cmd_GET_SUPPORTED_STREAMS send=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());

}

int CameraManager::recv_cmd_SET_CAM_AUDIO_CONF(std::string data)
{
	return -1;
}

int CameraManager::recv_cmd_SET_CAM_EVENTS(std::string data)
{
	Log.v("recv_cmd_SET_CAM_EVENTS %s", data.c_str());

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

int CameraManager::recv_cmd_SET_CAM_VIDEO_CONF(std::string data)
{
	Log.v("recv_cmd_SET_CAM_VIDEO_CONF %s", data.c_str());

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

int CameraManager::recv_cmd_SET_MOTION_DETECTION(std::string data)
{
	Log.v("recv_cmd_SET_MOTION_DETECTION %s", data.c_str());

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

int CameraManager::recv_cmd_SET_STREAM_CONFIG(std::string data)
{
	return -1;
}

int CameraManager::recv_cmd_GET_PTZ_CONF(std::string data)
{
	return -1;
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

	Log.v("%s", __FUNCTION__);

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
//	Log.v("confirm_direct_upload()=%s", scmd.c_str());
	return mWebSocket.WriteBack(scmd.c_str());
}

int CameraManager::recv_cmd_CAM_GET_LOG(std::string data)
{

	Log.v("%s %s", __FUNCTION__, data.c_str());

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

	std::string time_utc = GetStringFromTime(time(NULL) + m_nCamTZ);
	std::string url = "http://" + mCameraManagerConfig.getUploadURL() + "/" + mCameraManagerConfig.getCamPath() + "?sid=" + mCameraManagerConfig.getSID() + "&cat=log&type=txt&start=" + time_utc;

//https://52.23.188.59:8000/upload/u17m175852c175435_?sid=44uQeOY70ZZ7&cat=log&type=txt&start=20190129T090033.688

	Log.v("recv_cmd_CAM_GET_LOG url=%s", url.c_str());

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

//<=Camera Manager comands
