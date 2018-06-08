#ifndef __CAMERAMANAGERCONFIG_H__
#define __CAMERAMANAGERCONFIG_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"
#include "../CloudCommon/CloudRegToken.h"
#include "StreamConfig.h"

using namespace std;

class CameraManagerConfig : public CUnk {
	const char *TAG = "CameraManagerConfig";
	const int LOG_LEVEL = 2; //Log.VERBOSE;
	MLog Log;

	string mUploadUrl = "";
	string mMediaServer = "";
	string mCamPath = "";
	string mCA = ""; // TODO
	string mSID = "";
	string mPwd = "";
	string mUUID = "";
	string mConnID = "";
	long long mCamID = 0;
	CloudRegToken mRegToken;

	// some camera details
	bool mCameraActivity = false;
	bool mCameraStreaming = true;
	bool mCameraStatusLed = false;
	string mCameraIPAddress = "";
	string mCMVersion = "";
	string mCameraBrand = "";
	string mCameraModel = "";
	string mCameraSerialNumber = "";
	string mCameraVersion = "";
	string mCameraTimezone = "";
	string mCameraVendor = "";
	StreamConfig *mStreamConfig = NULL;

	// WS(S) - 8888(8883)
	// default parameters
	string PROTOCOL = "ws://";
	string DEFAULT_ADDRESS = "cam.skyvr.videoexpertsgroup.com";
	string mAddress = DEFAULT_ADDRESS;
	int PORT = 8888;

	string mReconnectAddress = "";

public:
	virtual ~CameraManagerConfig() {
		if (mStreamConfig)
			mStreamConfig->Release();
	};

	CameraManagerConfig() 
		: Log(TAG, LOG_LEVEL) 
	{
		mCameraIPAddress = "127.0.0.1";//CameraManagerHelper.getLocalIpAddress();
		mCameraBrand = "Camera Brand";//Build.BRAND;
		mCameraModel = "Camera Model"; //Build.MODEL;
		mCameraSerialNumber = "Camera Serial"; //Build.SERIAL;
		mCameraVersion = "1";
		mCMVersion = "Camera CM";
		mCameraVendor = "vendor";
		mCameraTimezone = "UTC";//TimeZone.getDefault().getID();

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
		Log.v("getAddress");

		string uri;
		string address = mReconnectAddress.empty() ? mAddress : mReconnectAddress;

		if ( !getConnID().empty() ) {
			uri = PROTOCOL + address + ":" + std::to_string(PORT) + "/ctl/" + getConnID() + "/";
		}
		else if (!mRegToken.isEmpty()) {
			uri = PROTOCOL + address + ":" + std::to_string(PORT) + "/ctl/NEW/" + mRegToken.getToken() + "/";
		}
		else {
			Log.e("getAddress, error");
		}
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