
#include "CloudStreamerSDK.h"
#include "utils/base64.h"
#include "utils/utils.h"

#include <jansson.h>

extern int json_try_get_int(json_t* jdata, const char* param);

CloudStreamerSDK::CloudStreamerSDK(ICloudStreamerCallback *callback)
	:Log("CloudStreamerSDK", 2)
{
	Log.d("=>CloudStreamerSDK");
	mCallback = callback;

	mCamid = -1;
	m_RefConnected = 0;
	m_bStarted = false;
	snap_handler = NULL;
	callback->parent = this;
}

CloudStreamerSDK::~CloudStreamerSDK()
{
	Log.d("<=CloudStreamerSDK");
}

void CloudStreamerSDK::setVersionOverride(string ver)
{
	mCameraManagerConfig.setVersionOverride(ver);
}

int CloudStreamerSDK::setSource(std::string& channel)
{

	//decode channel
	int out_len = channel.length() * 2;
	char* out_channel = new char[out_len];
	out_len = Decode64_2(out_channel, out_len, channel.c_str(), channel.length());

	if (out_len <= 0)
	{
		Log.d("Decode64_2 failed");
		return -1;
	}

	out_channel[out_len] = 0;

	json_error_t err;
	json_t* root = json_loads(out_channel, 0, &err);
	Log.d("=setSource %s", out_channel);
	delete[] out_channel;

	mToken = json_string_value(json_object_get(root, "token"));
	mCamid = json_integer_value(json_object_get(root, "camid"));

	{

		long long svcpPort = 8888;
		string proto = "http://";
		char szEndPoint[512] = { 0 };

		const char* host = json_string_value(json_object_get(root, "cam"));

		// Added by request from CNVR-1855 "Camera app doesn't stream to local server"
		if (!host)
			host = json_string_value(json_object_get(root, "api"));
		//

		if (!host)
		{
			MYGetPrivateProfileString(
				"EndPoint",
				"DefaultCamAddress",
				"cam.skyvr.videoexpertsgroup.com",
				szEndPoint,
				512,
				GetSettingsFile()
			);
			Log.d("%s EndPoint=%s", __FUNCTION__, szEndPoint);
			host = szEndPoint;
		}

		if (host) {
#if (WITH_OPENSSL || WITH_MBEDTLS)
			if (CertFileExist() && !SSL_Disabled())
			{
				int port = json_try_get_int(root, "cam_sp");
				if (port > 0)
					svcpPort = port;
				else
					svcpPort = 8883;

				proto = "https://";
			}
			else
#endif
			{
				int port = json_try_get_int(root, "cam_p");
				if (port > 0)
					svcpPort = port;
				else
					svcpPort = 8888;

				proto = "http://";
			}
		}

		if (host)
			mSvcpUrl = proto + std::string(host) + ":" + fto_string(svcpPort);
		else
		{
			const char* p = json_string_value(json_object_get(root, "svcp"));
			if (p) {
				Uri uri = Uri::Parse(p);
				mSvcpUrl = "http://" + uri.Host + ":8888";
			}
		}
	}

	Log.d("mSvcpUrl = %s", mSvcpUrl.c_str());

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
	CloudRegToken cloudToken(std::string("{\"token\" : \"" + mToken + "\"}"));
	mCameraManagerConfig.setRegToken(cloudToken);
	mCameraManagerConfig.setCMAddress(mConnection._getCloudAPI().getCMAddress());

	if (mCallback) {
		mCallback->onCameraConnected();
	}

	return 0;
}

int CloudStreamerSDK::Start()
{
	int ret = prepareCM();
	return ret;
}

int CloudStreamerSDK::Stop()
{
	closeCM();
	return 0;
}

int CloudStreamerSDK::sendCamEvent(const CameraEvent &camEvent)
{
	return mCameraManager.send_cam_event(camEvent);
}

int CloudStreamerSDK::GetDirectUploadUrl(unsigned long timeUTC, const char* type, const char* category, int size, int duration, int width, int height)
{
	return mCameraManager.get_direct_upload_url(timeUTC, type, category, size, duration, width, height);
}

int CloudStreamerSDK::CloudPing()
{
	return mCameraManager.CloudPing();
}

time_t CloudStreamerSDK::CloudPong()
{
	return mCameraManager.CloudPong();
}

//=>streamer CM
int CloudStreamerSDK::prepareCM()
{
	Log.d("=>prepareCM");

	int ret = mCameraManager.Open(mCameraManagerConfig, this);

	Log.d("<=prepareCM");
	return ret;
}

