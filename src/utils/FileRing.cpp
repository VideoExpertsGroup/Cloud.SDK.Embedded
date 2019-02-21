#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <linux/limits.h>
#define _open open
#define _close close
#define _read read
#define _write write
#define _lseek lseek
#endif

#include "FileRing.h"

#define FLG_ACCESS ( S_IREAD | S_IWRITE )
#define FLG_MODE_RD  ( O_RDONLY )

#ifdef WIN32
	#define FLG_MODE_ALL ( O_RDWR | O_CREAT | O_BINARY )
	#define SLASH_CHAR '\\'
#else 
	#define FLG_MODE_ALL ( O_RDWR | O_CREAT )
	#define SLASH_CHAR '/'

	#ifndef MAX_PATH
		#ifdef PATH_MAX
			#define MAX_PATH PATH_MAX
//FILENAME_MAX
		#else
			#define MAX_PATH (4096)
		#endif
	#endif
#endif //WIN32		

int CFileRing::GetStoredIndex()
{

	if(m_strIdxFile.empty())
		return 0;

	CAutoLock lock(&m_csLock);

	int idx = 0;

	int idxfile = _open(m_strIdxFile.c_str(), FLG_MODE_RD, FLG_ACCESS);

	if(idxfile>0)
	{
		int file_size = _lseek(idxfile, 0, SEEK_END);
		_lseek(idxfile, 0, SEEK_SET);

		if(file_size>0)
		{
			int read_len = 0;
			char* read_buf = new char[file_size + 1];
			memset(read_buf, 0, file_size + 1);
			if((read_len = _read(idxfile, read_buf, file_size)) > 0)
			{
				idx = atoi(read_buf);
			}
			delete[] read_buf;
		}	
		_close(idxfile);
	}
	return idx;
}

int CFileRing::SetStoredIndex(int idx)
{
	if(m_strIdxFile.empty())
		return -1;

	CAutoLock lock(&m_csLock);

	int idxfile = _open(m_strIdxFile.c_str(), FLG_MODE_ALL, FLG_ACCESS);
	if(idxfile<0)
		return -2;

	_lseek(idxfile, 0, SEEK_SET);

	char szIdx[32] = {0};
	sprintf(szIdx, "%d", idx);
	_write(idxfile, szIdx, strlen(szIdx)+1);
	_close(idxfile);
	return 0;
}

CFileRing::CFileRing()
{
        m_bInited = false;	
}

int CFileRing::Init(const char *filename, int maxFileSizeInBytes, int maxFileIndex)
{

	if(maxFileSizeInBytes==m_nMaxFileSize && m_nMaxFileIndex==maxFileIndex)
	{
		return 0;
	}

	m_strBaseFileName = filename;
	m_nMaxFileSize = maxFileSizeInBytes;
	m_nMaxFileIndex = maxFileIndex;
	m_nFile = 0;

	if(m_strBaseFileName.empty())
		return -1;

	char szPath[MAX_PATH]={0};
	char szIdxFile[MAX_PATH]={0};
	strcpy(szPath, m_strBaseFileName.c_str());
	char* z = strrchr(szPath, SLASH_CHAR);
	if(z)
		z[1] = 0;
	else
		strcpy(szPath,"./");

	m_strIdxFile = szPath;
	m_strIdxFile += "logindex.txt";
	m_nCurFileIndex = GetStoredIndex();

	if(m_nCurFileIndex>m_nMaxFileIndex)
		m_nCurFileIndex=0;

	m_strCurrentFilename = GetFileNameByIdx(m_nCurFileIndex);

	m_bInited = true;

	return 0;
}

CFileRing::~CFileRing()
{
	SetStoredIndex(m_nCurFileIndex);
	if(m_nFile)
		_close(m_nFile);
	m_nFile = 0;
}

int CFileRing::Write(const char *str, int len)
{

	CAutoLock lock(&m_csLock);

	if(m_nFile<=0)
	{
		m_nFile = _open(m_strCurrentFilename.c_str(), FLG_MODE_ALL, FLG_ACCESS);
		_lseek(m_nFile, 0, SEEK_END);
	}

	if(m_nFile<=0)
		return -1;

	if(m_strBaseFileName.empty())
		return 0;

//	int len = strlen(str);
	int file_size = GetCurFileSize();

//	printf("file=%s, len=%d, file_size=%d, m_nMaxFileSize=%d\n", m_strCurrentFilename.c_str(), len, file_size, m_nMaxFileSize);

	if (file_size + len >= m_nMaxFileSize)
	{
//		printf(">\n");

		int to_write = m_nMaxFileSize - file_size;
		if (to_write > 0)
		{
			_lseek(m_nFile, 0, SEEK_END);
			int x = _write(m_nFile, str, to_write);

//			printf("file=%s, len=%d, file_size=%d, m_nMaxFileSize=%d, to_write=%d, file_size2=%d, x=%d\n", m_strCurrentFilename.c_str(), len, file_size, m_nMaxFileSize, to_write, GetCurFileSize(), x);

			len -= to_write;
			str += to_write;
		}
		else
		{
//			printf("to_write<0 = %d\n", to_write);
		}

		SwitchToNextFile();
	}

	_lseek(m_nFile, 0, SEEK_END);
//	printf("GetCurFileSize()=%d\n", GetCurFileSize());
	int z = _write(m_nFile, str, len);
//	printf("len=%d, z=%d, GetCurFileSize()=%d\n", len, z, GetCurFileSize());
	return z;
}

