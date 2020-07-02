#ifndef __CAMERAMANAGERCONFIG_H__
#define __CAMERAMANAGERCONFIG_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"
#include "../../../cloud/core/Uri.h"
#include "../CloudCommon/CloudRegToken.h"
#include "StreamConfig.h"

using namespace std;

class CameraManagerConfig : public CUnk {
//	const char *TAG;
//	const int LOG_LEVEL;
	MLog Log;

	string mUploadUrl;
	string mMediaServer;
	string mCamPath;
	string mCA;// TODO
	string mSID;
	string mPwd;
	string mUUID;
	string mConnID;
	long long mCamID;
	CloudRegToken mRegToken;

	// some camera details
	bool mCameraActivity;
	bool mCameraStreaming;
	bool mCameraStatusLed;
	string mCameraIPAddress;
	string mCMVersion;
	string mCameraBrand;
	string mCameraModel;
	string mCameraSerialNumber;
	string mCameraVersion;
	string mCameraTimezone;
	string mCameraVendor;
	StreamConfig *mStreamConfig;

	// WS(S) - 8888(8883)
	// default parameters
	string PROTOCOL;
	string DEFAULT_ADDRESS;
	string mAddress;
	string mOvrVersion;
	int PORT;
	bool m_bSSL_Disabled;

	string mReconnectAddress;

public:
	virtual ~CameraManagerConfig() {
		if (mStreamConfig)
			mStreamConfig->Release();
	};

	CameraManagerConfig()
		: Log("CameraManagerConfig", 2)
	{
		mCameraIPAddress = "127.0.0.1";//CameraManagerHelper.getLocalIpAddress();
		mCameraBrand = "Camera Brand";//Build.BRAND;
		mCameraModel = "Camera Model"; //Build.MODEL;
		mCameraSerialNumber = "Camera Serial"; //Build.SERIAL;

		//Do not change mCameraVersion value manually! It overwrites by set_cm_ver.sh to value from the VERSION file.
		mCameraVersion = "1.0.142";

		mCMVersion = "Camera CM";
		mCameraVendor = "vendor";
		mCameraTimezone = "UTC";//TimeZone.getDefault().getID();
		mOvrVersion = "";

//		TAG = "CameraManagerConfig";
//		LOG_LEVEL = 2;
		mUploadUrl = "";
		mMediaServer = "";
		mCamPath = "";
		mCA = ""; // TODO
		mSID = "";
		mPwd = "";
		mUUID = "";
		mConnID = "";
		mCamID = 0;
		mRegToken;

		mCameraActivity = true;
		mCameraStreaming = true;
		mCameraStatusLed = false;
		mCameraIPAddress = "";
		mCameraBrand = "";
		mCameraModel = "";
		mCameraSerialNumber = "";
		mCameraTimezone = "";
		mCameraVendor = "";
		mStreamConfig = NULL;
		m_bSSL_Disabled = false;

#if (WITH_OPENSSL || WITH_MBEDTLS)
		PROTOCOL = "wss://";
		PORT = 8883;
//check for cert.pem
		if (!CertFileExist() || SSL_Disabled())
		{
			PROTOCOL = "ws://";
			PORT = 8888;
		}
#else
		PROTOCOL = "ws://";
		PORT = 8888;
#endif

		DEFAULT_ADDRESS = "cam.skyvr.videoexpertsgroup.com";

		if (1)
		{
			char szEndPoint[512] = { 0 };
			MYGetPrivateProfileString(
				"EndPoint",
				"DefaultCamAddress",
				"cam.skyvr.videoexpertsgroup.com",
				szEndPoint,
				512,
				GetSettingsFile()
			);
			Log.d("%s EndPoint=%s", __FUNCTION__, szEndPoint);
			DEFAULT_ADDRESS = szEndPoint;
		}

		mAddress = DEFAULT_ADDRESS;
		mReconnectAddress = "";

		Log.d("CameraManagerConfig constructor mReconnectAddress empty");

		configureStreamConfig();
	}

