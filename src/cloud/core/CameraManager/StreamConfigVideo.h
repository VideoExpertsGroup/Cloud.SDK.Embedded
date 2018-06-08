
#ifndef __STREAMCONFIGVIDEO_H__
#define __STREAMCONFIGVIDEO_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"

#include <jansson.h>

using namespace std;

class StreamConfigVideo
{
	const char *TAG = "StreamConfigVideo";
	const int LOG_LEVEL = 2; //Log.VERBOSE;
	MLog Log;

	int mVert = 480; // default value
	int mHorz = 640; // default value
	bool mVbr = true; // default value
	double mFps = 30; // default value
	int mGop = 30; // default value
	int mQuality = 0; // default value
	string mFormat = "H.264"; // default value
	string mStream = "Vid"; // default value

	const string HORZ = "horz";
	const string VERT = "vert";
	const string FORMAT = "format";
	const string VBR = "vbr";
	const string GOP = "gop";
	const string STREAM = "stream";
	const string QUALITY = "quality";
	const string FPS = "fps";

public:
	StreamConfigVideo() : Log(TAG, LOG_LEVEL) {};
	virtual ~StreamConfigVideo() {};

	StreamConfigVideo(string config) : Log(TAG, LOG_LEVEL) {
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

};

#endif //__STREAMCONFIGVIDEO_H__