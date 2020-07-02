
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

DWORD timeBeginPeriod(DWORD uPeriod) {};
DWORD timeEndPeriod(DWORD uPeriod) {};

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

void usleep(unsigned int usec)
{
	LARGE_INTEGER ft;
	ft.QuadPart = -(10 * (__int64)usec);
	HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
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

//ini files

/* Basic data header for the INI file tool library */

/* any ini file has its own header structure, also this forms 	*/
/* the head of any list search the software does		*/
typedef struct ini_file_head
{
	struct 	ini_file_head* next;	/* link list of ini files */
	struct	ini_file_sec* data;	/* sections in ini file */
	struct	ini_file_data* user;	/* pointer to user selected data */
	struct  ini_file_head* head;	/* used when the ini file isn't there */
	char* filename;			/* file name and path */
	char* access;			/* see if the source changes */
	int	flags;				/* access flags to this file */
} ini_file_head;

/* each section of an ini file has its own independant sub	*/
/* header, with the local data sporning off it			*/
typedef struct ini_file_sec
{
	struct 	ini_file_sec* next;	/* ini file sections */
	struct	ini_file_data* data;	/* ini file data */
	char* name;				/* section name */
} ini_file_sec;

/* data list, also the unused lines are stored here		*/
typedef struct ini_file_data
{
	struct	ini_file_data* next;	/* link point */
	char* name;				/* data name */
	char* string;			/* data string */
	int	data;				/* interger of data */
	int	flag;				/* is this line a comment, */
	/* or data */
} ini_file_data;


struct ini_file_head* ini_head = NULL;

void ini_section_filter(char* str);
void ctrlm(char* line);
int ini_line_decode(char* line);
int ini_find(const char* Section, const char* Entery, const char* File);
int get_ini_data(char* line, const char* entery, const char* data);
int form_ini_num(const char* data);
int new_element(struct ini_file_sec* fs, const char* Entery);
int new_section(struct ini_file_head* fh, const char* Section, const char* Entery);
int new_file(const char* File, const char* Section, const char* Element);

/* following are the sub routines used by the ini file handling	*/
/* routines, basically searching, file loading, and saving etc	*/
/* Load the selected ini file into memory. return 0 for error 	*/
/* else return 1 for OK						*/

int Load_Ini_File(const char* File)
{
	FILE* fp;
	char line[0x100];		/* 255 bytes max file line length */
	char Section[0x80];		/* local control for the current section */
	char Entery[0x100];		/* local control of entery data */
	char Data[0x100];		/* data for an entery */

	int  line_no;			/* current line in the file */

	struct ini_file_head* h;

	if (ini_head == NULL)  	/* compleatly new list */
	{


		ini_head = (struct ini_file_head*)malloc(sizeof(struct ini_file_head));

		ini_head->next = NULL;
		ini_head->data = NULL;
		h = ini_head;

	}
	else
	{
		h = ini_head;	/* set a basic pointer to list */

		while (h->next != NULL)
			h = h->next;

		h->next = (struct ini_file_head*)malloc(sizeof(struct ini_file_head));

		h = h->next;
		h->next = NULL;
		h->data = NULL;
	}


	h->filename = (char*)calloc(sizeof(char), strlen(File) + 2);


	strcpy(h->filename, File);	/* save the file name in the list */

	/* now we need to load the ini file into memory */

	if ((fp = fopen(h->filename, "r")) == NULL)
	{

		ini_head->head = h;		/* save head for latter */
		return 0;
	}


	/* Seed the default data for the load */
	line_no = 1;
	strcpy(Section, "");


	while (!feof(fp)) 	/* read in loop */
	{
		fgets(line, sizeof(line), fp);	/* read line */

		ctrlm(line);			/* clear the ctrlm off end of line */

		switch (ini_line_decode(line))
		{
		case 0:		/* comment */

			ini_find(Section, line, h->filename);

			ini_head->user->flag = 1;		/* mark comment */

			break;
		case 1:		/* Section header */

			sscanf(line, "[%s", Section);

			ini_section_filter(Section);

			break;
		case 2:		/* Entery info */

			get_ini_data(line, Entery, Data);

			ini_find(Section, Entery, h->filename);

			ini_head->user->string = (char*)calloc(sizeof(char),
				strlen(Data) + 1);

			strcpy(ini_head->user->string, Data);

			ini_head->user->data = form_ini_num(Data);

			break;
		default:		/* Error */
			fprintf(stderr, "Error on line %d in %s\n", line_no, h->filename);
			exit(-1);
			break;
		}

		line_no++;
	}

	fclose(fp);	/* file read, so close */
	return 1;	/* return TRUE for OK */
}

/* find the entery in the loaded ini file, if the entery isn't there create it */

int ini_find(const char* Section, const char* Entery, const char* File)
{
	struct ini_file_head* fh;
	struct ini_file_sec* fs;
	struct ini_file_data* fd;

	if (ini_head == NULL)
	{
		return 0;
	}
	fh = ini_head;
	while (fh != NULL)
	{
		if ((strcmp(fh->filename, File)) == 0)
		{
			fs = fh->data;
			while (fs != NULL)
			{
				if ((strcmp(fs->name, Section)) == 0)
				{
					fd = fs->data;
					while (fd != NULL)
					{
						if ((strcmp(fd->name, Entery)) == 0)
						{
							ini_head->user = fd;
							return 1;
						}
						fd = fd->next;
					} /* element not in list */
					new_element(fs, Entery);
					return 2;
				}
				fs = fs->next;
			} /* Section not in list */
			new_section(fh, Section, Entery);
			return 3;
		}
		fh = fh->next;
	} /* file not in list */
	new_file(File, Section, Entery);
	return 4;
}

/* following is the three functions needed to add new elements,sections,files to the ini list */

/* new_element, add a new element to the current section */
int new_element(struct ini_file_sec* fs, const char* Entery)
{
	struct ini_file_data* fd;



	fd = fs->data;

	if (fd != NULL)
	{
		while (fd->next != NULL)
			fd = fd->next;

		/* fd now points at the last element in the list */

		fd->next = (struct ini_file_data*)malloc(sizeof(struct ini_file_data));
		fd = fd->next;
	}
	else
	{
		fs->data = (struct ini_file_data*)malloc(sizeof(struct ini_file_data));
		fd = fs->data;
	}

	fd->next = NULL;
	fd->name = (char*)calloc(sizeof(char), strlen(Entery) + 1);
	strcpy(fd->name, Entery);
	fd->data = form_ini_num(Entery);
	fd->flag = 0;
	fd->string = NULL;	/* seed the basic data elemnt */
	ini_head->user = fd;	/* set the return data pointer */

	return 0;
}

/* new_section, add a new section and entery to the current file */
int new_section(struct ini_file_head* fh, const char* Section, const char* Entery)
{
	struct ini_file_sec* fs;

	fs = fh->data;

	if (fs != NULL)
	{
		while (fs->next != NULL)
			fs = fs->next;

		fs->next = (struct ini_file_sec*)malloc(sizeof(struct ini_file_sec));

		fs = fs->next;
	}
	else
	{
		fh->data = (struct ini_file_sec*)malloc(sizeof(struct ini_file_sec));
		fs = fh->data;
	}
	fs->next = NULL;
	fs->data = NULL;
	fs->name = (char*)calloc(sizeof(char), strlen(Section) + 1);

	strcpy(fs->name, Section);

	/* now use the new entery code to get it in */
	new_element(fs, Entery);
	return 0;
}

/* new_file, add a new file,section,entery to the header list */
int new_file(const char* File, const char* Section, const char* Element)
{
	struct ini_file_head* fh;

	fh = ini_head;

	if (fh != NULL)
	{
		while (fh->next != NULL)
			fh = fh->next;

		fh->next = (struct ini_file_head*)malloc(sizeof(struct ini_file_head));
		fh = fh->next;
	}
	else
	{
		ini_head = (struct ini_file_head*)malloc(sizeof(struct ini_file_head));
		fh = ini_head;
	}

	fh->next = NULL;
	fh->data = NULL;
	fh->user = NULL;	/* the return pointer is only used on the first element */

	fh->filename = (char*)calloc(sizeof(char), strlen(File) + 1);
	strcpy(fh->filename, File);
	new_section(fh, Section, Element);
	return 0;
}

int ini_line_decode(char* line)
{
	switch (line[0]) 	/* first char holds the key! */
	{
	case '#':
	case ';':
	case '\n':
		return 0;
	case '[':
		return 1;
	}
	return 2;
}

int get_ini_data(char* line, const char* entery, const char* data)
{
	char* copy;	/* copy of the incomming line */
	char* p, * p1;	/* pointer to line, and pointer to start of data */
	char c;		/* local char to test with */

	int  mode;	/* current scan mode */

	mode = 0;

	copy = (char*)calloc(sizeof(char), strlen(line) + 1);

	strcpy(copy, line);

	p = copy;
	p1 = NULL;
	/* init copmpleate */

	while (1)
	{

		c = *p;			/* get the char to test */
		if (c == '\0')
			mode = 2;	/* get out of here! */


		switch (mode) 		/* what to do with it ? */
		{
		case 0:	/* looking for end of entery */

			if (c == ' ' | c == '=')
			{
				*p = '\0';
				p++;	/* get p past the null */
				mode = 1;

			}

			break;
		case 1:	/* looking for start of data */

			if (c == ' ' | c == '=')
			{
				mode = 1;
				*p = (char)'\0';
				p++;

			}
			else
			{

				mode = 2;
				p1 = p;

				strcpy((char*)entery, copy);
				strcpy((char*)data, p1);
				free(copy);


				return 0;

			}
			break;
		case 2:	/* looking for end of line */


			strcpy((char*)entery, copy);


			if (p1 != NULL)
				sprintf((char*)data, "%s", p1);
			else
				sprintf((char*)data, "");

			free(copy);


			return 0;
		}
		if (mode != 1)
			p++;
	}
	return 0;
}

/* save_ini_file: saves the given file name back to disk, this is done */
/* every time data is written to the ini file, so there is no need to   */
/* save the data on exiting the users program				*/
int save_ini_file(const char* file)
{
	struct ini_file_head* h;
	struct ini_file_sec* s;
	struct ini_file_data* d;

	FILE* fp;

	h = ini_head;

	while (h != NULL)
	{

		if ((strcmp(h->filename, file)) == 0)
		{
			/* we have the file name */
			if ((fp = fopen(file, "w")) == NULL)
			{
				fprintf(stderr, "Can't write to file %s\n", file);
				return 0;
			}
			s = h->data;	/* start to recurse the ini data */
			while (s != NULL)
			{
				d = s->data;
				if (strlen(s->name) > 1)
					fprintf(fp, "[%s]\n", s->name);
				while (d != NULL)
				{
					if (strlen(d->name) > 1)
					{
						if (d->flag)
							fprintf(fp, "%s\n", d->name);
						else
							fprintf(fp, "%s = %s\n", d->name, d->string);
					}
					else
						fprintf(fp, "\n");	/* blank line */
					d = d->next;
				}
				s = s->next;
			}
			fclose(fp);
			return 0;
		}
		else
			h = h->next;
	}
	fprintf(stderr, "Can't save the ini file %s\n", file);
	return 0;
}


int form_ini_num(const char* data)
{
	int ret;

	if ((strcmp(data, "True")) == 0)
		return(1);
	else if ((strcmp(data, "true")) == 0)
		return(1);
	else if ((strcmp(data, "TRUE")) == 0)
		return(1);
	else if ((strcmp(data, "False")) == 0)
		return(0);
	else if ((strcmp(data, "false")) == 0)
		return(0);
	else if ((strcmp(data, "FALSE")) == 0)
		return(0);
	else if ((strcmp(data, "Yes")) == 0)
		return(1);
	else if ((strcmp(data, "yes")) == 0)
		return(1);
	else if ((strcmp(data, "YES")) == 0)
		return(1);
	else if ((strcmp(data, "No")) == 0)
		return(0);
	else if ((strcmp(data, "no")) == 0)
		return(0);
	else if ((strcmp(data, "NO")) == 0)
		return(0);

	/* convert the string into a number */
	return (int)strtol(data, NULL, 0);

}

/* if file loaded, return 1, else return 0 */
int ini_file_loaded(const char* File)
{
	struct ini_file_head* h;
	h = ini_head;

	while (h != NULL)
	{
		if ((strcmp(File, h->filename)) == 0)
			return 1;
		else
			h = h->next;
	}
	return 0;
}

void ctrlm(char* line)
{
	int a;
	int c;

	a = 0;
	while (1)
	{
		c = line[a];
		if (c == '\0')
			return;
		if (c == 13)
			c = '\0';
		if (c == 10)
			c = '\0';
		line[a] = c;
		a++;
	}
}

/* replace the trailing `]` in the section name */
void ini_section_filter(char* str)
{
	char c;
	int  p = 0;

	while (1)
	{
		c = str[p];
		if (c == '\0')
			return;
		if (c == ']')
			c = '\0';
		str[p] = c;
		p++;
	}
}


extern int MYGetPrivateProfileString(const char* Section, const char* Entery, const char* Default, char* Buffer, int szBuffer, const char* File)
{
	if (ini_head == NULL)
	{

		if (!Load_Ini_File(File))
		{

			new_section(ini_head->head, Section, Entery);

			ini_head->user->string = (char*)calloc(sizeof(char), strlen(Default) + 1);

			strcpy(ini_head->user->string, Default);

			ini_head->user->data = form_ini_num(Default);

			strcpy(Buffer, Default);

			return(strlen(Buffer));
		}
	}
	else
	{
		if (!ini_file_loaded(File))
		{

			if (!Load_Ini_File(File))
			{

				new_section(ini_head->head, Section, Entery);

				ini_head->user->string = (char*)calloc(sizeof(char), strlen(Default) + 1);

				strcpy(ini_head->user->string, Default);

				ini_head->user->data = form_ini_num(Default);

				strcpy(Buffer, Default);

				return(strlen(Buffer));

			}
		}
	}


	if ((ini_find(Section, Entery, File)) != 1) 		/* created an element */
	{
		ini_head->user->string = (char*)calloc(sizeof(char), strlen(Default) + 1);

		strcpy(ini_head->user->string, Default);

		ini_head->user->data = form_ini_num(Default);

		strcpy(Buffer, Default);

		return(strlen(Buffer));
	}
	else  	/* found element */
	{
		strcpy(Buffer, ini_head->user->string);

		return(strlen(Buffer));
	}

	return 0;
}


extern int MYWritePrivateProfileString(const char* Section, const char* Entery, const char* String, const char* File)
{

	if (!ini_find(Section, Entery, File))  /* list pointer is null */
	{
		new_file(File, Section, Entery);	/* so create it! */
		ini_head->user->string = (char*)calloc(sizeof(char), strlen(String) + 1);

		strcpy(ini_head->user->string, String);

		ini_head->user->data = form_ini_num(String);

		save_ini_file(File);
		return(strlen(String));
	}

	if (ini_head->user->string != NULL)
	{
		free(ini_head->user->string);
	}

	ini_head->user->string = (char*)calloc(sizeof(char), strlen(String) + 1);

	strcpy(ini_head->user->string, String);

	ini_head->user->data = form_ini_num(String);

	save_ini_file(File);
	return(strlen(String));

}

//ini files