	CameraManagerConfig &operator=(const CameraManagerConfig &src)
	{
		mUploadUrl = src.mUploadUrl;
		mMediaServer = src.mMediaServer;
		mCamPath = src.mCamPath;
		mCA = src.mCA; // TODO
		mSID = src.mSID;
		mPwd = src.mPwd;
		mUUID = src.mUUID;
		mConnID = src.mConnID;
		mCamID = src.mCamID;
		mRegToken = src.mRegToken;

		// some camera details
		mCameraActivity = src.mCameraActivity;
		mCameraStreaming = src.mCameraStreaming;
		mCameraStatusLed = src.mCameraStatusLed;
		mCameraIPAddress = src.mCameraIPAddress;
		mCMVersion = src.mCMVersion;
		mCameraBrand = src.mCameraBrand;
		mCameraModel = src.mCameraModel;
		mCameraSerialNumber = src.mCameraSerialNumber;
		mCameraVersion = src.mCameraVersion;
		mCameraTimezone = src.mCameraTimezone;
		mCameraVendor = src.mCameraVendor;
		mOvrVersion = src.mOvrVersion;

		if (mStreamConfig)
			mStreamConfig->Release();
		mStreamConfig = src.mStreamConfig;
		if (mStreamConfig)
			mStreamConfig->AddRef();

		// WS(S) - 8888(8883)
		// default parameters
		PROTOCOL = src.PROTOCOL;
		PORT = src.PORT;

		if (m_bSSL_Disabled)
			setSSLDisable(true);

		DEFAULT_ADDRESS = src.DEFAULT_ADDRESS;
		mAddress = src.mAddress;

		mReconnectAddress = src.mReconnectAddress;

		if(mReconnectAddress.empty())
			Log.d("&operator= mReconnectAddress empty");
		else
			Log.d("&operator= mReconnectAddress=%s" , mReconnectAddress.c_str());

		return *this;
	}


    void setCMURL(string url) {
        if (url.empty()) {
            mAddress = DEFAULT_ADDRESS;
            PORT = 8888;
        }
        else {
            Uri uri = Uri::Parse(url);
            mAddress = uri.Host;
            //PORT = atoi(uri.Port.c_str());
            PORT = 8888;
        }

	if(url.empty())
		Log.d("setCMURL url empty");
	else
		Log.d("setCMURL url=%s", url.c_str());

    }

	void setVersionOverride(string ver)
	{
		if (!ver.empty()) {
			mOvrVersion = ver;
		}
	}

	string getVersionOverride()
	{
		return mOvrVersion;
	}

	void setCMAddress(string url) {
		if (url.empty()) {
			mAddress = DEFAULT_ADDRESS;
			Log.d("setCMAddress DEFAULT_ADDRESS=%s", DEFAULT_ADDRESS.c_str());
		}
		else {
			mAddress = url;
			Log.d("setCMAddress url=%s", url.c_str());
		}

		if(mAddress.empty())
			Log.d("setCMAddress mAddress empty");
		else
			Log.d("setCMAddress mAddress=%s", mAddress.c_str());

	}

	string getCMAddress() {

		if(mAddress.empty())
			Log.d("getCMAddress mAddress empty");
		else
			Log.d("getCMAddress mAddress=%s", mAddress.c_str());

		return mAddress;
	}

	void setUploadURL(string val) {
		mUploadUrl = val;
	}

	string getUploadURL() {
		return mUploadUrl;
	}

	void setMediaServer(string val) {
		mMediaServer = val;
	}

	string getMediaServer() {
		return mMediaServer;
	}

	void setCA(string val) { mCA = val; }

	void setSID(string val) {
		mSID = val;
	}

	string getSID() {
		return mSID;
	}

	void setUUID(string val) {
		mUUID = val;
	}

	string getUUID() {
		return mUUID;
	}

	void setConnID(string val) {
		mConnID = val;
	}

	string getConnID() {
		return mConnID;
	}

	void setPwd(string val) {
		mPwd = val;
	}

	string getPwd() {
		return mPwd;
	}

	void setCamPath(string val) {
		mCamPath = val;
	}

	string getCamPath() {
		return mCamPath;
	}

	void setCamID(long long val) {
		mCamID = val;
	}

	long long getCamID() {
		return mCamID;
	}

	bool isRegistered() {
		return mCamID > 0;
	}

	void setReconnectAddress(string val) {
		mReconnectAddress = val;

		if(mReconnectAddress.empty())
			Log.d("setReconnectAddress mReconnectAddress empty");
		else
			Log.d("setReconnectAddress mReconnectAddress=%s" , mReconnectAddress.c_str());

	}
	void setRegToken(CloudRegToken &regToken) {
		mRegToken = regToken;
	}

