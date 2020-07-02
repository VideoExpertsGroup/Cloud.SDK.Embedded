
#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef _WIN32
#define NOMINMAX
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif
#include <SDKDDKVer.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#endif

#include "MLog.h"
#include "windefsws.h"

#ifndef MP4_FILE_DURATION
#define MP4_FILE_DURATION	(5)
#endif

time_t GetTimeFromStringUTC(const char* szTime);
time_t GetTimeFromStringLocal(const char* szTime);
string GetCloudUrl(string acc);

namespace std {
template <typename T>
std::string to_string(const T &x)
{
    std::stringstream buf;
    buf << x;
    return buf.str();
}
}
#define PARAMETER_NOT_SET (-99999)

#define PARAMETER_CHAR_MAX_LENGTH (32)

typedef struct image_params
{
	int brightness;
	int saturation;
	int contrast;
	int hue;
	int sharpness;
	int vert_flip;
	int horz_flip;
	int mirror;
	int rotate;
	int ir_light;
	int tdn;
	int power_freq_60hz;
	char white_balance[PARAMETER_CHAR_MAX_LENGTH];
	const char** white_balance_types;
	char noise_reduction[PARAMETER_CHAR_MAX_LENGTH];
	const char** noise_reduction_types;
	int noise_reduction_level;
	int blc;
}image_params_t;

int SetDefaultImageParams(image_params_t* img);

typedef struct motion_params
{
	char* gridmap;
	int sensitivity;
	int columns;
	int rows;
	int max_regions;
	int region_shape;
	int enabled;
}motion_params_t;

int SetDefaultMotionParams(motion_params_t* mpr);

int packbits(int8_t *source, int8_t *dest, int size);
int8_t *unpackbits(int8_t *sourcep, int8_t *destp, int destcount);

#define MAX_PTZ_PRESETS (1024)
#define MAX_OSD_LENGTH (128)
#define MAX_AUDIO_FMTS_LENGTH (256)

typedef struct ptz_preset
{
	char* name;
	char* token;
}ptz_preset_t;

typedef struct ptz_caps
{
	int left;
	int right;
	int top;
	int bottom;
	int zoom_in;
	int zoom_out;
	int stop;
	int max_presets;
	ptz_preset_t preset[MAX_PTZ_PRESETS];
}ptz_caps_t;

typedef struct ptz_command
{
	char* command;
}ptz_command_t;


enum CAM_CMD_TYPE
{
	CAM_CMD_REBOOT = 1,
	CAM_CMD_MAX = 2,
};

typedef struct cam_command
{
	CAM_CMD_TYPE cmd_id;
	void* cmd_param;
}cam_command_t;


typedef struct osd_params
{
	int		system_id;						// optional bool, enable / disable static part of OSD
	int		time;							// optional bool, enable / disable time part of OSD
	int		date;							// optional bool, enable / disable date part of OSD
	int		bkg_transp;						// optional bool, enable / disable OSD background transparency
	char	system_id_text[MAX_OSD_LENGTH];	// optional string, a static content of OSD
	char	time_format[MAX_OSD_LENGTH];	// optional string, one of predefined values from caps
	char	date_format[MAX_OSD_LENGTH];	// optional string, one of predefined values from caps
	char	font_size[MAX_OSD_LENGTH];		// optional string, one of predefined font sizes from caps
	char	font_color[MAX_OSD_LENGTH];		// optional string, name of one of predefined font colors from caps
	char	bkg_color[MAX_OSD_LENGTH];		// optional string, name of one of predefined background colors from caps
	char	alignment[MAX_OSD_LENGTH];		// optional string, one of predefined positions from caps
}osd_params_t;

typedef struct osd_settings
{
	osd_params_t params;
	osd_params_t caps;
}osd_settings_t;

int SetDefaultPTZParams(ptz_caps_t* ptzpr);
int FreePTZParams(ptz_caps_t* ptzpr);

