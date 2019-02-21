
#ifndef __STREAMCONFIG_H__
#define __STREAMCONFIG_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"

#include <vector>
#include <jansson.h>

#include "StreamConfigVideo.h"
#include "StreamConfigAudio.h"

using namespace std;

class StreamConfig : public CUnk
{
//	const char *TAG = "StreamConfig";
//	const int LOG_LEVEL = 2; //Log.VERBOSE;
	MLog Log;

	string VIDEO;
	string AUDIO;

	vector<StreamConfigVideo> video_streams;
	vector<StreamConfigAudio> audio_streams;

public:
	StreamConfig() : Log("StreamConfig", 2) {VIDEO = "video";AUDIO = "audio";};
	virtual ~StreamConfig() {};

	StreamConfig(string &config) : Log("StreamConfig", 2) {

		VIDEO = "video";
		AUDIO = "audio";

		json_error_t err;
		json_t *root = json_loads(config.c_str(), 0, &err);

		json_t *video = json_object_get(root, VIDEO.c_str());
		if (json_is_array(video)) {
			size_t i = 0;
			json_t *value;
			json_array_foreach(video, i, value) {
				StreamConfigVideo scv(json_string_value(value));
				video_streams.push_back(scv);
			}
		}

		json_t *audio = json_object_get(root, AUDIO.c_str());
		if (json_is_array(audio)) {
			size_t i = 0;
			json_t *value;
			json_array_foreach(video, i, value) {
				StreamConfigAudio scv(json_string_value(value));
				audio_streams.push_back(scv);
			}
		}
		json_decref(root);
	}

	void addVideoStreamConfig(StreamConfigVideo &vconf) {
		bool bExists = false;
		for (size_t i = 0; i < video_streams.size(); i++) {
			StreamConfigVideo tmpVConf = video_streams[i];
			if (tmpVConf.getStream() == vconf.getStream()) {
				bExists = true;
			}
		}
		if (!bExists) {
			video_streams.push_back(vconf);
		}
	}

	void addAudioStreamConfig(StreamConfigAudio &aconf) {
		bool bExists = false;
		for (size_t i = 0; i < audio_streams.size(); i++) {
			StreamConfigAudio tmpAConf = audio_streams[i];
			if (tmpAConf.getStream() == aconf.getStream()) {
				bExists = true;
			}
		}
		if (!bExists) {
			audio_streams.push_back(aconf);
		}
	}

	string toJSONObject() {

		json_error_t err;
		json_t *stream_config = json_object();

		//video
		json_t *video = json_array();
		for (size_t i = 0; i < video_streams.size(); i++) {
			json_t *t=json_loads(video_streams[i].toJSONObject().c_str(), 0, &err);
			json_array_append(video, t);
			json_decref(t);
		}
		json_object_set(stream_config, VIDEO.c_str(), video);

		//audio
		json_t *audio = json_array();
		for (size_t i = 0; i < audio_streams.size(); i++) {
			json_t *t = json_loads(audio_streams[i].toJSONObject().c_str(), 0, &err);
			json_array_append(audio, t);
			json_decref(t);
		}
		json_object_set(stream_config, AUDIO.c_str(), audio);

		string ret = json_dumps(stream_config, 0);

		json_decref(video);
		json_decref(audio);
		json_decref(stream_config);

		return ret;
	}


};
#endif //__STREAMCONFIG_H__