	CloudRegToken getRegToken() {
		return mRegToken;
	}

	/*void resetRegToken() {
		mRegToken.reset();
	}*/

	string getAddress() {
		Log.d("getAddress isEmpty=%d" , mRegToken.isEmpty());

		string uri;
		string address = mReconnectAddress.empty() ? mAddress : mReconnectAddress;
		bool noPort = (address.find(":") == string::npos);

		Log.d("getAddress address = %s" , address.c_str());

		if (!mRegToken.isEmpty()) {
			uri = PROTOCOL + address + (noPort ? ":" + fto_string(PORT) : "") + "/ctl/NEW/" + mRegToken.getToken() + "/";
		}
		else if ( !getConnID().empty() ) {
			uri = PROTOCOL + address + (noPort ?":" + fto_string(PORT) :"") + "/ctl/" + getConnID() + "/";
		}
		else {
			Log.e("getAddress, error");
		}

		Log.d("getAddress %s" ,uri.c_str());

		return uri;
	}

	void setCameraActivity(bool val) {
		mCameraActivity = val;
	}

	bool getCameraActivity() {
		return mCameraActivity;
	}

	bool getCameraStreaming() {
		return mCameraStreaming;
	}

	void setCameraStreaming(bool val) {
		mCameraStreaming = val;
	}

	bool getCameraStatusLed() {
		return mCameraStatusLed;
	}

	void setCameraStatusLed(bool val) {
		mCameraStatusLed = val;
	}

	string getCameraIPAddress() {
		return mCameraIPAddress;
	}

	string getCMVersion() {
		return mCMVersion;
	}
	void setCMVersion(string val) {
		mCMVersion = val;
	}

	string getCameraVendor() {
		return mCameraVendor;
	}
	void setCameraVendor(string val) {
		mCameraVendor = val;
	}

	string getCameraTimezone() {
		return mCameraTimezone;
	}
	void setCameraTimezone(string val) {
		mCameraTimezone = val;
	}

	string getCameraVersion() {
		return mCameraVersion;
	}

	string getCameraSerialNumber() {
		return mCameraSerialNumber;
	}

	string getCameraModel() {
		return mCameraModel;
	}

	string getCameraBrand() {
		return mCameraBrand;
	}


	void setStreamConfig(StreamConfig *conf) {
		if (mStreamConfig)
			mStreamConfig->Release();
		mStreamConfig = conf;
		mStreamConfig->AddRef();
	}

	StreamConfig *getStreamConfig() {
		return mStreamConfig;
	}

	void setSSLDisable(bool disable)
	{
		Log.d("WEBSOCKWRAP_ERROR dbg3");

		if (!CertFileExist() || SSL_Disabled())
			return;

#if (WITH_OPENSSL || WITH_MBEDTLS)

		m_bSSL_Disabled = disable;

		if (disable)
		{
			Log.d("WEBSOCKWRAP_ERROR dbg4");
			PROTOCOL = "ws://";
			PORT = 8888;
		}
		else
		{
			Log.d("WEBSOCKWRAP_ERROR dbg5");
			PROTOCOL = "wss://";
			PORT = 8883;
		}
#endif
	}

private:
	void configureStreamConfig() {
		mStreamConfig = new StreamConfig();

		// Video stream config
		StreamConfigVideo videoStreamConfig;
		videoStreamConfig.setStream("Vid"); // hardcoded name
		videoStreamConfig.setFormat("H.264");
		videoStreamConfig.setHorz(640);
		videoStreamConfig.setVert(480);
		videoStreamConfig.setVbr(true);
		videoStreamConfig.setQuality(0);
		videoStreamConfig.setGop(60);
		videoStreamConfig.setFps(30.0);
		videoStreamConfig.setCBRbrt(1024);
		videoStreamConfig.setVBRbrt(1024);
		mStreamConfig->addVideoStreamConfig(videoStreamConfig);

		// Audio stream config
		StreamConfigAudio audioStreamConfig;
		audioStreamConfig.setStream("Aud");
		audioStreamConfig.setBitrate(128);
		audioStreamConfig.setSrt(44.1);
		audioStreamConfig.setFormat("AAC");
		mStreamConfig->addAudioStreamConfig(audioStreamConfig);
	}

};

#endif
