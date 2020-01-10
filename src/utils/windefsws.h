#ifndef __WINDEFWS_INC__
#define __WINDEFWS_INC__

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <errno.h>
#include <objbase.h>
#include "windirent.h"

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
#include <sstream>
#include <algorithm>

using namespace std;

#ifndef _WIN32
#include <sys/time.h>
//#include <uuid/uuid.h>

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
extern DWORD GetCurrentThreadId();

long	InterlockedIncrement(long *lpAddend);
long	InterlockedDecrement(long *lpAddend);

void Sleep(unsigned long msec);

int ThreadJoin(pthread_t thread, void **value_ptr);

#define THREADRET void*

#else

#ifdef __cplusplus
extern "C" {
#endif	

typedef struct pthread_tag {
	HANDLE handle;
} pthread_t;

typedef struct pthread_mutex_tag {
	HANDLE handle;
} pthread_mutex_t;

/* stub */
typedef struct pthread_attr_tag {
	int attr;
} pthread_attr_t;

typedef struct pthread_mutexattr_tag {
	int attr;
} pthread_mutexattr_t;

typedef DWORD pthread_key_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);
void pthread_exit(void *value_ptr);
int pthread_join(pthread_t thread, void **value_ptr);
pthread_t pthread_self(void);
int pthread_detach(pthread_t thread);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int ThreadJoin(HANDLE thread, void **value_ptr);

#define THREADRET DWORD WINAPI

time_t timegm(struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif

int GetDiskInfo(const char *anyfile, unsigned long long* ullFree, unsigned long long* ullTotal);
unsigned long long getTotalSystemMemory();
size_t getCurrentRSS();
int getRSS(int* curr, int* max, int* growth);
std::string GetMachineName();
void create_guid(char guid[]);


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

#ifndef SAFE_FREE
#define SAFE_FREE(p)  if (p) {free (p); p = NULL;}
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

template <class T>
string fto_string(T param)
{
	std::ostringstream ss;
	ss << param;
	return ss.str();
}

#ifndef _WIN32
#include <limits>
template<typename T>
void set_max(T& val) { val = std::numeric_limits<T>::max(); }
template<typename T>
void set_min(T& val) { val = std::numeric_limits<T>::min(); }
#endif

#endif  // defined(__cplusplus)


#endif //__WINDEFWS_INC__