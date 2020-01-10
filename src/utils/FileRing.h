#ifndef INC_FILERING_H
#define INC_FILERING_H

#include <stdio.h>
#include <string>
#include "windefsws.h"

class CFileRing
{
public:
	CFileRing();
	~CFileRing();
	int Init(const char *filename, int maxFileSizeInBytes=65535, int maxFileIndex = 10);
	int Write(const char *str, int len);
	int GetData(unsigned char** ppData, unsigned int* pSize);
	std::string GetCurrentFileName() { return m_strCurrentFilename; }
	bool m_bInited;

private:
	int GetStoredIndex();
	int SetStoredIndex(int idx);
	int SwitchToNextFile();
	char* GetFileNameByIdx(int idx);
	int GetCurFileSize();

	int m_nFile;
	int m_nMaxFileSize;
	int m_nMaxFileIndex;
	std::string m_strBaseFileName;
	int m_nCurFileIndex;
	std::string m_strIdxFile;
	std::string m_strCurrentFilename;
	CCritSec m_csLock;

};

#endif //INC_FILERING_H
