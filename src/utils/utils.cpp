

#include "utils.h"
#include "base64.h"
#include "jansson.h"

int json_try_get_int(json_t* jdata, const char* param)
{
	int ret = 0;

	json_t* jstr = json_object_get(jdata, param);
	if (!jstr)
		return -999;

	const char* szParam = json_string_value(jstr);
	if (szParam && strlen(szParam))
		ret = atoi(szParam);
	else
		ret = json_integer_value(jstr);

	return ret;
}


time_t GetTimeFromString(bool bUTC, const char* szTime)
{
	if ((!szTime) || (strlen(szTime) < 18))
	{
		MLog Log("UTILS", 2);
		Log.e("%s, Invalid time string\n", __FUNCTION__);
		return 0;
	}

	struct tm timeinfo;
	char tmp[16] = { 0 };
	memset(&timeinfo, 0, sizeof(timeinfo));
	memset(tmp, 0, 16);
	memcpy(tmp, szTime, 4);
	timeinfo.tm_year = atoi(tmp) - 1900;
	memset(tmp, 0, 16);
	memcpy(tmp, szTime + 5, 2);
	timeinfo.tm_mon = atoi(tmp) - 1;
	memset(tmp, 0, 16);
	memcpy(tmp, szTime + 8, 2);
	timeinfo.tm_mday = atoi(tmp);
	memset(tmp, 0, 16);
	memcpy(tmp, szTime + 11, 2);
	timeinfo.tm_hour = atoi(tmp);
	memset(tmp, 0, 16);
	memcpy(tmp, szTime + 14, 2);
	timeinfo.tm_min = atoi(tmp);
	memset(tmp, 0, 16);
	memcpy(tmp, szTime + 17, 2);
	timeinfo.tm_sec = atoi(tmp);

	if(bUTC)
		return timegm(&timeinfo);

	return mktime(&timeinfo);

}

time_t GetTimeFromStringUTC(const char* szTime)
{
	return GetTimeFromString(true, szTime);
}

time_t GetTimeFromStringLocal(const char* szTime)
{
	return GetTimeFromString(false, szTime);
}


string GetCloudUrl(string acc)
{
	MLog Log("UTILS", 2);

	const char* acc_token = acc.c_str();
	int acc_len = strlen(acc_token) * 2 + 1;
	char* acc_data = (char*)malloc(acc_len);
	acc_len = Decode64_2(acc_data, acc_len, acc_token, strlen(acc_token));
	acc_data[acc_len] = 0;

	json_error_t err;
	json_t *jacc = json_loads(acc_data, 0, &err);

	if (!jacc)
	{
		Log.e("!jacc");
		free(acc_data);
		return "";
	}

	Log.d("acc_data==%s", acc_data);

	string url = "";
	const char *szApi = json_string_value(json_object_get(jacc, "api"));
	int nApiPort = 0;

#if (0)//WITH_OPENSSL)
	const char* pszApiPort = "api_sp";
#else
	const char* pszApiPort = "api_p";
#endif

	int port = json_try_get_int(jacc, pszApiPort);
	if (port >= 0)
		nApiPort = port;

	if (szApi)
	{
		Log.d("szApi==%s", szApi);
		url = szApi;
	}
	else
	{
		url = "web.skyvr.videoexpertsgroup.com";

		char szEndPoint[512] = { 0 };
		MYGetPrivateProfileString(
			"EndPoint",
			"DefaultApiAddress",
			"web.skyvr.videoexpertsgroup.com",
			szEndPoint,
			512,
			GetSettingsFile()
		);
		url = szEndPoint;
		Log.v("szApi==NULL, use default url %s", url.c_str());
	}

	if (nApiPort > 0)
	{
		url += ":";
		url += to_string(nApiPort);
	}

	Log.d("%s url %s", __FUNCTION__, url.c_str());

	free(acc_data);
	json_decref(jacc);

	return url;
}

int SetDefaultImageParams(image_params_t* img)
{
	if(!img)
		return -1;

	img->brightness = PARAMETER_NOT_SET;
	img->contrast = PARAMETER_NOT_SET;
	img->saturation = PARAMETER_NOT_SET;
	img->hue = PARAMETER_NOT_SET;
	img->vert_flip = PARAMETER_NOT_SET;
	img->horz_flip = PARAMETER_NOT_SET;
	img->ir_light = PARAMETER_NOT_SET;
	img->tdn = PARAMETER_NOT_SET;
	img->sharpness = PARAMETER_NOT_SET;
	img->power_freq_60hz = PARAMETER_NOT_SET;
	img->blc = PARAMETER_NOT_SET;
	img->white_balance_types = (const char**)PARAMETER_NOT_SET;
	memset(img->white_balance, 0, PARAMETER_CHAR_MAX_LENGTH);
	img->noise_reduction_level = PARAMETER_NOT_SET;
	img->noise_reduction_types = (const char**)PARAMETER_NOT_SET;
	memset(img->noise_reduction, 0, PARAMETER_CHAR_MAX_LENGTH);

	return 0;
}

