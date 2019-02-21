	
#ifdef WIN32
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include <process.h>
#include <signal.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#include <sys/stat.h> 
#include <fcntl.h>
#endif

#include <CloudSDK.h>
#include <CloudStreamerSDK.h>
#include <Interfaces/ICloudStreamerCallback.h>

//#define HIKVISION  (1)
//#define USE_FFSTREAMER_LIB (1)

#ifdef USE_FFSTREAMER_LIB
#include "../ffstreamer/fflib.h"
#ifdef _WIN32
#pragma comment (lib,"../../fflib/fflib.lib")
#endif //_WIN32
void* streamerinst = NULL;
#endif //USE_FFSTREAMER_LIB

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
#ifdef USE_FFSTREAMER_LIB
		if (streamerinst)
		{
			stream_stop(streamerinst);
			stream_term(streamerinst);
			streamerinst = NULL;
		}
#else
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
#endif
        bTerminate = 1;
//		exit(0);
	}
}

class CStreamerCallback : public ICloudStreamerCallback, public CUnk
{
//	const char *TAG = "CStreamerCallback";
//	const int LOG_LEVEL = 2; //Log.VERBOSE;
	MLog Log;

public:
	CStreamerCallback() : Log("CStreamerCallback", 2)
	{
		pid = -1;
	};

	virtual ~CStreamerCallback() {};

	void onStarted(std::string url_push) //Cloud gets ready for data, url_push == rtmp://...
	{
		Log.v("=onStarted rtmp url=%s", url_push.c_str());

		streamPushStart(url_push);
	}
	void onStopped()                
	{
		Log.v("=onStopped");
		streamPushStop();
	}
	void onError(int error)
	{
		Log.v("=onError %d", error);
	}
	void onCameraConnected() 
	{
		Log.v("=onCameraConnected");
	}
	void onCommand(std::string cmd)
	{
		Log.v("=onCommand cmd=%s", cmd.c_str());
	}

	int onRawMsg(std::string& data)
	{
		return 0;
	}
	void onUploadUrl(void* inst, std::string url, int refid)
	{
	}
	void onCamGetLog(std::string url)
	{
	}

private:

	int pid;
	int streamPushStart(std::string url_push) {

#ifdef USE_FFSTREAMER_LIB
		stream_init(&streamerinst, ARG1_CAMERA_URL.c_str(), url_push.c_str());
		stream_start(streamerinst);
#else

        std::string cmdline;
#ifdef HIKVISION
        std::string szffmpeg = "streamer";
#else
        std::string szffmpeg = "ffmpeg";
#endif

        if (strstr(ARG1_CAMERA_URL.c_str(), "http://"))
        {
            cmdline = szffmpeg + " -f h264 -i " + ARG1_CAMERA_URL + " -vcodec copy -acodec copy -f flv " + url_push;
        }
        else
        {
            cmdline = szffmpeg + " -i " + ARG1_CAMERA_URL + " -vcodec copy -acodec copy -f flv " + url_push;
        }

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
        char szProc[256] = {0};
		/*Spawn a child to run the program.*/
		pid = fork();
		if (pid == 0) { /* child process */
#ifdef HIKVISION
			sprintf(szProc, "%s%s", gszPath, szffmpeg.c_str());
			execv(szProc, &args[0]);
			fprintf(stderr, "szProc=%s\n", szProc);
#else
			execv("/usr/local/bin/ffmpeg", &args[0]);
#endif
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

            fprintf(stderr, "szProc=%s\n", szProc);
            fprintf(stderr, "execv err=%s\n", strerror(errno));

            for (size_t i = 0; i < args.size(); i++) {
                //Log.v(" args[%d]=%s", i, args[i]);
                delete[] args[i];
            }
            Log.v("=streamPushStart pid(%d)", pid);

#endif //USE_FFSTREAMER_LIB

		return pid;
	}
	int streamPushStop() {

#ifdef USE_FFSTREAMER_LIB
		if (streamerinst)
		{
			stream_stop(streamerinst);
		}
#else

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
#endif //USE_FFSTREAMER_LIB
		return pid;
	}

};

int main(int argc, char **argv)
{

    signal(SIGINT, signal_handler);

//    getchar();

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

    printf("szPath=%s\n", gszPath);

#ifdef _WIN32
    int nLogLvl = LOGLEVEL_VERBOSE;
#else
    int nLogLvl = LOGLEVEL_ASSERT;
#endif

    char* pLogLevel = getenv("LOGLEVEL");
    if (pLogLevel != NULL)
        nLogLvl = atoi(pLogLevel);

    CloudSDK::setLogLevel(nLogLvl);// 2);
    MLog Log("app", nLogLvl);// 2);

    Log.e("LOGLEVEL=%d\n", nLogLvl);

    Log.v("=>test_cloudstreamer %s\n", CloudSDK::getLibVersion());
/*
    if(argc < 3){
       Log.e("test_cloudstreamer. \n Usage: <camera_url> <access_token> \n Example: test_cloudstreamer rtsp://10.20.16.80 tokenstringAABBCCDD\n");
           return -1;
    }
*/

    if(argc < 3)
    {
        char* config_buf = NULL;
        int config_file_fd = 0; 
        int file_ok = 1;

        //const char* file_path_dav = "/dav/package/vxg_app/data.cfg";
        char file_path_dav[256];
        strcpy(file_path_dav, gszPath);
        strcat(file_path_dav, "data.cfg");

        config_file_fd = open(file_path_dav, 0, 0666);//fopen(file_path_dav, "rb");
        if(!config_file_fd)
        {
            Log.e("config_file_fd open \"%s\" failed - [%s]\n", file_path_dav, strerror(errno));
            file_ok = 0;
        }

        int file_len = lseek(config_file_fd, 0, SEEK_END);
        lseek(config_file_fd, 0, SEEK_SET);

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
        }

        close(config_file_fd);

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

        if(config_buf)delete[]config_buf;

//fprintf(stderr, "ARG1_CAMERA_URL = %s\n", ARG1_CAMERA_URL.c_str());
//fprintf(stderr, "ARG2_ACCESS_TOKEN = %s\n", ARG2_ACCESS_TOKEN.c_str());
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

//    getchar();

#ifdef USE_FFSTREAMER_LIB
	if (streamerinst)
	{
		stream_stop(streamerinst);
		stream_term(streamerinst);
		streamerinst = NULL;
	}
#endif //USE_FFSTREAMER_LIB

    Log.v("awaiting done\n");

	streamer->Release(); streamer = NULL;

	streamer_cb->Release(); streamer_cb = NULL;

	Log.v("<=test_cloudstreamer %s\n", CloudSDK::getLibVersion());

//	getchar();
	return 0;
}

