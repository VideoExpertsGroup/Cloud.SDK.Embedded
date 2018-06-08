//
//  Copyright © 2017 VXG Inc. All rights reserved.
//  Contact: https://www.videoexpertsgroup.com/contact-vxg/
//  This file is part of the demonstration of the VXG Cloud Platform.
//
//  Commercial License Usage
//  Licensees holding valid commercial VXG licenses may use this file in
//  accordance with the commercial license agreement provided with the
//  Software or, alternatively, in accordance with the terms contained in
//  a written agreement between you and VXG Inc. For further information
//  use the contact form at https://www.videoexpertsgroup.com/contact-vxg/
//

#ifndef __CLOUDSHARECONNECTION_H__
#define __CLOUDSHARECONNECTION_H__

#include "../utils/utils.h"
#include "../utils/_cunk.h"
#include "../utils/MLog.h"
#include "../cloud/core/CloudAPI.h"
#include "../cloud/core/Uri.h"
#include "../Enums/CloudReturnCodes.h"

class CloudShareConnection : public CUnk 
{
	const char *TAG = "CloudShareConnection";
	const int LOG_LEVEL = 2; //Log.VERBOSE;
	MLog Log;

	CloudAPI mCloudAPI;
	bool mOpened;
	long long  m_ServerTimeDiff;

public:
	CloudShareConnection()
		:Log(TAG, LOG_LEVEL) 
	{
		mOpened = false;
		m_ServerTimeDiff = 0;
	}
	virtual ~CloudShareConnection() {
	}

	CloudAPI &_getCloudAPI() {
		return mCloudAPI;
	}


	int openSync(std::string &share_token) {
		std::string s = "https://web.skyvr.videoexpertsgroup.com";
		return openSync(share_token, s);
	}

	int openSync(std::string &share_token, std::string &baseurl) {
		std::string mProtocol = "https";
		std::string def_address = "web.skyvr.videoexpertsgroup.com";
		std::string mHost = def_address;
		int mPort = 80;
		std::string mPrefixPath = "";

		if (baseurl.length() > 1 && baseurl.at(baseurl.length()-1) == '/') {
			baseurl = baseurl.substr(0, baseurl.length() - 1);
		}

		Uri uri = Uri::Parse(baseurl);
		mProtocol = uri.Protocol;
		mHost = uri.Host;
		mPort = atoi(uri.Port.c_str());
		if (mPort == 0)
			mPort = 80;
		mPrefixPath = uri.Path;

		Log.i("Protocol: %s", mProtocol.c_str());
		Log.i("Host: %s", mHost.c_str());
		Log.i("Port: %d", mPort);
		Log.i("PrefixPath: %s", mPrefixPath.c_str());

		mCloudAPI.setHost(mHost + ((mPort > 0 && mPort != 80) ? ":" + std::to_string(mPort) : ""));
		mCloudAPI.setProtocol(mProtocol);
		mCloudAPI.setPrefixPath(mPrefixPath);
		mCloudAPI.setShareToken(share_token);
		if (!mHost.find(def_address)) {
			mCloudAPI.setCMAddress(mHost);
		}
		mOpened = true;

		return RET_OK;
	}

};

#endif //__CLOUDSHARECONNECTION_H__