int SetDefaultMotionParams(motion_params_t* mpr)
{
	if (!mpr)
		return -1;
	mpr->sensitivity = PARAMETER_NOT_SET;
	mpr->gridmap = NULL;
	mpr->columns = PARAMETER_NOT_SET;
	mpr->rows = PARAMETER_NOT_SET;
	mpr->max_regions = PARAMETER_NOT_SET;
	mpr->region_shape = PARAMETER_NOT_SET;
	mpr->enabled = PARAMETER_NOT_SET;
	return 0;
}


int packbits(int8_t *src, int8_t *dst, int n)
{
	int8_t *p, *q, *run, *dataend;
	int count, maxrun;

	dataend = src + n;
	for (run = src, q = dst; n > 0; run = p, n -= count)
	{
		maxrun = n < 128 ? n : 128;
		if (run <= (dataend - 3) && run[1] == run[0] && run[2] == run[0])
		{
			for (p = run + 3; p < (run + maxrun) && *p == run[0]; )
				++p;
			count = p - run;
			*q++ = 1 + 256 - count;
			*q++ = run[0];
		}
		else
		{
			for (p = run; p < (run + maxrun); )
				if (p <= (dataend - 3) && p[1] == p[0] && p[2] == p[0])
					break;
				else
					++p;
			count = p - run;
			*q++ = count - 1;
			memcpy(q, run, count);
			q += count;
		}
	}
	return q - dst;
}

int8_t *unpackbits(int8_t *sourcep, int8_t *destp, int destcount)
{

//	MLog Log("UTILS", 2);
//	Log.e("%s [%d]\n", __FUNCTION__, __LINE__);

	int runlength;
	while (destcount > 0)
	{
		runlength = *sourcep;
//		Log.e("%s [%d], runlength=%d\n", __FUNCTION__, __LINE__, runlength);
		sourcep++;
		if (runlength == -128)
			continue;
		else if (runlength < 0)
		{
			runlength = (-runlength) + 1;
			if (runlength > destcount)
				return NULL;

			memset(destp, *sourcep, runlength);
			sourcep += 1;
			destp += runlength;
		}
		else
		{
			++runlength;

			if (runlength > destcount)
				return NULL;

			memcpy(destp, sourcep, runlength);
			sourcep += runlength;
			destp += runlength;
		}
		destcount -= runlength;
	}
	return (destcount == 0) ? sourcep : NULL;
}


int FreePTZParams(ptz_caps_t* ptzpr)
{
	if (!ptzpr)
		return -1;
	for (int n = 0; n < MAX_PTZ_PRESETS; n++)
	{
		if (ptzpr->preset[n].name)
			free(ptzpr->preset[n].name);
		if (ptzpr->preset[n].token)
			free(ptzpr->preset[n].token);
		ptzpr->preset[n].name = NULL;
		ptzpr->preset[n].token = NULL;
	}
	return 0;
}

int SetDefaultPTZParams(ptz_caps_t* ptzpr)
{
	if (!ptzpr)
		return -1;

	ptzpr->left = 0;
	ptzpr->right = 0;
	ptzpr->top = 0;
	ptzpr->bottom = 0;
	ptzpr->zoom_in = 0;
	ptzpr->zoom_out = 0;
	ptzpr->stop = 0;
	ptzpr->max_presets = 0;

	for (int n = 0; n < MAX_PTZ_PRESETS; n++)
	{
		ptzpr->preset[n].name = NULL;
		ptzpr->preset[n].token = NULL;
	}
	return 0;
}

clDefs clMainColors[] =
{
	"Auto",		"010101",
	"Black",	"000000",
	"Maroon",	"800000",
	"Green",	"008000",
	"Olive",	"808000",
	"Navy",		"000080",
	"Purple",	"800080",
	"Teal",		"008080",
	"Gray",		"808080",
	"Silver",	"C0C0C0",
	"Red",		"FF0000",
	"Lime",		"00FF00",
	"Yellow",	"FFFF00",
	"Blue",		"0000FF",
	"Fuchsia",	"FF00FF",
	"Aqua",		"00FFFF",
	"White",	"FFFFFF",
	"",	""
};

