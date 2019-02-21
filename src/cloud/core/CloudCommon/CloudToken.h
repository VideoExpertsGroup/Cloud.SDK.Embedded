//
//  Copyright © 2016 VXG Inc. All rights reserved.
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

#ifndef __CLOUDTOKEN_H__
#define __CLOUDTOKEN_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"

#include <jansson.h>

using namespace std;

class CloudToken : public CUnk {
//	const char *TAG = "CloudToken";
//	const int LOG_LEVEL = 2; //Log.VERBOSE;
	MLog Log;

	string mToken;
	string mExpire;
	string mType;
	string mStatus;
	bool mHasError;
	string mErrorDetail;

public:

	CloudToken(string json_data)
		:Log("CloudToken", 2)
	{
		mHasError = true;
		mErrorDetail = "Invalid json";

		json_error_t err;
		json_t *root = json_loads(json_data.c_str(), 0, &err);
		if (!root) {
			return;
		}

		const char *pet = json_string_value(json_object_get(root, "errorType"));
		const char *ped = json_string_value(json_object_get(root, "errorDetail"));
		const char *p;

		if( pet && ped ){
				mHasError = true;
				mErrorDetail = ped;
				p = json_string_value(json_object_get(root, "status"));
				if (p)
					mStatus = p;
		}
		else {
			mHasError = false;
			mErrorDetail = "";

			p = json_string_value(json_object_get(root, "token"));
			if (p)
				mToken = p;

			p = json_string_value(json_object_get(root, "expire"));
			if (p)
				mExpire = p;

			p = json_string_value(json_object_get(root, "status"));
			if (p)
				mStatus = p;
		}

	}

	CloudToken()
		:Log("CloudToken", 2)
	{
		// nothing
		mHasError = true;
		mErrorDetail = "Invalid json";
	}

	virtual ~CloudToken()
	{

	}

	bool isEmpty() {
		return (mToken.empty() || mExpire.empty() || mType.empty());
	}

	string &getToken() {
		return mToken;
	}

	string &getExpire() {
		return mExpire;
	}

	string &getType() {
		return mType;
	}

	string &getStatus() {
		return mStatus;
	}

    void reset(){
        mToken = "";
        mExpire = "";
        mType = "";
    }


    void setToken(string &token){
        mToken = token;
    }

    void setExpire(string &expire){
        mExpire = expire;
    }

    void setType(string &type){
        mType = type;
    }

    /*long long calcExpireTime() {
        if (mExpire == null)
            return 1000;
        return CloudHelpers.parseTime(mExpire) - CloudHelpers.currentTimestampUTC();
    }

    public int minTimeForRefresh(){
        return 1000 * 60;
    }*/

    bool hasError(){
        return mHasError;
    }

    string &getErrorDetail(){
        return mErrorDetail;
    }

};

#endif //__CLOUDTOKEN_H__