int CloudStreamerSDK::closeCM()
{
	Log.d("=>closeCM");

	mCameraManager.Close();

	Log.d("<=closeCM");
	return 0;
}
//<=

//=>ICameraManagerCallback
void CloudStreamerSDK::onPrepared()
{
	Log.d("=onPrepared url=%s", mCameraManager.getStreamUrl().c_str());
	m_RefConnected = 0;
	m_bStarted = false;
}

void CloudStreamerSDK::onClosed(int error, std::string reason)
{
	int rlen = reason.length();

	if (-1 == error)
	{
		if(rlen > 0)
			error = reason_str_to_int(reason);
		else 
			error = 0xDDDD;
	}

	if (RET_ERROR_NO_CLOUD_CONNECTION == error)
	{
		Log.d("%s %s", __FUNCTION__, "RET_ERROR_NO_CLOUD_CONNECTION");
	}
	else
	{
		if (rlen > 0)
			Log.d("%s %d %d:%s %p", __FUNCTION__, rlen, error, reason.c_str(), mCallback);
		else
			Log.d("%s %d %d %p", __FUNCTION__, rlen, error, mCallback);
	}

	if (mCallback)
		mCallback->onError(error);
//	if (mCallback)
//		mCallback->onClosed(error);

}

void CloudStreamerSDK::onUpdateConfig(CameraManagerConfig &config)
{
	Log.d("=onUpdateConfig");

	if (mCallback)
	if(*mCameraManagerConfig.getStreamConfig() != *config.getStreamConfig())
	{
		mCallback->setStreamConfig(config);
	}
	mCameraManagerConfig = config;
}

void CloudStreamerSDK::onStreamStart()
{
	long cnt = InterlockedIncrement(&m_RefConnected);

	Log.d("=>onStreamStart cnt=%d, m_bStarted=%d", cnt, m_bStarted);

	long long pub_sid = mCameraManager.getPublishSID();
	string strUrl = mCameraManager.getStreamUrl();

	//Log.d("pub_sid=%lld", pub_sid);

	if (-1 != pub_sid)
	{
		char szpsid[32];
		sprintf(szpsid, "&psid=%lld", pub_sid);
		strUrl += szpsid;
		Log.d("strUrl=%s", strUrl.c_str());
	}

	//if (mCallback && cnt == 1) // Client will handle it because
	if (mCallback && !m_bStarted)
		mCallback->onStarted(strUrl);
	m_bStarted = true;
	Log.d("<=onStreamStart");
}

void CloudStreamerSDK::onStreamStop()
{
	long cnt = InterlockedDecrement(&m_RefConnected);
	Log.d("=onStreamStop cnt=%d, m_bStarted=%d", cnt, m_bStarted);

	//if (mCallback && cnt == 0)
	if(mCallback && m_bStarted)
		mCallback->onStopped();
	m_bStarted = false;

}

void CloudStreamerSDK::onCommand(std::string cmd, std::string &retVal)
{
	Log.d("=onCommand %s", cmd.c_str());
	if (mCallback)
		mCallback->onCommand(cmd, retVal);

}

void CloudStreamerSDK::onUpdatePreview(std::string url)
{
	Log.d("=onUpdatePreview");
	if (mCallback)
		mCallback->UpdatePreview(url);
}

int CloudStreamerSDK::onRawMessage(std::string& data)
{
	Log.e("=onRawMessage");

	//long cnt = InterlockedIncrement(&m_RefConnected);

	int ret = -1;

	if (mCallback)// && cnt == 1)
	{
		ret = mCallback->onRawMsg(data);
		Log.e("data = %s, ret=%d", data.c_str(), ret);
	}
	else
	{
		Log.e("=onRawMessage mCallback=%p", mCallback);// , cnt);
	}

	return ret;
}

void CloudStreamerSDK::onRecvUploadUrl(std::string url, int refid)
{
	Log.d("=onRecvUploadUrl");

	if (mCallback)
		mCallback->onUploadUrl(this, url, refid);
}

void CloudStreamerSDK::onSetLogEnable(bool bEnable)
{
	Log.d("=%s", __FUNCTION__);

	if (mCallback)
		mCallback->SetLogEnable(bEnable);
}

void CloudStreamerSDK::onSetActivity(bool bEnable)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->SetActivity(bEnable);
}

void CloudStreamerSDK::onSetImageParams(image_params_t* img)
{
	Log.d("=%s", __FUNCTION__);

	if (mCallback)
		mCallback->SetImageParams(img);
}

