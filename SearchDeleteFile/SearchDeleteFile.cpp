// SearchDeleteFile.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "SearchDeleteFile.h"



SEARCHDELETEFILE_API char* SearchDeletedFileInfo(const int magic, const char* diskPartition, const char* fileType)
{
	if(NULL == diskPartition || NULL == fileType)
	{
		return NULL;
	}
	if(VL_MAGIC_NUMBER != magic)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "SearchDeletedFileInfo ����magic��ֵ����");
		return NULL;
	}

	CFuncs::Init();
	bool bRet=false;

	string strList;
	CDeletedFile *pDeleFileObj = new CDeletedFile();
	bRet = pDeleFileObj->SearchDeletedFile(diskPartition, fileType, strList);
	if (!bRet)
	{ 
		delete pDeleFileObj;
		pDeleFileObj = NULL;
		return NULL;
	}
	delete pDeleFileObj;
	pDeleFileObj = NULL; 
	char* outBuf = (char *)malloc(strList.length() + 1);
	if(NULL == outBuf)
	{
		return NULL;
	}
	else
	{
		memset(outBuf,0,sizeof(outBuf));
		strList._Copy_s(outBuf, strList.length(), strList.length(), 0);
	
	}

	return outBuf;
}

SEARCHDELETEFILE_API void DestroyBuffer(char* buff)
{
	if(NULL != buff)
	{
		free(buff);
		buff = NULL;
	}
}

SEARCHDELETEFILE_API int DeletedFileRecovery(const int magic, const char* fileName, const char* fileAddr, const char* diskPartition, const char* recoveryPath)
{
	CFuncs::Init();
	bool bRet = false;
	if(VL_MAGIC_NUMBER != magic)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "DeletedFileRecovery ����magic��ֵ����");
		return RVT_MAGIC;
	}
	string strRecoveryPath = string(recoveryPath);
	if('\\' != strRecoveryPath[strRecoveryPath.length() - 1])
	{
		strRecoveryPath.append("\\");
	}
	if(!CFuncs::DirectoryExists(strRecoveryPath.c_str()))
	{
		CreateDirectory(strRecoveryPath.c_str(), NULL);
	}

	string tempInfo = string(fileName);
	char szFileName[1024] = {0};
	string strFileName =  CUrlConver::UrlUTF8Decoder(tempInfo, szFileName, 1024);

	CFileRecovery *pFileRecovery = new CFileRecovery();
	bRet = pFileRecovery->FileRecovery(strFileName.c_str(), fileAddr, strRecoveryPath.c_str(), diskPartition);
	if(!bRet)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "DeletedFileRecovery ���� FileRecovery�ָ��ļ�ʧ��");
		delete pFileRecovery;
		pFileRecovery = NULL;
		return RVT_FALSE;
	}
	delete pFileRecovery;
	pFileRecovery = NULL;
	return RVT_TRUE;
}
