
#ifndef __CLOUDREGTOKEN_H__
#define __CLOUDREGTOKEN_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"

#include <jansson.h>

using namespace std;

class CloudRegToken : public CUnk {
//	const char *TAG;
//	const int LOG_LEVEL;
	MLog Log;

	string mToken;
	string mExpire;
	string mType;
	string mStatus;
	string mRtmpPublish;
	long long mCmngrID;

public:

	CloudRegToken()
		:Log("CloudRegToken", 2)
	{
		mCmngrID = 0;
//		TAG = "CloudRegToken";
//		LOG_LEVEL = 2; //Log.VERBOSE;
		// nothing
	}

    CloudRegToken(string json_data)
		:Log("CloudRegToken", 2)
	{
		mCmngrID = 0;
//		TAG = "CloudRegToken";
//		LOG_LEVEL = 2; //Log.VERBOSE;

		json_error_t err;
		json_t *root = json_loads(json_data.c_str(), 0, &err);
		if (!root)
			return;

		const char *p = json_string_value(json_object_get(root, "token"));
		if (p)
			mToken = p;

		p = json_string_value(json_object_get(root, "expire"));
		if (p)
			mExpire = p;

		p = json_string_value(json_object_get(root, "status"));
		if (p)
			mStatus = p;

		p = json_string_value(json_object_get(root, "cmngr_id"));
		if (p)
			mCmngrID = json_integer_value(json_object_get(root, "cmngr_id"));

		p = json_string_value(json_object_get(root, "rtmp"));
		if (p)
			mRtmpPublish = p;

		json_decref(root);

    }

	virtual ~CloudRegToken()
	{

	}

	CloudRegToken &operator=(const CloudRegToken &src)
	{
		mToken = src.mToken;
		mExpire = src.mExpire;
		mType = src.mType;
		mStatus = src.mStatus;
		mRtmpPublish = src.mRtmpPublish;
		mCmngrID = src.mCmngrID;

		return *this;
	}

    bool isEmpty(){
        return (mToken.empty());
    }

    string &getToken(){
        return mToken;
    }

    string &getExpire(){
        return mExpire;
    }

    string &getType(){
        return mType;
    }

    string &getStatus(){
        return mStatus;
    }
    string &getRtmpPublish(){
        return mRtmpPublish;
    }

	long long getCmngrID() {
		return mCmngrID;
	}

};

#endif //__CLOUDREGTOKEN_H__