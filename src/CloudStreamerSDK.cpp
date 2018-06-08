
#include "CloudStreamerSDK.h"
#include "utils/base64.h"

#include <jansson.h>

CloudStreamerSDK::CloudStreamerSDK(ICloudStreamerCallback *callback)
	:Log(TAG, LOG_LEVEL)
{
	Log.v("=>CloudStreamerSDK");
	mCallback = callback;

	mCamid = -1;
	m_RefConnected = 0;
}

CloudStreamerSDK::~CloudStreamerSDK()
{
	Log.v("<=CloudStreamerSDK");
}

int CloudStreamerSDK::setSource(std::string &channel)
{

	//decode channel
	int out_len = channel.length() * 2;
	char *out_channel = new char[out_len];
	out_len = Decode64_2(out_channel, out_len, channel.c_str(), channel.length());
	out_channel[out_len] = 0;

	json_error_t err;
	json_t *root = json_loads(out_channel, 0, &err);
	delete out_channel;

	mToken = json_string_value(json_object_get(root, "token"));
	mCamid = json_integer_value(json_object_get(root, "camid"));
	const char *p = json_string_value(json_object_get(root, "svcp"));
	if (p) {
		mSvcpUrl = p;
	}

	json_decref(root);

	//open connection
	if (mSvcpUrl.empty()) {
		mConnection.openSync(mToken);
	}
	else {
		mConnection.openSync(mToken, mSvcpUrl);
	}

	//get camera params
	/*CloudAPI api = mConnection._getCloudAPI();
	pair<int, string> cam_res = api.getCamera(mCamid);
	if (cam_res.first != 200) {
		if (mCallback) {
			mCallback->onError(RET_ERROR_NOT_AUTHORIZED);
		}
		return RET_ERROR_NOT_AUTHORIZED;
	}*/

	//CloudRegToken cloudToken( std::string ( "{token"+ mToken+"}"));
	//ws://cam.skyvr.videoexpertsgroup.com:8888/ctl/NEW/Sx6jGf/

	//in near future it gotta been mToken
	CloudRegToken cloudToken(std::string("{\"token\" : \""+ mToken+"\"}"));
	mCameraManagerConfig.setRegToken(cloudToken);

	if (mCallback) {
		mCallback->onCameraConnected();
	}

	return 0;
}

int CloudStreamerSDK::Start()
{
	prepareCM();
	return 0;
}

int CloudStreamerSDK::Stop()
{
	closeCM();
	return 0;
}


//=>streamer CM
int CloudStreamerSDK::prepareCM()
{
	Log.v("=>prepareCM");

	mCameraManager.Open(mCameraManagerConfig, this);

	Log.v("<=prepareCM");
	return 0;
}

int CloudStreamerSDK::closeCM()
{
	Log.v("=>closeCM");

	mCameraManager.Close();

	Log.v("<=closeCM");
	return 0;
}
//<=

//=>ICameraManagerCallback
void CloudStreamerSDK::onPrepared()
{
	Log.v("=onPrepared url=%s", mCameraManager.getStreamUrl().c_str());
	m_RefConnected = 0;
}

void CloudStreamerSDK::onClosed(int error, std::string reason)
{
	Log.v("=onClosed %d:%s", error, reason.c_str());
	if (mCallback)
		mCallback->onError(error);
}

void CloudStreamerSDK::onUpdateConfig(CameraManagerConfig &config)
{
	Log.v("=onUpdateConfig");
	mCameraManagerConfig = config;
}

void CloudStreamerSDK::onStreamStart()
{
	Log.v("=onStreamStart");

	long cnt = InterlockedIncrement(&m_RefConnected);

	if (mCallback && cnt == 1)
		mCallback->onStarted(mCameraManager.getStreamUrl());
}

void CloudStreamerSDK::onStreamStop()
{
	Log.v("=onStreamStop");

	long cnt = InterlockedDecrement(&m_RefConnected);

	if (mCallback && cnt == 0)
		mCallback->onStopped();
}

void CloudStreamerSDK::onCommand(std::string cmd)
{
	Log.v("=onCommand %s", cmd.c_str());
}

void CloudStreamerSDK::onUpdatePreview()
{
	Log.v("=onUpdatePreview");
}

//<=ICameraManagerCallback