void CloudStreamerSDK::onGetImageParams(image_params_t* img)
{
	Log.d("=%s", __FUNCTION__);

	if (mCallback)
		mCallback->GetImageParams(img);
}

void CloudStreamerSDK::onSetMotionParams(motion_params_t* mpr)
{
	Log.d("=%s", __FUNCTION__);

	if (mCallback)
		mCallback->SetMotionParams(mpr);
}

void CloudStreamerSDK::onGetMotionParams(motion_params_t* mpr)
{
	Log.d("=%s", __FUNCTION__);

	if (mCallback)
		mCallback->GetMotionParams(mpr);
}

void CloudStreamerSDK::onGetEventLimits(time_t* pre, time_t* post)
{
	Log.d("=%s", __FUNCTION__);

	if (mCallback)
		mCallback->GetEventLimits(pre, post);
}

void CloudStreamerSDK::onSetEventLimits(time_t pre, time_t post)
{
	Log.d("=%s", __FUNCTION__);

	if (mCallback)
		mCallback->SetEventLimits(pre, post);
}

void CloudStreamerSDK::onSetTimeZone(const char* time_zone_str)
{
	Log.d("=%s", __FUNCTION__);

	if (mCallback)
		mCallback->SetTimeZone(time_zone_str);
}

void CloudStreamerSDK::onGetTimeZone(char* time_zone_str)
{
	Log.d("=%s", __FUNCTION__);

	if (mCallback)
		mCallback->GetTimeZone(time_zone_str);
}

void CloudStreamerSDK::onGetPTZParams(ptz_caps_t* ptz)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->GetPTZParams(ptz);
}

void CloudStreamerSDK::onSendPTZCommand(ptz_command_t* ptz)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->SendPTZCommand(ptz);
}

void CloudStreamerSDK::onGetAudioParams(audio_settings_t* audset)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->GetAudioParams(audset);
}

void CloudStreamerSDK::onSetAudioParams(audio_settings_t* audset)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->SetAudioParams(audset);
}

void CloudStreamerSDK::onGetOSDParams(osd_settings_t* osd)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->GetOSDParams(osd);
}

void CloudStreamerSDK::onSetOSDParams(osd_settings_t* osd)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->SetOSDParams(osd);
}

int CloudStreamerSDK::onSendCameraCommand(cam_command_t* ccmd)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		return mCallback->SendCameraCommand(ccmd);
	return -2;
}

void CloudStreamerSDK::onTriggerEvent(string evt, string meta)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->TriggerEvent(this, evt, meta);
}

void CloudStreamerSDK::onStartBackward(string url)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->StartBackward(url);
}

void CloudStreamerSDK::onStopBackward(string url)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->StopBackward(url);
}

//<=ICameraManagerCallback

void CloudStreamerSDK::SetUserParam(void* ptr)
{
	snap_handler = ptr;
}

void* CloudStreamerSDK::GetUserParam()
{
	return snap_handler;
}

int CloudStreamerSDK::ConfirmUpload(std::string url)
{
	return mCameraManager.confirm_direct_upload(url);
}

void CloudStreamerSDK::onGetLog(std::string url)
{
	Log.d("=onGetLog %s", url.c_str());
	if (mCallback)
		mCallback->onCamGetLog(url);
}

int CloudStreamerSDK::SetCamTimeZone(int tz)
{
	return mCameraManager.set_cam_tz(tz);
}

int CloudStreamerSDK::GetCamTimeZone()
{
	return mCameraManager.get_cam_tz();
}

time_t CloudStreamerSDK::GetCloudTime()
{
	if (mCallback)
		return mCallback->onGetCloudTime();
	return 0;
}

void* CloudStreamerSDK::GetCallbackClassPtr()
{
	return mCallback;
}

void CloudStreamerSDK::onSetByEventMode(bool bMode)
{
	Log.d("%s %d", __FUNCTION__, bMode);
	if (mCallback)
		mCallback->onSetRecByEventsMode(bMode);
}

void CloudStreamerSDK::onSetPeriodicEvents(const char* name, int period, bool active)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->SetPeriodicEvents(name, period, active);
}

void CloudStreamerSDK::onGetWiFiList(wifi_list_t* wifilist)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->GetWiFiList(wifilist);
}

void CloudStreamerSDK::onSetCurrenWiFi(wifi_params* params)
{
	Log.d("=%s", __FUNCTION__);
	if (mCallback)
		mCallback->SetCurrenWiFi(params);
}

