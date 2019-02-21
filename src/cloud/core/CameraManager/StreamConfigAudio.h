
#ifndef __STREAMCONFIGAUDIO_H__
#define __STREAMCONFIGAUDIO_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"

using namespace std;

class StreamConfigAudio
{

//    char *TAG = (char*)"StreamConfigAudio";
//    int LOG_LEVEL = 2; //Log.VERBOSE;
    MLog Log;

    string mStream;
    int mBitrate;
    string mFormat;
    double mSrt;

    string FORMAT;
    string BRT;
    string SRT;
    string STREAM;

    void const_init()//for avoid -std=c++11 requirement
    {
        mStream = "Aud"; // default value
        mBitrate = 128; // in kbps
        mFormat = "AAC"; // default value
        mSrt = 44.1; // default value

        FORMAT = "format";
        BRT = "brt";
        SRT = "srt";
        STREAM = "stream";
    }

public:
    StreamConfigAudio() : Log("StreamConfigAudio", 2) { const_init(); };
	virtual ~StreamConfigAudio() {};

	StreamConfigAudio(string config) : Log("StreamConfigAudio", 2) {
        const_init();
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