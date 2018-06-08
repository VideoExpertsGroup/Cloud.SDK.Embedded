
#ifndef __STREAMCONFIGAUDIO_H__
#define __STREAMCONFIGAUDIO_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"

using namespace std;

class StreamConfigAudio
{
	const char *TAG = "StreamConfigAudio";
	const int LOG_LEVEL = 2; //Log.VERBOSE;
	MLog Log;

	string mStream = "Aud"; // default value
	int mBitrate = 128; // in kbps
	string mFormat = "AAC"; // default value
	double mSrt = 44.1; // default value

	const string FORMAT = "format";
	const string BRT = "brt";
	const string SRT = "srt";
	const string STREAM = "stream";

public:
	StreamConfigAudio() : Log(TAG, LOG_LEVEL) {};
	virtual ~StreamConfigAudio() {};

	StreamConfigAudio(string config) : Log(TAG, LOG_LEVEL) {
		json_error_t err;
		json_t *root = json_loads(config.c_str(), 0, &err);
		if (root) {
			json_t *v;

			v = json_object_get(root, FORMAT.c_str());
			if (v) mFormat = json_string_value(v);

			v = json_object_get(root, BRT.c_str());
			if (v) mBitrate = (int)json_integer_value(v);

			v = json_object_get(root, SRT.c_str());
			if (v) mSrt = json_real_value(v);

			v = json_object_get(root, STREAM.c_str());
			if (v) mStream = json_string_value(v);

			json_decref(root);
		}
	};

	string toJSONObject() {
		string ret;

		json_t *audio = json_object();
		json_object_set(audio, FORMAT.c_str(), json_string(mFormat.c_str()));
		json_object_set(audio, BRT.c_str(), json_integer(mBitrate));
		json_object_set(audio, SRT.c_str(), json_real(mSrt));
		json_object_set(audio, STREAM.c_str(), json_string(mStream.c_str()));

		ret = json_dumps(audio, 0);
		json_decref(audio);

		return ret;
	}

	double getSrt() {
		return mSrt;
	}

	void setSrt(double srt) {
		mSrt = srt;
	}

	int getBiterate() {
		return mBitrate;
	}
	void setBitrate(int brt) {
		mBitrate = brt;
	}

	string getFormat() {
		return mFormat;
	}

	void setFormat(string format) {
		mFormat = format;
	}

	string getStream() {
		return mStream;
	}

	void setStream(string stream) {
		mStream = stream;
	}

};

#endif //__STREAMCONFIGAUDIO_H__