
#ifndef __STREAMCONFIG_H__
#define __STREAMCONFIG_H__

#include "../../../utils/utils.h"
#include "../../../utils/_cunk.h"
#include "../../../utils/MLog.h"

#include <map>
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
public:
	std::map<std::string, StreamConfigVideo> video_streams;
	std::map<std::string, StreamConfigAudio> audio_streams;


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
			const json_t *value;
			json_array_foreach(video, i, value) {
				if (json_is_object(value)) {
					std::string videoConfig = json_dumps(value, 0);
					StreamConfigVideo scv(videoConfig);
					video_streams[scv.getStream()] = scv;
				}
			}
		}

		json_t *audio = json_object_get(root, AUDIO.c_str());
		if (json_is_array(audio)) {
			size_t i = 0;
			json_t *value;
			json_array_foreach(audio, i, value) {
				if (json_is_object(value)) {
					std::string audioConfig = json_dumps(value, 0);
					StreamConfigAudio sca(audioConfig);
					audio_streams[sca.getStream()] = sca;
				}
			}
		}
		json_decref(root);
	}

	void addVideoStreamConfig(StreamConfigVideo &vconf) {
		video_streams[vconf.getStream()] = vconf;
	}

	void addAudioStreamConfig(StreamConfigAudio &aconf) {
		audio_streams[aconf.getStream()] = aconf;
	}

	string toJSONObject() {

		json_error_t err;
		json_t *stream_config = json_object();

		//video
		json_t *video = json_array();
		for (std::map<std::string, StreamConfigVideo>::iterator it = video_streams.begin();
				it != video_streams.end(); ++it) {
			json_t *t = json_loads(it->second.toJSONObject().c_str(), 0, &err);
			json_array_append(video, t);
			json_decref(t);
		}
		json_object_set(stream_config, VIDEO.c_str(), video);

		//audio
		json_t *audio = json_array();
		for (std::map<std::string, StreamConfigAudio>::iterator it = audio_streams.begin();
				it != audio_streams.end(); ++it) {
			json_t *t = json_loads(it->second.toJSONObject().c_str(), 0, &err);
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

	StreamConfig& GetStreamConfigByName(std::string name) {

	}

	bool operator==(StreamConfig& rvalue)
	{
		return !(toJSONObject().compare(rvalue.toJSONObject()));
	}

	bool operator!=(StreamConfig& rvalue)
	{
		return !(*this == rvalue);
	}
};
#endif //__STREAMCONFIG_H__