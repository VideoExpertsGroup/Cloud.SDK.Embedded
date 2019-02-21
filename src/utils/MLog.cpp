
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


//#define SHOW_THREADS

#define MAX_DBG_LEN 8*1024
//#define MAX_DBG_LEN 128*1024
//#define OUTPUT_DBG_STRING
#define OUTPUT_PRINTF

#define MAX_LOG_SIZE	(64*1024)

LogLevel MLog::globalLevel = LOGLEVEL_ASSERT;
bool MLog::isSignedWithDebugKey = false;
int MLog::ndbgFile = -1;
char MLog::FileName[256] = {0};
int MLog::nMaxLogFileSize = MAX_LOG_SIZE;
CFileRing MLog::m_fr;
DWORD log_start_timestamp = 0;

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

	char* pLogSize = getenv("MAX_LOG_SIZE");
	if (pLogSize != NULL)
		nMaxLogFileSize = atoi(pLogSize);

	int nMaxLogFileIdx = 5;
	char* pLogIdx = getenv("MAX_LOG_IDX");
	if (pLogIdx != NULL)
		nMaxLogFileIdx = atoi(pLogIdx);

	m_fr.Init(filename, nMaxLogFileSize, nMaxLogFileIdx);

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

unsigned int get_timedelta()
{
	if (log_start_timestamp == 0)log_start_timestamp = timeGetTime();
	return (unsigned int)(timeGetTime() - log_start_timestamp);
}


long get_msec()
{
#ifndef _WIN32
	timeval time;
	gettimeofday(&time, NULL);
	return time.tv_usec / 1000;
#else
	SYSTEMTIME time;
	GetSystemTime(&time);
	return time.wMilliseconds;
#endif
}

void MLog::dbg(int level, const char *tag, const char * str, va_list vl)
{
	char buf_arg[MAX_DBG_LEN] = { 0 };
	char buf_prt[MAX_DBG_LEN] = { 0 };
	time_t tTime = 0; struct tm* pt = 0;

	//add time 
	time(&tTime); pt = localtime(&tTime);

	vsnprintf(buf_arg, MAX_DBG_LEN, str, vl);

#ifdef SHOW_THREADS
	unsigned int t = get_timedelta();
	DWORD dwThread = GetCurrentThreadId();
	snprintf(buf_prt, MAX_DBG_LEN, "%.8d:%.3d %02d:%02d:%02d [0x%8.8X] %s %s\n", t / 1000, t % 1000, pt->tm_hour, pt->tm_min, pt->tm_sec, dwThread, tag, buf_arg);
#else
	snprintf(buf_prt, MAX_DBG_LEN, "%02d:%02d:%02d.%.03d %s %s\n", pt->tm_hour, pt->tm_min, pt->tm_sec, get_msec() , tag, buf_arg);
#endif



#ifdef OUTPUT_DBG_STRING
	OutputDebugString(buf_prt);
#endif
#ifdef OUTPUT_PRINTF
	printf("%s",buf_prt);
#endif
	m_fr.Write(buf_prt, strlen(buf_prt));

}

int MLog::GetLogData(unsigned char** pData, unsigned int* nSize)
{

	if(!pData || !nSize)
		return -1;

	return 	m_fr.GetData(pData, nSize);
}


