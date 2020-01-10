

#include "utils.h"
#include "base64.h"
#include "jansson.h"

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

	string url = "";
	const char *szApi = json_string_value(json_object_get(jacc, "api"));
	const char *szApiPort = json_string_value(json_object_get(jacc, "api_p"));
	if (szApi)
	{
		Log.d("szApi==%s", szApi);
		url = szApi;
	}
	else
	{
		url = "web.skyvr.videoexpertsgroup.com";
		Log.e("szApi==NULL, use default url %s", url.c_str());
	}

	if (szApiPort)
	{
		url += ":";
		url += szApiPort;
	}

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
	img->white_balance_types = (char**)PARAMETER_NOT_SET;
	memset(img->white_balance, 0, PARAMETER_CHAR_MAX_LENGTH);
	img->noise_reduction_level = PARAMETER_NOT_SET;
	img->noise_reduction_types = (char**)PARAMETER_NOT_SET;
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
	"G.711alaw",	"G711A",
	"G.711ulaw",	"G711U",
	"G.726",		"",
	"G.729",		"",
	"G.729a",		"",
	"G.729b",		"",
	"PCM",			"",
	"MP3",			"MP3",
	"AC3",			"",
	"AAC",			"AAC",
	"ADPCM",		"ADPCM",
	"MP2L2",		""
	"",				""
};
//[“RAW”, “ADPCM”, “MP3”, “NELLY8”, “NELLY16”, “NELLY”, “G711A”, “G711U”, “AAC”, “SPEEX”, “UNKNOWN”]

const char* ConvertAudioName(const char* name)
{
	if (!name)
		return NULL;

	int n = 0;
	while (strlen(audNameTable[n].NameCam))
	{
		if (!strcmp(audNameTable[n].NameCam, name))
		{
			return(audNameTable[n].NameServ);
			break;
		}
		n++;
	}
	return NULL;
}
