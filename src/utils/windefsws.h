#ifndef __WINDEFWS_INC__
#define __WINDEFWS_INC__

#ifdef _WIN32
#include <windows.h>
#endif

#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <vector>
#include <list>
#include <iostream>

#ifndef _WIN32
#include <sys/time.h>

typedef unsigned char  BYTE;
typedef unsigned int   UINT;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef DWORD*		LPDWORD;
typedef void*		LPVOID;
typedef long		LONG;
typedef pthread_t HANDLE;
typedef int		BOOL;

typedef struct CRITICAL_SECTION {
	int	nLockCount;
	pthread_mutex_t sem_id;
	pthread_t thread_id;
} CRITICAL_SECTION;

typedef void* (*PTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

extern void InitializeCriticalSection(CRITICAL_SECTION* lock);
extern void DeleteCriticalSection(CRITICAL_SECTION* lock);
extern void EnterCriticalSection(CRITICAL_SECTION* lock);
extern void LeaveCriticalSection(CRITICAL_SECTION* lock);
extern HANDLE CreateThread(void* pSecAttr, size_t nStackSize, LPTHREAD_START_ROUTINE lpStartAddress, void* lpParameter, DWORD dwFlags, DWORD* lpThreadId);
extern BOOL TerminateThread(HANDLE hThread, DWORD dwExitCode);
extern DWORD timeGetTime();

long	InterlockedIncrement(long *lpAddend);
long	InterlockedDecrement(long *lpAddend);

void Sleep(unsigned long msec);


#endif

#ifndef FALSE
	typedef int BOOL;
	#define FALSE false
	#define TRUE true
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)  if (p) {delete[] (p); p = NULL;}
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)  if (p) {delete (p); p = NULL;}
#endif

#if defined(__cplusplus)
class CCritSec
{
	CRITICAL_SECTION	m_CritSec;

public:
	CCritSec() { ::InitializeCriticalSection(&m_CritSec); };
	~CCritSec() { ::DeleteCriticalSection(&m_CritSec); };
	void Lock() { ::EnterCriticalSection(&m_CritSec); };
	void Unlock() { ::LeaveCriticalSection(&m_CritSec); };
};


class CAutoLock
{
protected:
	CCritSec * m_pLock;

public:
	CAutoLock(CCritSec * plock) { m_pLock = plock; m_pLock->Lock(); };
	~CAutoLock() { m_pLock->Unlock(); };
};
#endif  // defined(__cplusplus)

//extern void logprintf(WSWRAP_DEBUG_LEVELS level, const char * msg, ...);

#endif //__WINDEFWS_INC__