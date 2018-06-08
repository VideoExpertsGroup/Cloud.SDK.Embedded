#ifndef __MLOG_H__
#define __MLOG_H__

#include "utils.h"

enum LogLevel {
	LOGLEVEL_ASSERT = 7,
	LOGLEVEL_ERROR = 6,
	LOGLEVEL_WARN = 5,
	LOGLEVEL_INFO = 4,
	LOGLEVEL_DEBUG = 3,
	LOGLEVEL_VERBOSE = 2
};

class MLog
{
	std::string tag;
	int level;

public:
	static LogLevel globalLevel;
	static bool isSignedWithDebugKey;
	static int ndbgFile;

	static void setLogEnable(bool b);

	MLog(const char *tag, int level);
	MLog(const char *tag, int level, const char * filename);
	MLog(std::string &tag, int level);
	MLog(std::string &tag, int level, const char * filename);

	void v(const char *fmt, ...) {
		va_list vl;
		va_start(vl, fmt);
		if (signedWithDebug() && level <= LOGLEVEL_VERBOSE) {
			dbg(level, tag.c_str(), fmt, vl);
		}
		va_end(vl);
	}

	void d(const char *fmt, ...) {
		va_list vl;
		va_start(vl, fmt);
		if (signedWithDebug() && level <= LOGLEVEL_DEBUG) {
			dbg(level, tag.c_str(), fmt, vl);
		}
		va_end(vl);
	}

	void i(const char *fmt, ...) {
		va_list vl;
		va_start(vl, fmt);
		if (signedWithDebug() && level <= LOGLEVEL_INFO) {
			dbg(level, tag.c_str(), fmt, vl);
		}
		va_end(vl);
	}

	void w(const char *fmt, ...) {
		va_list vl;
		va_start(vl, fmt);
		if (signedWithDebug() && level <= LOGLEVEL_WARN) {
			dbg(level, tag.c_str(), fmt, vl);
		}
		va_end(vl);
	}

	void e(const char *fmt, ...) {
		va_list vl;
		va_start(vl, fmt);
		if (signedWithDebug() && level <= LOGLEVEL_ERROR) {
			dbg(level, tag.c_str(), fmt, vl);
		}
		va_end(vl);
	}

protected:

	void _init(int level, const char * filename);

	bool signedWithDebug()
	{
		//force debug
		//isSignedWithDebugKey = true;

		return isSignedWithDebugKey;
	}

	void dbg(int level, const char *tag, const char *str, va_list vl);

};
#endif //__MLOG_H__