
#ifndef __CLOUDSDK_H__
#define __CLOUDSDK_H__

#include "utils/utils.h"
#include "utils/Context.h"
#include "utils/MLog.h"

#define CLOUDSDK_LIB_VERSION "2.0.101_20200106"

class CloudSDK
{
	static Context *mContext;
	
public:
	static const char * getLibVersion() { return CLOUDSDK_LIB_VERSION; };

	static void setContext(Context *context) {
		mContext = context;
	}
	static void *getContext() {
		return mContext;
	}

	static void setLogEnable(bool b, const char * filename) {
		MLog::setLogEnable(b);
		if (b) {
			MLog Log("CloudSDK", 2, filename);
			Log.d("CLOUDSDK_LIB_VERSION=%s", CLOUDSDK_LIB_VERSION);
		}
	}
	static bool getLogEnable() {
		return MLog::isSignedWithDebugKey;
	}

	static void setLogLevel(int level) {
		MLog::globalLevel = (LogLevel)level;
	}
	static int getLogLevel() {
		return MLog::globalLevel;
	}

};

#endif //__CLOUDSDK_H__