int SetDefaultOSDParams(osd_params_t* osdptr)
{
	osdptr->system_id = PARAMETER_NOT_SET;
	osdptr->time = PARAMETER_NOT_SET;
	osdptr->date = PARAMETER_NOT_SET;
	osdptr->bkg_transp = PARAMETER_NOT_SET;
	memset(osdptr->system_id_text, 0, MAX_OSD_LENGTH);
	memset(osdptr->time_format, 0, MAX_OSD_LENGTH);
	memset(osdptr->date_format, 0, MAX_OSD_LENGTH);
	memset(osdptr->font_size, 0, MAX_OSD_LENGTH);
	memset(osdptr->font_color, 0, MAX_OSD_LENGTH);
	memset(osdptr->bkg_color, 0, MAX_OSD_LENGTH);
	memset(osdptr->alignment, 0, MAX_OSD_LENGTH);
	return 0;
}

int SetDefaultOSDSettings(osd_settings_t* osdptr)
{
	SetDefaultOSDParams(&osdptr->caps);
	SetDefaultOSDParams(&osdptr->params);
	return 0;
}

int FreeOSDParams(osd_params_t* osdptr)
{
	return 0;
}

int FreeOSDSettings(osd_settings_t* osdptr)
{
	FreeOSDParams(&osdptr->caps);
	FreeOSDParams(&osdptr->params);
	return 0;
}

int SetDefaultAudioCaps(audio_caps_t* audcaps)
{
	audcaps->backward = PARAMETER_NOT_SET;
	audcaps->mic = PARAMETER_NOT_SET;
	audcaps->spkr = PARAMETER_NOT_SET;
	memset(audcaps->echo_cancel, 0, MAX_AUDIO_FMTS_LENGTH);
	memset(audcaps->backward_formats, 0, MAX_AUDIO_FMTS_LENGTH);
	return 0;
}

int SetDefaultAudioParams(audio_settings_t* audset)
{
	audset->mic_gain = PARAMETER_NOT_SET;
	audset->mic_mute = PARAMETER_NOT_SET;
	audset->spkr_mute = PARAMETER_NOT_SET;
	audset->spkr_vol = PARAMETER_NOT_SET;
	memset(audset->echo_cancel, 0, MAX_AUDIO_FMTS_LENGTH);
	SetDefaultAudioCaps(&audset->caps);
	return 0;
}

audDefs audNameTable[] =
{
	"G.711alaw",	"G711A",	0x10000 + 7,	//AV_CODEC_ID_PCM_AULAW
	"G.711ulaw",	"G711U",	0x10000 + 6,	//AV_CODEC_ID_PCM_MULAW
	"G.726",		"",			0x11000 + 11,	//AV_CODEC_ID_ADPCM_G726
	"G.729",		"",			0x15000 + 53,	//AV_CODEC_ID_G729
	"G.729a",		"",			0,
	"G.729b",		"",			0,
	"PCM",			"",			0x10000 + 0,	//AV_CODEC_ID_PCM_S16LE
	"MP3",			"MP3",		0x15000 + 1,	//AV_CODEC_ID_MP3
	"AC3",			"",			0x15000 + 3,	//AV_CODEC_ID_AC3
	"AAC",			"AAC",		0x15000 + 2,	//AV_CODEC_ID_AAC
	"ADPCM",		"ADPCM",	0x11000 + 6,	//AV_CODEC_ID_ADPCM_MS
	"MP2L2",		"",			0x15000 + 0,	//AV_CODEC_ID_MP2
	"",				"",			0
};
//[“RAW”, “ADPCM”, “MP3”, “NELLY8”, “NELLY16”, “NELLY”, “G711A”, “G711U”, “AAC”, “SPEEX”, “UNKNOWN”]

const char* ConvertAudioNameHik2Serv(const char* name)
{
	if (!name)
		return NULL;

	int n = 0;
	while (strlen(audNameTable[n].NameCam))
	{
		if (!strcmp(audNameTable[n].NameCam, name))
			return(audNameTable[n].NameServ);
		n++;
	}
	return NULL;
}

const char* ConvertAudioNameServ2Hik(const char* name)
{
	if (!name)
		return NULL;

	int n = 0;
	while (strlen(audNameTable[n].NameCam))
	{
		if (!strcmp(audNameTable[n].NameServ, name))
			return(audNameTable[n].NameCam);
		n++;
	}
	return NULL;
}

