
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

	pthread_detach(tidp);

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
#ifdef OLDCPP
	return ++*lpAddend;
#else
//	return __atomic_add_fetch(lpAddend, 1, __ATOMIC_SEQ_CST);
	return __sync_add_and_fetch(lpAddend, 1);
#endif
}

long	InterlockedDecrement(long *lpAddend)
{
#ifdef OLDCPP
	return --*lpAddend;
#else
//	return __atomic_sub_fetch(lpAddend, 1, __ATOMIC_SEQ_CST);
	return __sync_sub_and_fetch(lpAddend, 1);
#endif
}


int ThreadJoin(pthread_t thread, void **value_ptr)
{
	return pthread_join(thread, value_ptr);
}

#include <sys/vfs.h>
int GetDiskInfo(const char *anyfile, unsigned long long* ullFree, unsigned long long* ullTotal)
{
	struct statfs64 buf;
	int err = statfs64(anyfile, &buf);
	*ullTotal = buf.f_blocks * buf.f_bsize;
	*ullFree = buf.f_bfree * buf.f_bsize;
	if (err != 0)
		return errno;
	return 0;
}

#include <unistd.h>

unsigned long long getTotalSystemMemory()
{
/*
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	return pages * page_size;
*/
/*
#include <sys/sysinfo.h>
	struct sysinfo sys_info;
	unsigned long long total_ram = 0;
	if (sysinfo(&sys_info) != -1)
		total_ram = ((uint64_t)sys_info.totalram * sys_info.mem_unit) / 1024;
	return total_ram;
*/
	FILE *meminfo = fopen("/proc/meminfo", "r");
	if (meminfo == NULL)
		return 0;

	char line[256];
	while (fgets(line, sizeof(line), meminfo))
	{
		int ram;
		if (sscanf(line, "MemTotal: %d kB", &ram) == 1)
		{
			fclose(meminfo);
			return ram * 1024;
		}
	}
	fclose(meminfo);
	return 0;
}

#else

typedef void(*windows_thread)(void *);

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
	uintptr_t handle = _beginthread((windows_thread)start_routine, 0, arg);
	thread->handle = (HANDLE)handle;
	if (thread->handle == (HANDLE)-1) {
		return 1;
	}
	else {
		return 0;
	}
}

int pthread_detach(pthread_t thread)
{
	/* Do nothing */
	return 0;
}

void pthread_exit(void *value_ptr)
{
	_endthread();
}

int pthread_join(pthread_t thread, void **value_ptr)
{
	DWORD dwExitCode = 0;
	DWORD retvalue = WaitForSingleObject(thread.handle, INFINITE);
	if (retvalue == WAIT_OBJECT_0)
	{
		GetExitCodeThread(thread.handle, &dwExitCode);
//		CloseHandle(thread.handle);
		return dwExitCode;
	}
	else
	{
		return EINVAL;
	}
}

int ThreadJoin(HANDLE hthread, void **value_ptr)
{
	DWORD dwExitCode = 0;
	DWORD retvalue = WaitForSingleObject(hthread, INFINITE);
	if (retvalue == WAIT_OBJECT_0)
	{
		GetExitCodeThread(hthread, &dwExitCode);
		//CloseHandle(hthread);
		return dwExitCode;
	}
	else
	{
		return EINVAL;
	}
}

pthread_t pthread_self(void)
{
	pthread_t pt;
	pt.handle = GetCurrentThread();
	return pt;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	return !CloseHandle(mutex->handle);
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	HANDLE handle = CreateMutex(NULL, FALSE, NULL);
	if (handle != NULL) {
		mutex->handle = handle;
		return 0;
	}
	else {
		return 1;
	}
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	DWORD retvalue = WaitForSingleObject(mutex->handle, INFINITE);
	if (retvalue == WAIT_OBJECT_0) {
		return 0;
	}
	else {
		return EINVAL;
	}
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	DWORD retvalue = WaitForSingleObject(mutex->handle, 0);
	if (retvalue == WAIT_OBJECT_0) {
		return 0;
	}
	else if (retvalue == WAIT_TIMEOUT) {
		return EBUSY;
	}
	else {
		return EINVAL;
	}
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	return !ReleaseMutex(mutex->handle);
}

