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
	int PORT;

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
		mCameraVersion = "3"; //"1" in CloudDK.Android
		mCMVersion = "Camera CM";
		mCameraVendor = "vendor";
		mCameraTimezone = "UTC";//TimeZone.getDefault().getID();

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

		mCameraActivity = false;
		mCameraStreaming = true;
		mCameraStatusLed = false;
		mCameraIPAddress = "";
		mCMVersion = "";
		mCameraBrand = "";
		mCameraModel = "";
		mCameraSerialNumber = "";
		mCameraVersion = "";
		mCameraTimezone = "";
		mCameraVendor = "";
		mStreamConfig = NULL;

		PROTOCOL = "ws://";
		DEFAULT_ADDRESS = "cam.skyvr.videoexpertsgroup.com";
		mAddress = DEFAULT_ADDRESS;
		PORT = 8888;
		mReconnectAddress = "";

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

		if (mStreamConfig)
			mStreamConfig->Release();
		mStreamConfig = src.mStreamConfig;
		if (mStreamConfig)
			mStreamConfig->AddRef();

		// WS(S) - 8888(8883)
		// default parameters
		PROTOCOL = src.PROTOCOL;
		DEFAULT_ADDRESS = src.DEFAULT_ADDRESS;
		mAddress = src.mAddress;
		PORT = src.PORT;

		mReconnectAddress = src.mReconnectAddress;

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
    }

	void setCMAddress(string url) {
		if (url.empty()) {
			mAddress = DEFAULT_ADDRESS;
		}
		else {
			mAddress = url;
		}
	}

	string getCMAddress() {
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
		Log.v("getAddress isEmpty=%d" , mRegToken.isEmpty());

		string uri;
		string address = mReconnectAddress.empty() ? mAddress : mReconnectAddress;
		bool noPort = (address.find(":") == string::npos);

		if (!mRegToken.isEmpty()) {
			uri = PROTOCOL + address + (noPort ? ":" + fto_string(PORT) : "") + "/ctl/NEW/" + mRegToken.getToken() + "/";
		}
		else if ( !getConnID().empty() ) {
			uri = PROTOCOL + address + (noPort ?":" + fto_string(PORT) :"") + "/ctl/" + getConnID() + "/";
		}
		else {
			Log.e("getAddress, error");
		}

		Log.v("getAddress %s" ,uri.c_str());
		
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
