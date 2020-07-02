	
#ifdef WIN32
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include <process.h>
#include <signal.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <fcntl.h>
#endif

#include <CloudSDK.h>
#include <CloudStreamerSDK.h>
#include <Interfaces/ICloudStreamerCallback.h>

std::string ARG1_CAMERA_URL;
std::string ARG2_ACCESS_TOKEN;

char gszPath[256]={0};
int ffmpeg_pid = 0;
int bTerminate = 0;

#ifdef _WIN32
void EnableDebugPriv(void)
{
    HANDLE              hToken;
    LUID                SeDebugNameValue;
    TOKEN_PRIVILEGES    TokenPrivileges;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &SeDebugNameValue))
        {
            TokenPrivileges.PrivilegeCount = 1;
            TokenPrivileges.Privileges[0].Luid = SeDebugNameValue;
            TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if (AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
            {
                CloseHandle(hToken);
            }
            else
            {
                CloseHandle(hToken);
                throw std::exception("Couldn't adjust token privileges!");
            }
        }
        else
        {
            CloseHandle(hToken);
            throw std::exception("Couldn't look up privilege value!");
        }
    }
    else
    {
        throw std::exception("Couldn't open process token!");
    }
}
#endif

static void signal_handler(int sig )
{
	if (sig == SIGINT || sig == SIGTERM)
	{ 
		fprintf(stderr, "\nSIGTERM received\n\n");
		if (ffmpeg_pid)
		{
#ifndef _WIN32
			kill(ffmpeg_pid, SIGTERM);
#else
			EnableDebugPriv();
			HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ffmpeg_pid);
			TerminateProcess(hProc, 0);
			CloseHandle(hProc);
#endif
			ffmpeg_pid = 0;
		}
		bTerminate = 1;
	}
}

class CStreamerCallback : public ICloudStreamerCallback, public CUnk
{
	MLog Log;

public:
	CStreamerCallback() : Log("CStreamerCallback", 2)
	{
		pid = -1;
	};

	virtual ~CStreamerCallback() {};


	// Start rtmp publishing
	void onStarted(std::string url_push) //Cloud gets ready for data, url_push == rtmp://...
	{
		Log.v("=onStarted rtmp url=%s", url_push.c_str());

		streamPushStart(url_push);
	}

	// Stop rtmp publishing
	void onStopped()                
	{
		Log.v("=onStopped");
		streamPushStop();
	}

	// Error with cloud connection occured
	void onError(int error)
	{
		Log.v("=onError %d", error);
	}

	//Called when SDK successfully connected to the server
	void onCameraConnected() 
	{
		Log.v("=onCameraConnected");
	}

	void onCommand(std::string cmd)
	{
		Log.v("=onCommand cmd=%s", cmd.c_str());
	}

	// Handler for the raw messages from the server
	int onRawMsg(std::string& data)
	{
		return 0;
	}

	// Called when SDK receives direct upload url from the server
	void onUploadUrl(void* inst, std::string url, int refid)
	{

	}

	// Server provides url to upload log file using http POST
	void onCamGetLog(std::string url)
	{

	}

	// Should return current time from the server
	time_t onGetCloudTime()
	{
		return 0;
	}

	// Server sets "record by event" recording mode
	void onSetRecByEventsMode(bool bEventsMode)
	{
	}

	// Should set provided resolution, fps, bitrate, cbr/vbr mode and vbr quality on the camera.
	void setStreamConfig(CameraManagerConfig &config)
	{
	}

	// Called when server requested stream config and stream caps
	void onCommand(std::string data, std::string &retVal)
	{
	}

	// Set username and password for access to the camera
	void setHttpUser(std::string name, std::string password)
	{
	}

	// Switching from LOGLEVEL_ERROR to LOGLEVEL_DEBUG and vice versa
	void SetLogEnable(bool bEnable)
	{

	}

	// Set brightness/contrast/saturation on the camera
	void SetImageParams(image_params_t* img)
	{
	}

	// Get brightness/contrast/saturation values from the camera
	void GetImageParams(image_params_t* img)
	{
	}

	// Set camera motion detector sensitivity, row/cols granularity and motion grid
	void SetMotionParams(motion_params_t* mpr)
	{
	}

	// Get camera motion detector sensitivity, row/cols granularity and motion grid
	void GetMotionParams(motion_params_t* mpr)
	{
	}

	// Get camera timezone
	void GetTimeZone(char* time_zone_str)
	{
	}

	// Set camera timezone
	void SetTimeZone(const char* time_zone_str)
	{
	}

	// Get pan/tilt/zoom support from the camera
	void GetPTZParams(ptz_caps_t* ptz)
	{
	}

	// Get speaker volume, mic gain and audio compression type from the camera
	void GetAudioParams(audio_settings_t* audset)
	{
	}

	// Set speaker volume, mic gain and audio compression
	void SetAudioParams(audio_settings_t* audset)
	{
	}

