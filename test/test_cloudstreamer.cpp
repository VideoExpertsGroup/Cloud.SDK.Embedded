
#ifdef WIN32
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h> /* for wait */
#endif

#include <CloudSDK.h>
#include <CloudStreamerSDK.h>
#include <Interfaces/ICloudStreamerCallback.h>


std::string ARG1_CAMERA_URL;
std::string ARG2_ACCESS_TOKEN;


class CStreamerCallback : public ICloudStreamerCallback, public CUnk
{
	const char *TAG = "CStreamerCallback";
	const int LOG_LEVEL = 2; //Log.VERBOSE;
	MLog Log;

public:
	CStreamerCallback() : Log(TAG, LOG_LEVEL) {};
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

private:

	int pid = -1;
	int streamPushStart(std::string url_push) {
#ifndef WIN32
		/*Spawn a child to run the program.*/
		pid = fork();
		if (pid == 0) { /* child process */
			std::string cmdline = "ffmpeg -i " + ARG1_CAMERA_URL + " -vcodec copy -acodec copy -f flv " + url_push;

			std::vector<char *> args;
			std::istringstream iss(cmdline);

			std::string token;
			while(iss >> token) {
			    char *arg = new char[token.size() + 1];
			    copy(token.begin(), token.end(), arg);
			    arg[token.size()] = '\0';
			    args.push_back(arg);
			}
			args.push_back(0);
			execv("/usr/bin/ffmpeg", &args[0]);

			for(size_t i = 0; i < args.size(); i++){
                            //Log.v(" args[%d]=%s", i, args[i]);
			    delete[] args[i];
			}
			Log.v("=streamPushStart argv=%s", &cmdline[0]);
			//exit(127); /* only if execv fails */
		}
		Log.v("=streamPushStart pid(%d)", pid);
#endif
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
#endif
		return pid;
	}

};

int main(int argc, char **argv)
{
	CloudSDK::setLogEnable(true, "teststreamer.log");
	CloudSDK::setLogLevel(2);
	MLog Log("app", 2);

    Log.v("=>test_cloudstreamer %s\n", CloudSDK::getLibVersion());
    if(argc < 3){
       Log.e("test_cloudstreamer. \n Usage: <camera_url> <access_token> \n Example: test_cloudstreamer rtsp://10.20.16.80 tokenstringAABBCCDD\n");
           return -1;
    }
    ARG1_CAMERA_URL = argv[1];
	ARG2_ACCESS_TOKEN = argv[2];
	Log.v("=test_cloudstreamer args(%s %s)", ARG1_CAMERA_URL.c_str(), ARG2_ACCESS_TOKEN.c_str());

	CStreamerCallback *streamer_cb = new CStreamerCallback();
    CloudStreamerSDK *streamer = new CloudStreamerSDK(streamer_cb);
	streamer->setSource(ARG2_ACCESS_TOKEN);
    streamer->Start();

	int ticks = 1000;
	while (ticks--) {
#ifdef _WIN32
		Sleep(1000);
#else
		usleep(1000*1000);
#endif
	}

    Log.v("awaiting done\n");

	streamer->Release(); streamer = NULL;

	streamer_cb->Release(); streamer_cb = NULL;

	Log.v("<=test_cloudstreamer %s\n", CloudSDK::getLibVersion());

	getchar();
	return 0;
}