int SetDefaultOSDSettings(osd_settings_t* osdptr);
int FreeOSDSettings(osd_settings_t* ptzpr);
int SetDefaultOSDParams(osd_params_t* osdptr);
int FreeOSDParams(osd_params_t* ptzpr);


typedef struct clDefs
{
	char Name[16];
	char Color[16];
}clDefs_t;

extern clDefs clMainColors[];

typedef struct audio_caps
{
	int mic;		// bool, microphone is supported
	int spkr;		// bool, speaker is supported
	char echo_cancel[MAX_AUDIO_FMTS_LENGTH];// list of string, echo cancellation modes, empty or absent means not supported
	int backward;	// bool, backward audio supported.Obsolete.Server will ignore it when backward_formats exists.If true and backward_formats is missed, server will interpret supported formats list as[“UNKNOWN”]
	char backward_formats[MAX_AUDIO_FMTS_LENGTH];//list of string, list of supported backward formats. Supported values [“RAW”, “ADPCM”, “MP3”, “NELLY8”, “NELLY16”, “NELLY”, “G711A”, “G711U”, “AAC”, “SPEEX”, “UNKNOWN”]. Empty list or missing parameter – camera doesn't support back audio channel.
}audio_caps_t;

typedef struct audio_settings
{
	int mic_gain;	// optional int range 0 - 100, microphone gain
	int mic_mute;	// optional bool, microphone mute
	int spkr_vol;	// optional int range 0 - 100, speaker volume
	int spkr_mute;	// optional bool, speaker mute
	char echo_cancel[MAX_AUDIO_FMTS_LENGTH];// optional string, echo cancellation mode, “” means off
	audio_caps_t caps;
}audio_settings_t;

int SetDefaultAudioParams(audio_settings_t* audset);
int SetDefaultAudioCaps(audio_caps_t* audcaps);

typedef struct audDefs
{
	char NameCam[16];
	char NameServ[16];
	uint32_t FFmpegId;
}audDefs;

extern audDefs audNameTable[];
const char* ConvertAudioNameHik2Serv(const char* name);
const char* ConvertAudioNameServ2Hik(const char* name);
const char* ConvertAudioNameFF2Hik(uint16_t id);

#define WIFI_ENCRYPTION_UNKNOWN		0
#define WIFI_ENCRYPTION_NONE		1
#define WIFI_ENCRYPTION_WEP			2
#define WIFI_ENCRYPTION_WPA			4
#define WIFI_ENCRYPTION_WPA2		8
#define WIFI_ENCRYPTION_WPA_ENT		16
#define WIFI_ENCRYPTION_WPA2_ENT	32
#define WIFI_ENCRYPTION_WPA_RADIUS	64

#define MAX_WIFI_POINTS				32

extern const char* WiFiEncryptionTypesServer[];
extern const char* WiFiEncryptionTypesHk[];
extern const DWORD WiFiEncryptionID[];

const char* EncryptionNameServer(DWORD type);
const char* EncryptionNameHk(DWORD type);
DWORD EncryptionTypeServer(const char* name);
DWORD EncryptionTypeHk(const char* name);

typedef struct wifi_params
{
	char		ssid[64];
	char		mac[32];
	char		password[32];
	int			signal;
	DWORD		encryption_caps;
	DWORD		encryption;
	bool		connected;
}wifi_params_t;

typedef struct wifi_list
{
	wifi_params ap[MAX_WIFI_POINTS];
}wifi_list_t;

void SetDefaultWiFiParams(wifi_params* param);
void SetDefaultWiFiList(wifi_list* wlist);

const char* GetProgramPath();
const char* GetSettingsFile();
bool CertFileExist();
bool SSL_Disabled();
bool IsSplitLog();
bool RTMPS_Disabled();

std::string str_replace(std::string str, std::string old_str, std::string new_str);
std::string str_encode(std::string str);

#endif //__UTILS_H__

