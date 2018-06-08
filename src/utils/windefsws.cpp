
//#include "stdafx.h"
//#include "windefsws.h"
#include "utils.h"


#define CURRENT_DEBUG_LEVEL  DEBUG_LEVEL_DEBUG


DWORD       start_timestamp = 0;
BOOL        console_logging = 1;
BOOL        file_logging    = 1;
FILE*       pLogFile        = NULL;
const char* log_file_name   = "./log.txt";


#ifndef _WIN32

void InitializeCriticalSection(CRITICAL_SECTION* lock)
{
	pthread_mutex_init(&lock->sem_id, NULL);
	lock->nLockCount = 0;
}

void DeleteCriticalSection(CRITICAL_SECTION* lock)
{
	pthread_mutex_destroy(&lock->sem_id);
}

void EnterCriticalSection(CRITICAL_SECTION* lock)
{
	pthread_t id = pthread_self();
	if (lock->nLockCount && lock->thread_id == id)
	{
		lock->nLockCount++;
		return;
	}
	pthread_mutex_lock(&lock->sem_id);
	lock->thread_id = id;
	lock->nLockCount++;
}

void LeaveCriticalSection(CRITICAL_SECTION* lock)
{
	pthread_t id = pthread_self();
	lock->nLockCount--;
	if (lock->thread_id == id && lock->nLockCount == 0)
		pthread_mutex_unlock(&lock->sem_id);
}

HANDLE CreateThread(void* pSecAttr, size_t nStackSize, LPTHREAD_START_ROUTINE lpStartAddress, void* lpParameter, DWORD dwFlags, DWORD* lpThreadId)
{
	pthread_t tidp;
	pthread_attr_t  attr = { 0 };
	HANDLE ret = 0;
	if (nStackSize)
	{
		pthread_attr_setstacksize(&attr, nStackSize);
		pthread_create(&tidp, &attr, lpStartAddress, lpParameter);
	}
	else
	{
		pthread_create(&tidp, NULL, lpStartAddress, lpParameter);
	}

	*lpThreadId = tidp;
	return tidp;
}

BOOL TerminateThread(HANDLE hThread, DWORD dwExitCode)
{
	void *res = NULL;
	int ret = 0;
	if (hThread)
	{
		ret = pthread_join(hThread, &res);
		if (ret != 0)pthread_cancel(hThread);
	}
	return (ret==0);
}

DWORD timeGetTime()
{
	timeval tim;
	gettimeofday(&tim, NULL);
	unsigned long ms = (tim.tv_sec * 1000u) + (long)(tim.tv_usec / 1000.0);
	return ms;
}

void Sleep(unsigned long msec)
{
	struct timespec timeout0;
	struct timespec timeout1;
	struct timespec* tmp;
	struct timespec* t0 = &timeout0;
	struct timespec* t1 = &timeout1;

	t0->tv_sec = msec / 1000;
	t0->tv_nsec = (msec % 1000) * (1000 * 1000);

	while ((nanosleep(t0, t1) == (-1)) && (errno == EINTR))
	{
		tmp = t0;
		t0 = t1;
		t1 = tmp;
	}
}

DWORD GetCurrentThreadId()
{
	return pthread_self();
}

long	InterlockedIncrement(long *lpAddend)
{
//  return __atomic_add_fetch(lpAddend, 1, __ATOMIC_SEQ_CST);
    return __sync_add_and_fetch(lpAddend, 1);
}

long	InterlockedDecrement(long *lpAddend)
{
//  return __atomic_sub_fetch(lpAddend, 1, __ATOMIC_SEQ_CST);
    return __sync_sub_and_fetch(lpAddend, 1);
}


#endif //_WIN32

/*
unsigned int get_timedelta()
{
	if (start_timestamp == 0)start_timestamp = timeGetTime();
	return (unsigned int)(timeGetTime() - start_timestamp);
}


void logprintf(WSWRAP_DEBUG_LEVELS level, const char * msg, ...)
{
//    return;
	if (!console_logging && !file_logging)return;

	if (level > CURRENT_DEBUG_LEVEL)return;

	va_list	ap;
	char str[2048];
	va_start(ap, msg);
	vsprintf(str, msg, ap);
	va_end(ap);
	char text[2048];

	unsigned int t = get_timedelta();
	DWORD dwThread = GetCurrentThreadId();
	sprintf(text, "%.8d:%.3d [0x%4.4X]  %s", t / 1000, t % 1000, dwThread, str);

	if (console_logging)printf(text);

	if (file_logging)
	{
		if (!pLogFile)
		{
			remove(log_file_name);
			pLogFile = fopen(log_file_name, "w+b");
		}

		if (pLogFile)
		{
			fseek(pLogFile, 0, SEEK_END);
			fwrite(text, strlen(text), 1, pLogFile);
		}
	}
}
*/