char* CFileRing::GetFileNameByIdx(int idx)
{
	static char szName[MAX_PATH]={0};
	if(m_strBaseFileName.empty())
		return NULL;

	char* pFileName = new char[strlen(m_strBaseFileName.c_str())+1];
	strcpy(pFileName, m_strBaseFileName.c_str());
	char* pLastDot = strrchr(pFileName,'.');
	char* pExt = NULL;
	if(pLastDot)
		pExt = pLastDot+1;

	*pLastDot = 0;

	sprintf(szName,"%s_%d.%s", pFileName, idx, pExt);
	delete[] pFileName;

	return szName; 
}

int CFileRing::GetCurFileSize()
{
	if(!m_nFile)
		return 0;

	CAutoLock lock(&m_csLock);

//	long pos = ftell(m_nFile);
	long fend = _lseek(m_nFile, 0, SEEK_END);
//	lseek(m_nFile, pos, SEEK_SET);
	return fend;
}

int CFileRing::SwitchToNextFile()
{

	CAutoLock lock(&m_csLock);

	if(m_nFile)
		_close(m_nFile);

	m_nCurFileIndex++;

	if(m_nCurFileIndex>m_nMaxFileIndex)
		m_nCurFileIndex=0;
	
	SetStoredIndex(m_nCurFileIndex);

	m_strCurrentFilename = GetFileNameByIdx(m_nCurFileIndex);
	remove(m_strCurrentFilename.c_str());

//	printf("switch to %s\n", m_strCurrentFilename.c_str());

	m_nFile = _open(m_strCurrentFilename.c_str(), FLG_MODE_ALL, FLG_ACCESS);

	if(m_nFile<=0)
		return -1;

	return 0;
}

int CFileRing::GetData(unsigned char** ppData, unsigned int* pSize)
{
	int start_idx = m_nCurFileIndex + 1;
	if (start_idx > m_nMaxFileIndex)
		start_idx = 0;

	int buff_size = 0;

	CAutoLock lock(&m_csLock);

	for (int n = 0; n <= m_nMaxFileIndex; n++)
	{
		int fdDataFile = _open(GetFileNameByIdx(start_idx), FLG_MODE_RD, FLG_ACCESS);
		start_idx++; if(start_idx > m_nMaxFileIndex)start_idx = 0;

		if (fdDataFile > 0)
		{
			buff_size += _lseek(fdDataFile, 0, SEEK_END);
			_close(fdDataFile);
		}
	}

	start_idx = m_nCurFileIndex + 1;
	if (start_idx > m_nMaxFileIndex)
		start_idx = 0;

	buff_size += 1;
	unsigned char* data_buf = (unsigned char*)malloc(buff_size);
	memset(data_buf, 0, buff_size);
	unsigned char* read_buf = data_buf;
	unsigned int readed_bytes = 0;

	for (int n = 0; n <= m_nMaxFileIndex; n++)
	{

//		printf("\nfile=%s\n", GetFileNameByIdx(start_idx));

		int fdDataFile = _open(GetFileNameByIdx(start_idx), FLG_MODE_RD, FLG_ACCESS);
		start_idx++; if (start_idx > m_nMaxFileIndex) start_idx = 0;

		if (fdDataFile > 0)
		{
			int file_size = _lseek(fdDataFile, 0, SEEK_END);
			_lseek(fdDataFile, 0, SEEK_SET);

			if (file_size > 0)
			{
				int read_len = 0;
				memset(read_buf, 0, file_size);
				if ((read_len = _read(fdDataFile, read_buf, file_size)) > 0)
				{
//					printf("%s", read_buf);
					read_buf += read_len;
					readed_bytes += read_len;
				}
			}
			_close(fdDataFile);
		}
	}

	*ppData = data_buf;
	*pSize = readed_bytes;

	return 0;
}
