
#ifndef __STREAMCONFIGVIDEO_H__
#define __STREAMCONFIGVIDEO_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"

#include <jansson.h>

using namespace std;

class StreamConfigVideo
{

//    const char *TAG = "StreamConfigVideo";
//    const int LOG_LEVEL = 2;
    MLog Log;

    int mVert;
    int mHorz;
    bool mVbr;
    double mFps;
    int mGop;
    int mQuality;
	int mVbrBrt;
	int mCbrBrt;
	string mFormat;
    string mStream;

    string HORZ;
    string VERT;
    string FORMAT;
    string VBR;
    string GOP;
    string STREAM;
    string QUALITY;
    string FPS;
	string CBR_BRT;
	string VBR_BRT;

    void const_init()//for avoid -std=c++11 requirement
    {
        mVert = 480; // default value
        mHorz = 640; // default value
        mVbr = true; // default value
        mFps = 30; // default value
        mGop = 30; // default value
        mQuality = 0; // default value
        mFormat = "H.264"; // default value
        mStream = "Vid"; // default value
		mVbrBrt = 1022;
		mCbrBrt = 1023;

        HORZ	= "horz";
        VERT	= "vert";
        FORMAT	= "format";
        VBR		= "vbr";
        GOP		= "gop";
        STREAM	= "stream";
        QUALITY	= "quality";
        FPS		= "fps";
		CBR_BRT	= "brt";
		VBR_BRT	= "vbr_brt";
    }

public:
    StreamConfigVideo() : Log("StreamConfigVideo", 2) { const_init(); };
	virtual ~StreamConfigVideo() {};

	StreamConfigVideo(string config) : Log("StreamConfigVideo", 2) {
        const_init();
		json_error_t err;
		json_t *root = json_loads(config.c_str(), 0, &err);
		if (root) {
			json_t *v;

			v = json_object_get(root, VERT.c_str());
			if (v) mVert = (int)json_integer_value(v);

			v = json_object_get(root, HORZ.c_str());
			if (v) mHorz = (int)json_integer_value(v);

			v = json_object_get(root, STREAM.c_str());
			if (v) mStream = json_string_value(v);

			v = json_object_get(root, FORMAT.c_str());
			if (v) mFormat = json_string_value(v);

			v = json_object_get(root, VBR.c_str());
			if (v) mVbr = json_boolean_value(v);

			v = json_object_get(root, FPS.c_str());
			if (v) mFps = json_real_value(v);

			v = json_object_get(root, QUALITY.c_str());
			if (v) mQuality = (int)json_integer_value(v);

			v = json_object_get(root, GOP.c_str());
			if (v) mGop = (int)json_integer_value(v);

			v = json_object_get(root, CBR_BRT.c_str());
			if (v) mCbrBrt = (int)json_integer_value(v);

			v = json_object_get(root, VBR_BRT.c_str());
			if (v) mVbrBrt = (int)json_integer_value(v);

			json_decref(root);
		}
	};

	string toJSONObject() {
		string ret;

		json_t *video = json_object();
		json_object_set(video, VERT.c_str(), json_integer(mVert));
		json_object_set(video, HORZ.c_str(), json_integer(mHorz));
		json_object_set(video, STREAM.c_str(), json_string(mStream.c_str()));
		json_object_set(video, FORMAT.c_str(), json_string(mFormat.c_str()));
		json_object_set(video, VBR.c_str(), json_boolean(mVbr));
		json_object_set(video, FPS.c_str(), json_real(mFps));
		json_object_set(video, QUALITY.c_str(), json_integer(mQuality));
		json_object_set(video, GOP.c_str(), json_integer(mGop));
		json_object_set(video, CBR_BRT.c_str(), json_integer(mCbrBrt));
		json_object_set(video, VBR_BRT.c_str(), json_integer(mVbrBrt));

		ret = json_dumps(video, 0);
		json_decref(video);

		return ret;
	}

	void setVert(int vert) {
		mVert = vert;
	}

	int getVert() {
		return mVert;
	}

	void setHorz(int horz) {
		mHorz = horz;
	}

	int getHorz() {
		return mHorz;
	}

	bool isVbr() {
		return mVbr;
	}

	void setVbr(bool mVbr) {
		this->mVbr = mVbr;
	}

	double getFps() {
		return mFps;
	}

	void setFps(double mFps) {
		this->mFps = mFps;
	}

	int getGop() {
		return mGop;
	}

	void setGop(int mGop) {
		this->mGop = mGop;
	}

	int getQuality() {
		return mQuality;
	}

	void setQuality(int mQuality) {
		this->mQuality = mQuality;
	}

	string getFormat() {
		return mFormat;
	}

	void setFormat(string mFormat) {
		this->mFormat = mFormat;
	}

	string getStream() {
		return mStream;
	}

	void setStream(string mStream) {
		this->mStream = mStream;
	}

	int getCBRbrt() {
		return mCbrBrt;
	}

	int getVBRbrt() {
		return mVbrBrt;
	}

	void setCBRbrt(int mBrt) {
		this->mCbrBrt = mBrt;
	}

	void setVBRbrt(int mBrt) {
		this->mVbrBrt = mBrt;
	}

};

#endif //__STREAMCONFIGVIDEO_H__