time_t timegm(struct tm *tm)
{
	return _mkgmtime(tm);
}
/*
int is_leap(unsigned y) {
	y += 1900;
	return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

time_t timegm(struct tm *tm)
{
	static const unsigned ndays[2][12] = {
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};
	time_t res = 0;
	int i;

	for (i = 70; i < tm->tm_year; ++i)
		res += is_leap(i) ? 366 : 365;

	for (i = 0; i < tm->tm_mon; ++i)
		res += ndays[is_leap(tm->tm_year)][i];
	res += tm->tm_mday - 1;
	res *= 24;
	res += tm->tm_hour;
	res *= 60;
	res += tm->tm_min;
	res *= 60;
	res += tm->tm_sec;
	return res;
}
*/

int GetDiskInfo(const char *anyfile, unsigned long long* ullFree, unsigned long long* ullTotal)
{
	char szPath[MAX_PATH];
	strcpy(szPath, anyfile);
	char* slash = strrchr(szPath, '\\');
	if (!slash)slash = strrchr(szPath, '/');
	if (slash)slash[0] = 0;
	ULARGE_INTEGER liFreeAvailable, liTotal;
	*ullTotal = 0;
	*ullFree = 0;

	if (GetDiskFreeSpaceEx(szPath, &liFreeAvailable, &liTotal, NULL))
	{
		*ullTotal = liTotal.QuadPart;
		*ullFree = liFreeAvailable.QuadPart;
		return 0;
	}
	return GetLastError();
}

unsigned long long getTotalSystemMemory()
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullTotalPhys;
}

#endif //_WIN32

// RSS begin
/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getCurrentRSS( ) for an unknown OS."
#endif

/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t getCurrentRSS()
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
	return (size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
	/* OSX ------------------------------------------------------ */
	struct mach_task_basic_info info;
	mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
		(task_info_t)&info, &infoCount) != KERN_SUCCESS)
		return (size_t)0L;      /* Can't access? */
	return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	/* Linux ---------------------------------------------------- */
	long rss = 0L;
	FILE* fp = NULL;
	if ((fp = fopen("/proc/self/statm", "r")) == NULL)
		return (size_t)0L;      /* Can't open? */
	if (fscanf(fp, "%*s%ld", &rss) != 1)
	{
		fclose(fp);
		return (size_t)0L;      /* Can't read? */
	}
	fclose(fp);
	return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);

#else
	/* AIX, BSD, Solaris, and Unknown OS ------------------------ */
	return (size_t)0L;          /* Unsupported. */
#endif
}

int getRSS(int* curr, int* max, int* growth)
{
	static int prev = 0;
	int now = getCurrentRSS();
	int dt = now - prev;
	prev = now;
	static int max_mem = 0;
	max_mem = (max_mem > now) ? max_mem : now;
	if (max)*max = max_mem;
	if (curr)*curr = now;
	if (growth)*growth = dt;
	return now;
}
// RSS end


std::string GetMachineName()
{
	char *temp = 0;
	std::string computerName="";
#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)
	temp = getenv("COMPUTERNAME");
	if (temp != 0) {
		computerName = temp;
		temp = 0;
	}
#else
	temp = getenv("HOSTNAME");
	if (temp != 0) {
		computerName = temp;
		temp = 0;
	}
	else {
		temp = new char[512];
		if (gethostname(temp, 512) == 0) { // success = 0, failure = -1
			computerName = temp;
		}
		delete[]temp;
		temp = 0;
	}
#endif
	return computerName;
}

void create_guid(char guid[])
{
#ifndef _WIN32
//	uuid_t out;
//	uuid_generate(out);
//	uuid_unparse_lower(out, guid);

	srand(timeGetTime());

	sprintf(guid, "%x%x-%x-%x-%x-%x%x%x", 
		rand(), rand(),                 // Generates a 64-bit Hex number
		rand(),                         // Generates a 32-bit Hex number
		((rand() & 0x0fff) | 0x4000),   // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
		rand() % 0x3fff + 0x8000,       // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
		rand(), rand(), rand());        // Generates a 96-bit Hex number
#else
	GUID out = {0};
	CoCreateGuid(&out);
	sprintf(guid, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", out.Data1, out.Data2, out.Data3, out.Data4[0], out.Data4[1], out.Data4[2], out.Data4[3], out.Data4[4], out.Data4[5], out.Data4[6], out.Data4[7]);
#endif

	for ( ; *guid; ++guid) *guid = tolower(*guid);
}