	// send PTZ commands top/bottom/right/left/zoom_in/zoom_out commands to the camera
	void SendPTZCommand(ptz_command_t* ptz)
	{
	}

	// Get camera current OSD parameters such as date format, time format, font size, enabled/disabled etc
	void GetOSDParams(osd_settings_t* osd)
	{
	}

	// Set camera OSD parameters
	void SetOSDParams(osd_settings_t* osd)
	{
	}

	// Directly sends command to the camera. Currently only "reboot" implemented.
	int SendCameraCommand(cam_command_t* ccmd)
	{
		return 0;
	}

	// Called when Trigger event received from the server 
	void TriggerEvent(void* inst, string evt, string meta)
	{
	}

	// Enable/Disable streaming, recording, snapshots and mp4 uploading and events.
	void SetActivity(bool bEnable)
	{
	}

private:

	int pid;
	int streamPushStart(std::string url_push)
	{

	        std::string cmdline;
        	std::string szffmpeg = "./ffmpeg";

	        if (strstr(ARG1_CAMERA_URL.c_str(), "http://"))
        	{
	            cmdline = szffmpeg + " -f h264 -i " + ARG1_CAMERA_URL + " -vcodec copy -acodec copy -f flv " + url_push;
        	}
	        else
        	{
	            cmdline = szffmpeg + " -i " + ARG1_CAMERA_URL + " -vcodec copy -acodec copy -f flv " + url_push + " -v 99";
        	}

		Log.v("=streamPushStart cmdline=%s", cmdline.c_str());

	        std::vector<char *> args;
        	std::istringstream iss(cmdline);

	        std::string token;
        	while (iss >> token) {
			char *arg = new char[token.size() + 1];
			copy(token.begin(), token.end(), arg);
			arg[token.size()] = '\0';
			args.push_back(arg);
		}
		args.push_back(0);

#ifndef WIN32
		// Spawn a child to run the program.
		pid = fork();
		if (pid == 0)
		{ // child process 
			execvp("./ffmpeg", &args[0]);
		}
		else
		{
			ffmpeg_pid = pid;
		}

		Log.v("=streamPushStart pid(%d)", pid);
#else //WIN32
		char szProc[256];
		sprintf(szProc, "%s%s", gszPath, "ffmpeg.exe");

		STARTUPINFOA si; memset(&si, 0, sizeof(si));
		PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
		si.cb = sizeof(si);

		if (CreateProcessA(szProc,
			(LPSTR)cmdline.c_str(),
			NULL,             // Process handle not inheritable. 
			NULL,             // Thread handle not inheritable. 
			FALSE,            // Set handle inheritance to FALSE. 
			0,                // No creation flags. 
			NULL,             // Use parent's environment block. 
			NULL,             // Use parent's starting directory. 
			&si,              // Pointer to STARTUPINFO structure.
			&pi)             // Pointer to PROCESS_INFORMATION structure.
		)
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			pid = pi.dwProcessId;
			ffmpeg_pid = pi.dwProcessId;
		}
#endif //WIN32

		Log.e("execv err=%s\n", strerror(errno));

		for (size_t i = 0; i < args.size(); i++) {
			delete[] args[i];
		}

		Log.v("=streamPushStart pid(%d)", pid);

		return pid;
	}

	int streamPushStop() {

#ifndef WIN32
		if(pid != -1){
			Log.v("=>streamPushStop pid(%d)", pid);
			kill(pid, SIGKILL);
			waitpid(pid,0,0); /* wait for child to exit */
			Log.v("<=streamPushStop pid(%d)", pid);
			pid = -1;
		}
#else
		if (ffmpeg_pid)
		{
			EnableDebugPriv();
			HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ffmpeg_pid);
			TerminateProcess(hProc, 0);
			CloseHandle(hProc);
			ffmpeg_pid = 0;
		}
#endif
		return pid;
	}

	
	// Application should receive rtmp audio stream from this url and play it on the camera
	void StartBackward(string url)
	{
		Log.d("%s", __FUNCTION__);
	}
	// Application should stops playback backward audio on the camera
	void StopBackward(string url)
	{
		Log.d("%s", __FUNCTION__);
	}

	//Application should start/stop generate events with provided nmae and period
	void SetPeriodicEvents(const char* name, int period, bool active)
	{
		Log.d("%s", __FUNCTION__);
	}
	// Gets pre-event and post-event durations for local mp4 recording
	void GetEventLimits(time_t* pre, time_t* post)
	{
		Log.d("%s", __FUNCTION__);
	}
	// Sets pre-event and post-event durations for local mp4 recording
	void SetEventLimits(time_t pre, time_t post)
	{
		Log.d("%s", __FUNCTION__);
	}

	// On this command application should fills the list of available wi-fi access points
	void GetWiFiList(wifi_list_t* wifilist)
	{
		Log.d("%s", __FUNCTION__);
	}

	// Camera need to be switched to provided acess point
	void SetCurrenWiFi(wifi_params* params)
	{
		Log.d("%s", __FUNCTION__);
	}

	// Camera should generate snapshot and POST it to this url
	void UpdatePreview(std::string url)
	{
		Log.d("%s", __FUNCTION__);
	}
};

