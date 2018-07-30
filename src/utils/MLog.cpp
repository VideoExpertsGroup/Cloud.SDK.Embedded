
#include "MLog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h> 

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifdef _DEBUG
#include <fcntl.h>
#include <sys\stat.h>
#include <io.h>
#include <stdarg.h>
#include <tchar.h>
#include <stdio.h>
#endif //_DEBUG


#define MAX_DBG_LEN 8*1024
//#define OUTPUT_DBG_STRING
#define OUTPUT_PRINTF

LogLevel MLog::globalLevel = LOGLEVEL_ASSERT;
bool MLog::isSignedWithDebugKey = false;
int MLog::ndbgFile = -1;

void MLog::setLogEnable(bool b)
{
	MLog::isSignedWithDebugKey = b;
}

void MLog::_init(int level, const char * filename)
{
//	int m = std::min<int>(globalLevel, level);
//  this->level = signedWithDebug() ? m : globalLevel;
    int m = std::max<int>(globalLevel, level);
    this->level = signedWithDebug() ? m : LOGLEVEL_ASSERT;

	if (filename && ndbgFile == -1) {
#ifdef WIN32
		char mname[255]; char *find;
		int flags1, flags2;
		mname[0] = 0;
		GetModuleFileNameA(NULL, mname, 255); if (!strlen(mname)) return;
		find = strrchr(mname, '.');
		find[0] = 0;
		strcat(mname, filename);
		flags1 = O_RDWR | O_TEXT | O_CREAT;
		flags2 = S_IREAD | S_IWRITE;
		ndbgFile = open(mname, flags1, flags2);
		//strcpy(filename, mname);
#else
		int flags1, flags2;
		flags1 = O_RDWR | O_CREAT;
		flags2 = S_IREAD | S_IWRITE;
		ndbgFile = open(filename, flags1, flags2);
#endif //WIN32		
	}

}

MLog::MLog(const char *tag, int level) {
	this->tag = tag;
	_init(level, NULL);
}

MLog::MLog(const char *tag, int level, const char * filename) {
	this->tag = tag;
	_init(level, filename);
}

MLog::MLog(std::string &tag, int level) {
	this->tag = tag;
	_init(level, NULL);
}

MLog::MLog(std::string &tag, int level, const char * filename) {
	this->tag = tag;
	_init(level, filename);
}


void MLog::dbg(int level, const char *tag, const char * str, va_list vl)
{
	char buf_arg[MAX_DBG_LEN] = { 0 };
	char buf_prt[MAX_DBG_LEN] = { 0 };
	time_t tTime = 0; struct tm* pt = 0;

	//add time 
	time(&tTime); pt = localtime(&tTime);

	vsnprintf(buf_arg, MAX_DBG_LEN, str, vl);
	snprintf(buf_prt, MAX_DBG_LEN, "%s %02d:%02d:%02d %s\n", tag, pt->tm_hour, pt->tm_min, pt->tm_sec, buf_arg);

#ifdef OUTPUT_DBG_STRING
	OutputDebugString(buf_prt);
#endif
#ifdef OUTPUT_PRINTF
	printf(buf_prt);
#endif

	if (ndbgFile != -1) {
		lseek(ndbgFile, 0, SEEK_END);
		write(ndbgFile, buf_prt, strlen(buf_prt));
	}
}