const char* ConvertAudioNameFF2Hik(uint16_t id)
{
	int n = 0;
	while (strlen(audNameTable[n].NameCam))
	{
		if (audNameTable[n].FFmpegId == id)
		{
			return(audNameTable[n].NameCam);
		}
		n++;
	}
	return NULL;
}

const char* GetProgramPath()
{
	std::string path="";
	path.resize(1024);
	static char app_path[1024] = { 0 };
	if (!strlen(app_path))
	{
#ifndef _WIN32
		ssize_t ret = readlink("/proc/self/exe", &path[0], path.size());
		path.resize(ret);
/*
		if (path.at(0) == 0)
			Log.e("/proc/self/exe FAILED\n ");
		else
			Log.d("/proc/self/exe = %s\n", path.c_str());
*/
		strcpy(app_path, path.c_str());
#else
		DWORD size = GetModuleFileNameA(NULL, app_path, 1024);
#endif
		char* z = strrchr(app_path, '/');//linux
		if (!z)z = strrchr(app_path, '\\');//windows
		if (z) z[1] = 0;
		else
			strcpy(app_path, "./");
	}

	return app_path;
}

bool CertFileExist()
{
	int nCertFile = -1;
	char certpath[MAX_PATH] = { 0 };
	sprintf(certpath, "%s%s", GetProgramPath(), "cert.pem");
	nCertFile = _open(certpath, O_RDONLY);
	if (nCertFile < 0)
		return false;
	_close(nCertFile);
	return true;
}

int filecopy(FILE* dest, FILE* src)
{
	const int size = 64;
	char buffer[size];
	while (!feof(src))
	{
		int n = fread(buffer, 1, size, src);
		fwrite(buffer, 1, n, dest);
	}
	fflush(dest);
	return 0;
}

int filecopy(char* dest, char* src)
{
	FILE* infile = fopen(src, "rb");
	if (!infile)
		return -1;
	FILE* outfile = fopen(dest, "wb");
	if (!outfile)
		return -1;
	filecopy(outfile, infile);
	fclose(infile);
	fclose(outfile);
	return 0;
}

const char* GetSettingsFile()
{
	static char filepath[MAX_PATH] = { 0 };
	if (0 == filepath[0])
	{
		char defpath[MAX_PATH] = { 0 };
		sprintf(defpath, "%s%s", GetProgramPath(), "settings.cfg");
		DIR* dp = NULL;
		if ((dp = opendir("/config")) != NULL)
		{
			sprintf(filepath, "/config/vxg_app.cfg");
			FILE* cfgfile = fopen(filepath, "rb");
			if (!cfgfile)
				filecopy(filepath, defpath);
			else
				fclose(cfgfile);
		}
		else
		{
			strcpy(filepath, defpath);
		}
	}
	return filepath;
}

bool SSL_Disabled()
{
	char szSSL[32] = { 0 };
	MYGetPrivateProfileString(
		"EndPoint",
		"SSLDisabled",
		"0",
		szSSL,
		32,
		GetSettingsFile()
	);
	return atoi(szSSL);
}

bool RTMPS_Disabled()
{
	char szVal[32] = { 0 };
	MYGetPrivateProfileString(
		"EndPoint",
		"RTPMSDisabled",
		"0",
		szVal,
		32,
		GetSettingsFile()
	);
	return atoi(szVal);
}

bool IsSplitLog()
{
	char szSplitLog[32] = { 0 };
	MYGetPrivateProfileString(
		"Log",
		"Split",
		"1",
		szSplitLog,
		32,
		GetSettingsFile()
	);

	if (strlen(szSplitLog))
		return atoi(szSplitLog);
	else
		return 0;
}

const char* WiFiEncryptionTypesServer[] = { "", "none", "WEP", "WPA", "WPA2", "WPA_enterprise", "WPA2_enterprise", NULL };
const char* WiFiEncryptionTypesHk[] = { "", "disable", "WEP", "WPA-personal", "WPA2-personal", "WPA-enterprise", "WPA2-enterprise", "WPA-RADIUS", NULL };
const DWORD WiFiEncryptionID[] = {
WIFI_ENCRYPTION_UNKNOWN		,
WIFI_ENCRYPTION_NONE		,
WIFI_ENCRYPTION_WEP			,
WIFI_ENCRYPTION_WPA			,
WIFI_ENCRYPTION_WPA2		,
WIFI_ENCRYPTION_WPA_ENT		,
WIFI_ENCRYPTION_WPA2_ENT	,
WIFI_ENCRYPTION_WPA_RADIUS	,
};