int main(int argc, char **argv)
{

	signal(SIGINT, signal_handler);

	char szLog[256]={0};
	strcpy(gszPath, argv[0]);
	char* z = strrchr(gszPath,'/');//linux
	if (!z)z = strrchr(gszPath, '\\');//windows
	if(z)
		z[1] = 0;
	else
		strcpy(gszPath,"./");

	sprintf(szLog, "%s%s", gszPath, "streamer.log");
	CloudSDK::setLogEnable(true, szLog);

//	printf("szPath=%s\n", gszPath);

	int nLogLvl = LOGLEVEL_VERBOSE;

	char* pLogLevel = getenv("LOGLEVEL");
	if (pLogLevel != NULL)
		nLogLvl = atoi(pLogLevel);

	CloudSDK::setLogLevel(nLogLvl);
	MLog Log("app", nLogLvl);

	Log.e("LOGLEVEL=%d\n", nLogLvl);

	Log.v("=>test_cloudstreamer %s\n", CloudSDK::getLibVersion());

	if(argc < 3)
	{
		char* config_buf = NULL;
		int config_file_fd = 0; 
		int file_ok = 1;

		//const char* file_path_dav = "/dav/package/vxg_app/data.cfg";
		char file_path_dav[256];
		strcpy(file_path_dav, gszPath);
		strcat(file_path_dav, "data.cfg");

		int file_len = 0;
		config_file_fd = open(file_path_dav, 0, 0666);//fopen(file_path_dav, "rb");
		if(config_file_fd < 0)
		{
			Log.e("config_file_fd open \"%s\" failed - [%s]\n", file_path_dav, strerror(errno));
			file_ok = 0;
		}
		else
		{
			file_len = lseek(config_file_fd, 0, SEEK_END);
			lseek(config_file_fd, 0, SEEK_SET);
		}

		//fprintf(stderr,"config_file_fd=%d, file_len=%d\n", config_file_fd, file_len);

		if(0==file_len)
		{
			file_ok = 0;
		}     	
		else
		{
			int read_len = 0;
			config_buf = new char[file_len+1];
			memset(config_buf, 0, file_len+1);
			if((read_len = read(config_file_fd, config_buf, file_len)) <= 0)
			{
				Log.e("read config failed - [%s] \n", strerror(errno));
				file_ok = 0;
			}
			else
			{
				Log.v("config_buf=%s, file_len=%d, read_len=%d\n", config_buf, file_len, read_len);
			}
			close(config_file_fd);
		}


		if(!file_ok)
		{
			Log.e("test_cloudstreamer. \n Usage: <camera_url> <access_token> \n Example: test_cloudstreamer rtsp://10.20.16.80 tokenstringAABBCCDD\n");
			return -1;
		}

		Log.v("test_cloudstreamer. json read start.\n");

		json_error_t jerr;
		json_t *root = json_loads(config_buf, 0, &jerr);
		std::string szToken = json_string_value(json_object_get(root, "vxgAccessToken"));
		std::string szUser  = json_string_value(json_object_get(root, "vxgCameraUsername"));
		std::string szPassword = json_string_value(json_object_get(root, "vxgCameraPassword"));
		json_decref(root);

		Log.v("test_cloudstreamer. json read finish.\n");

		ARG2_ACCESS_TOKEN = szToken;

		if(szPassword.length())
			ARG1_CAMERA_URL = "rtsp://" +  szUser + ":" + szPassword + "@127.0.0.1";
		else
			ARG1_CAMERA_URL = "rtsp://127.0.0.1";

		Log.v("test_cloudstreamer. ARG1_CAMERA_URL=%s.\n", ARG1_CAMERA_URL.c_str());

		if(config_buf)
			delete[]config_buf;
	}
	else
	{
		ARG1_CAMERA_URL = argv[1];
		ARG2_ACCESS_TOKEN = argv[2];
	}

	Log.v("=test_cloudstreamer args(%s %s)", ARG1_CAMERA_URL.c_str(), ARG2_ACCESS_TOKEN.c_str());
	CStreamerCallback *streamer_cb = new CStreamerCallback();
	CloudStreamerSDK *streamer = new CloudStreamerSDK(streamer_cb);
	streamer->setSource(ARG2_ACCESS_TOKEN);
	streamer->Start();

	while (!bTerminate)
	{
		Sleep(500);
	}

	Log.v("awaiting done\n");

	streamer->Release(); streamer = NULL;

	streamer_cb->Release(); streamer_cb = NULL;

	Log.v("<=test_cloudstreamer %s\n", CloudSDK::getLibVersion());

//	getchar();
	return 0;
}

