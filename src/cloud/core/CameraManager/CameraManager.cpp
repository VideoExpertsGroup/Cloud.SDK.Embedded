
#include "CameraManager.h"
#include "CameraManagerCommands.h"
#include "CameraManagerParams.h"
#include "CameraManagerDoneStatus.h"
#include "CameraManagerByeReasons.h"
#include <jansson.h>

using namespace std;

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
			p->Log.v("WEBSOCKWRAP_RECEIVE %s", szIn);
			p->onReceive(szIn);
			delete[] szIn;
		}
		break;
	case WEBSOCKWRAP_ERROR:
		p->Log.v("WEBSOCKWRAP_ERROR");
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
	: Log(TAG, LOG_LEVEL)
	  ,mWebSocket(OnEvent, this)
{
	misPrepared = false;
	misCamRegistered = false;
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

	mWebSocket.Connect(uri.c_str());

	Log.v("<=Open");

	return 0;
}

int CameraManager::Reconnect()
{
	Log.v("=>Reconnect");

	std::string uri;
	uri = mCameraManagerConfig.getAddress();

	misReconnect = true;
	mWebSocket.Disconnect();
	misReconnect = false;

	mWebSocket.Connect(uri.c_str());

	Log.v("<=Reconnect");

	return 0;
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
	send_cmd_register();
}

void CameraManager::onDisconnected()
{
	if (misReconnect)
		return;

	if (mCallback)
		mCallback->onClosed(-1, mByeReason);
}

void CameraManager::onReceive(std::string data)
{
	Log.i("=>onReceive Receive message: %s", data.c_str());

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
	else {
		Log.e("<=onReceive unhandled cmd=%s", scmd);
	}

	json_decref(jdata);

	Log.i("<=onReceive Receive message: %s", data.c_str());

}

void CameraManager::on_update_cam_config()
{
	json_t *jo = json_object();

	if(!mCameraManagerConfig.getUUID().empty())
		json_object_set(jo, CameraManagerParams::UUID, json_string(mCameraManagerConfig.getUUID().c_str()));
	if (!mCameraManagerConfig.getSID().empty())
		json_object_set(jo, CameraManagerParams::SID, json_string(mCameraManagerConfig.getSID().c_str()));
	if (!mCameraManagerConfig.getPwd().empty())
		json_object_set(jo, CameraManagerParams::PWD, json_string(mCameraManagerConfig.getPwd().c_str()));
	if (!mCameraManagerConfig.getConnID().empty())
		json_object_set(jo, CameraManagerParams::CONNID, json_string(mCameraManagerConfig.getConnID().c_str()));
	json_object_set(jo, CameraManagerParams::CAM_ID, json_integer(mCameraManagerConfig.getCamID()));

	mCameraConfig = json_dumps(jo, 0);
	json_decref(jo);

	if (mCallback)
		mCallback->onUpdateConfig(mCameraManagerConfig);
}


//=>Camera Manager comands
int CameraManager::send_cmd_register()
{
	string scmd;

	json_t *jcmd = json_object();
	json_object_set(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::REGISTER));
	json_object_set(jcmd, CameraManagerParams::MSGID, json_integer(messageId++));
	json_object_set(jcmd, CameraManagerParams::VER, json_string(mCameraManagerConfig.getCMVersion().c_str()));
	json_object_set(jcmd, CameraManagerParams::TZ, json_string(mCameraManagerConfig.getCameraTimezone().c_str()));
	json_object_set(jcmd, CameraManagerParams::VENDOR, json_string(mCameraManagerConfig.getCameraVendor().c_str()));
	if (!mCameraManagerConfig.getPwd().empty()) {
		json_object_set(jcmd, CameraManagerParams::PWD, json_string(mCameraManagerConfig.getPwd().c_str()));
	}
	if (!mCameraManagerConfig.getSID().empty()) {
		json_object_set(jcmd, CameraManagerParams::PREV_SID, json_string(mCameraManagerConfig.getSID().c_str()));
	}
	if (!mCameraManagerConfig.getRegToken().isEmpty()) {
		json_object_set(jcmd, CameraManagerParams::REG_TOKEN, json_string(mCameraManagerConfig.getRegToken().getToken().c_str()));
	}

	scmd = json_dumps(jcmd, 0);
	json_decref(jcmd);

	Log.v("send_cmd_register()=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());
}