const char* EncryptionNameServer(DWORD type)
{
	int n = 0;
	while (WiFiEncryptionTypesServer[n])
	{
		if (WiFiEncryptionID[n] == type)
			return WiFiEncryptionTypesServer[n];
		n++;
	}
	return NULL;
}

const char* EncryptionNameHk(DWORD type)
{

	int n = 0;
	while (WiFiEncryptionTypesHk[n])
	{
		if (WiFiEncryptionID[n] == type)
			return WiFiEncryptionTypesHk[n];
		n++;
	}
	return NULL;
}

DWORD EncryptionTypeServer(const char* name)
{
	if (!name)
		return WIFI_ENCRYPTION_UNKNOWN;
	if (!strcmp(name, WiFiEncryptionTypesServer[1]))
		return WIFI_ENCRYPTION_NONE;
	if (!strcmp(name, WiFiEncryptionTypesServer[2]))
		return WIFI_ENCRYPTION_WEP;
	if (!strcmp(name, WiFiEncryptionTypesServer[3]))
		return WIFI_ENCRYPTION_WPA;
	if (!strcmp(name, WiFiEncryptionTypesServer[4]))
		return WIFI_ENCRYPTION_WPA2;
	if (!strcmp(name, WiFiEncryptionTypesServer[5]))
		return WIFI_ENCRYPTION_WPA_ENT;
	if (!strcmp(name, WiFiEncryptionTypesServer[6]))
		return WIFI_ENCRYPTION_WPA2_ENT;

	return WIFI_ENCRYPTION_UNKNOWN;
}

DWORD EncryptionTypeHk(const char* name)
{
	if (!name)
		return WIFI_ENCRYPTION_UNKNOWN;
	if (!strcmp(name, WiFiEncryptionTypesHk[1]))
		return WIFI_ENCRYPTION_NONE;
	if (!strcmp(name, WiFiEncryptionTypesHk[2]))
		return WIFI_ENCRYPTION_WEP;
	if (!strcmp(name, WiFiEncryptionTypesHk[3]))
		return WIFI_ENCRYPTION_WPA;
	if (!strcmp(name, WiFiEncryptionTypesHk[4]))
		return WIFI_ENCRYPTION_WPA2;
	if (!strcmp(name, WiFiEncryptionTypesHk[5]))
		return WIFI_ENCRYPTION_WPA_ENT;
	if (!strcmp(name, WiFiEncryptionTypesHk[6]))
		return WIFI_ENCRYPTION_WPA2_ENT;
	if (!strcmp(name, WiFiEncryptionTypesHk[7]))
		return WIFI_ENCRYPTION_WPA_RADIUS;

	return WIFI_ENCRYPTION_UNKNOWN;
}

void SetDefaultWiFiParams(wifi_params* param)
{
	memset(param->ssid, 0, 64);
	memset(param->mac, 0, 32);
	memset(param->password, 0, 32);
	param->signal = PARAMETER_NOT_SET;
	param->encryption_caps = PARAMETER_NOT_SET;
	param->encryption = PARAMETER_NOT_SET;
	param->connected = false;
}

void SetDefaultWiFiList(wifi_list* wlist)
{
	for (int n = 0; n < MAX_WIFI_POINTS; n++)
		SetDefaultWiFiParams(&wlist->ap[n]);
}

std::string str_replace(std::string str, std::string old_str, std::string new_str)
{
	size_t old_str_len = old_str.length();
	size_t new_str_len = new_str.length();
	for (size_t p = str.find(old_str, 0); p != std::string::npos; p = str.find(old_str, p + new_str_len))
		str.replace(p, old_str_len, new_str);
	return str;
}

char* uri_reserved[] = { "%",    " ",   "!",   "\"",   "#",   "$",   "&",   "'",   "(",   ")",   "*",   "+",   ",",   "/",   ":",   ";",   "=",   "?",   "@",   "[",   "]",   "^",   "`",   "{",   "|",   "}",   "~" };
char* uri_encoded[]  = { "%25", "%20", "%21", "%22", "%23", "%24",   "%26", "%27", "%28", "%29", "%2A", "%2B", "%2C", "%2F", "%3A", "%3B", "%3D", "%3F", "%40", "%5B", "%5D", "%5E", "%60", "%7B", "%7C", "%7D", "%7E" };

std::string str_encode(std::string str)
{
	for (int n = 0; n < sizeof(uri_reserved) / sizeof(uri_reserved[0]); n++)
		str = str_replace(str, uri_reserved[n], uri_encoded[n]);
	return str;
}