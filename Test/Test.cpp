// Test.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <regex> 
#include "../Common/Funcs.h"

using namespace std;

typedef char* (*PFCallbackSearchDeletedFile)(const int magic, const char* diskPartition, const char* checkExt);

typedef void (*PFCallbackDestroy)(char *buff);

typedef bool (*PFCallbackDeletedFileRecorvy)(const int magic, const char* fileName, const char* fileAddr,  const char* diskPartition, const char* recoveryPath);

#define VL_MAGIC_NUMBER (0x23E72DAC)


int _tmain(int argc, _TCHAR* argv[])
{
	HINSTANCE m_Deletechek = ::LoadLibrary("SearchDeleteFile.dll");
	if (NULL == m_Deletechek)
	{
		printf("����:%d\n",GetLastError());
	}
	PFCallbackSearchDeletedFile Delefielchek=(PFCallbackSearchDeletedFile)::GetProcAddress(m_Deletechek,"SearchDeletedFileInfo");
	if (NULL == Delefielchek)
	{
		printf("����:%d\n",GetLastError());
	}
	PFCallbackDestroy delefree = (PFCallbackDestroy)::GetProcAddress(m_Deletechek,"DestroyBuffer");
	if (NULL == delefree)
	{
		printf("����:%d\n",GetLastError());
	}
	PFCallbackDeletedFileRecorvy recorvy= (PFCallbackDeletedFileRecorvy)::GetProcAddress(m_Deletechek,"DeletedFileRecovery");
	char* cdisk= "e";
	int Imagic = VL_MAGIC_NUMBER;
	char checkfileName[20]={'.','d','o','c','x',';','.','d','o','c','\0',};
	char *repath="d:\\VanliVM123\\";
	vector<DWORD> datasize;
	char *deledata = Delefielchek(Imagic, cdisk, checkfileName);
	if (NULL == deledata)
	{  
		printf("����");
	}

	string strData = string(deledata);
	CFuncs::WriteLogInfo(SLT_INFORMATION, "�ļ�BUFF��%s", strData.c_str());
	delefree(deledata);
	size_t beginPos = strData.find("FileName:");
	while(string::npos != beginPos)
	{
		size_t endPos = strData.find(",FileCreateTime:", beginPos);
		if(endPos == string::npos)
		{
			return -1;
		}
		string strFilename = strData.substr(beginPos + 9, endPos - beginPos - 9);

		CFuncs::WriteLogInfo(SLT_INFORMATION, "�ָ��ļ�%s�ҵ���", strFilename.c_str());

	/*	beginPos = strData.find("AddrData:", endPos);
		if(string::npos == beginPos)
		{*/
			beginPos = strData.find("FileAddr:", endPos);
			if(string::npos == beginPos)
			{
				return -1;
			}
	/*	}*/

		endPos = strData.find(",FilePath:", beginPos);
		if(endPos == string::npos)
		{
			return -1;
		}
		string strAddrData = strData.substr(beginPos, endPos - beginPos);

		bool ret=recorvy(Imagic, strFilename.c_str(), strAddrData.c_str(),  cdisk,  repath);
		if (ret)  
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "�ָ��ļ�%sʧ��", strFilename.c_str());
		}
		beginPos = strData.find("FileName:", endPos);
	}
	return -1;
}