int CameraManager::send_cmd_cam_register() {
	string scmd;

	json_t *jcmd = json_object();
	json_object_set(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_REGISTER));
	json_object_set(jcmd, CameraManagerParams::MSGID, json_integer(messageId++));

	json_object_set(jcmd, CameraManagerParams::IP, json_string(mCameraManagerConfig.getCameraIPAddress().c_str()));
	json_object_set(jcmd, CameraManagerParams::UUID, json_string(mCameraManagerConfig.getUUID().c_str()));
	json_object_set(jcmd, CameraManagerParams::BRAND, json_string(mCameraManagerConfig.getCameraBrand().c_str()));
	json_object_set(jcmd, CameraManagerParams::MODEL, json_string(mCameraManagerConfig.getCameraModel().c_str()));
	json_object_set(jcmd, CameraManagerParams::SN, json_string(mCameraManagerConfig.getCameraSerialNumber().c_str()));
	json_object_set(jcmd, CameraManagerParams::VERSION, json_string(mCameraManagerConfig.getCameraVersion().c_str()));

	scmd = json_dumps(jcmd, 0);
	json_decref(jcmd);

	Log.v("send_cmd_cam_register()=%s", scmd.c_str());

	return mWebSocket.WriteBack(scmd.c_str());
}


int CameraManager::send_cmd_done(long long cmd_id, std::string cmd, std::string status)
{
	string scmd;

	json_t *jcmd = json_object();
	json_object_set(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::DONE));
	json_object_set(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd.c_str()));
	json_object_set(jcmd, CameraManagerParams::STATUS, json_string(status.c_str()));

	scmd = json_dumps(jcmd, 0);
	json_decref(jcmd);

	Log.v("send_cmd_done()=%s", scmd.c_str());
	return mWebSocket.WriteBack(scmd.c_str());
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
	json_object_set(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_AUDIO_CONF));
	json_object_set(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	//audio caps
	json_t *aud_caps = json_object();
	json_object_set(aud_caps, CameraManagerParams::MIC, json_false());
	json_object_set(aud_caps, CameraManagerParams::SPKR, json_false());
	json_object_set(aud_caps, CameraManagerParams::BACKWARD, json_false());

	json_object_set(jcmd, CameraManagerParams::CAPS, aud_caps);

	scmd = json_dumps(jcmd, 0);
	json_decref(aud_caps);
	json_decref(jcmd);
	json_decref(jdata);

	Log.v("recv_cmd_GET_CAM_AUDIO_CONF send=%s", scmd.c_str());
	return mWebSocket.WriteBack(scmd.c_str());
}

int CameraManager::recv_cmd_GET_CAM_EVENTS(std::string data)
{
	return -1;
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
	json_object_set(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::CAM_STATUS));
	json_object_set(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	//camera status
	json_object_set(jcmd, CameraManagerParams::IP, json_string(mCameraManagerConfig.getCameraIPAddress().c_str()));
	json_object_set(jcmd, CameraManagerParams::ACTIVITY, json_true() /*json_boolean(mCameraManagerConfig.getCameraActivity())*/);
	json_object_set(jcmd, CameraManagerParams::STREAMING, json_boolean(mCameraManagerConfig.getCameraStreaming()));
	json_object_set(jcmd, CameraManagerParams::STATUS_LED, json_boolean(mCameraManagerConfig.getCameraStatusLed()));


	scmd = json_dumps(jcmd, 0);
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
	return -1;
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
	json_object_set(jcmd, CameraManagerParams::CMD, json_string(CameraManagerCommands::SUPPORTED_STREAMS));
	json_object_set(jcmd, CameraManagerParams::REFID, json_integer(cmd_id));
	json_object_set(jcmd, CameraManagerParams::ORIG_CMD, json_string(cmd));
	json_object_set(jcmd, CameraManagerParams::CAM_ID, json_integer(cam_id));

	//video
	json_t *video = json_array();
	json_array_append(video, json_string("Aud"));
	json_object_set(jcmd, CameraManagerParams::AUDIO_ES, video);

	//audio
	json_t *audio = json_array();
	json_array_append(audio, json_string("Vid"));
	json_object_set(jcmd, CameraManagerParams::VIDEO_ES, audio);

	//stream
	json_t *stream1 = json_object();
	json_object_set(stream1, CameraManagerParams::ID, json_string("Main")); // TODO hardcoded
	json_object_set(stream1, CameraManagerParams::VIDEO, json_string("Vid")); // TODO hardcoded
	json_object_set(stream1, CameraManagerParams::AUDIO, json_string("Aud")); // TODO hardcoded
	json_t *stream = json_array();
	json_array_append(stream, stream1);

	json_object_set(jcmd, CameraManagerParams::STREAMS, stream);

	scmd = json_dumps(jcmd, 0);
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


//<=Camera Manager comands
