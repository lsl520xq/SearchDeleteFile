#include "StdAfx.h"
#include "DeletedFile.h"


CDeletedFile::CDeletedFile(void)
{
	m_ParentDevice = NULL;
}


CDeletedFile::~CDeletedFile(void)
{
	if (m_ParentDevice != NULL && m_ParentDevice != INVALID_HANDLE_VALUE)
	{
		(void)CloseHandle(m_ParentDevice);	
		m_ParentDevice = NULL;
	}
}
bool  CDeletedFile::FileTimeConver(UCHAR* szFileTime, string& strTime)
{
	if(NULL == szFileTime)
	{
		return false;
	}
	LPFILETIME pfileTime = (LPFILETIME)szFileTime;
	SYSTEMTIME systemTime = {0};
	BOOL bTime = FileTimeToSystemTime(pfileTime, &systemTime);
	if (bTime)
	{
		char szTime[32] = { 0 };
		sprintf_s(szTime, _countof(szTime), "%04d-%02d-%02d %02d:%02d:%02d", systemTime.wYear, systemTime.wMonth,
			systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
		strTime.assign(szTime);
		return true;
	}
	return false;
}


bool CDeletedFile::ReadSQData(HANDLE hDevice, UCHAR* Buffer, DWORD SIZE, DWORD64 addr, DWORD *BackBytesCount)
{
	LARGE_INTEGER LiAddr = {NULL};	
	LiAddr.QuadPart = addr;
	DWORD dwError = NULL;

	BOOL bRet = SetFilePointerEx(hDevice, LiAddr, NULL,FILE_BEGIN);
	if(!bRet)
	{
		dwError = GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("ReadSQData::SetFilePointerExʧ��!, ���󷵻���: dwError = %d"), dwError);
		return false;	
	}
	bRet = ReadFile(hDevice, Buffer, SIZE, BackBytesCount, NULL);
	if(!bRet)
	{
		dwError = GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("ReadSQData::ReadFileʧ��!, ���󷵻���: dwError = %d"), dwError);					
		return false;	
	}
	return true;
}
bool CDeletedFile::UnicodeToZifu(UCHAR* Source_Unico, string& fileList, DWORD Size)
{
	wchar_t *toasiclls = new wchar_t[Size/2+1];
	if (NULL == toasiclls)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "UnicodeToZifu::new::����toasiclls�ڴ�ʧ��!");
		return false;
	}
	memset(toasiclls,0,Size+2);
	char *str = (char*)malloc(Size+2);
	if (NULL == str)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "UnicodeToZifu::malloc::����str�ڴ�ʧ��!");
		delete toasiclls;
		toasiclls = NULL;
		return false;
	}
	memset(str,0,Size+2);
	for (DWORD nl = 0; nl < Size; nl += 2)
	{
		toasiclls[nl/2] = Source_Unico[nl+1]<<8 | Source_Unico[nl];
	}
	
	int nRet=WideCharToMultiByte(CP_OEMCP, 0, toasiclls, -1, str, Size+2, NULL, NULL); 
	if(nRet<=0)  
	{  
		CFuncs::WriteLogInfo(SLT_ERROR, "Unicode_To_Zifu::WideCharToMultiByte::ת��ʧ��ʧ��!");
		free(str);
		str = NULL;
		delete toasiclls;
		toasiclls = NULL;
		return false;
	}  
	else  
	{  
		bool StrDef = true;
		for (DWORD i = 0;i < Size; i++)
		{
			if (str[i] == 0)
			{
				fileList.append(str, i);
				StrDef = false;
				break;
			}
		}
		if (StrDef)
		{
			fileList.append(str, Size);
		}
	
    }   
		
	free(str);
	str = NULL;
	delete toasiclls;
	toasiclls = NULL;
	return true;
}
bool  CDeletedFile::GetMbrStartAddr(HANDLE hDevice, vector<DWORD64>& start_sq, UCHAR *ReadSectorBuffer, DWORD64 *LiAddr,DWORD *PtitionIdetifi)
{
	LMBR_Heads MBR = NULL;
	DWORD BackBytesCount = NULL;

	if(!ReadSQData(hDevice, ReadSectorBuffer, SECTOR_SIZE, (*LiAddr), &BackBytesCount))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetMbrStartAddr::ReadSQData��ȡʧ��!,��ַ��%llu",(*LiAddr));
		return false;
	}
	for (int i = 0; i < 64; i += 16)
	{
		MBR = (LMBR_Heads)&ReadSectorBuffer[446+i];				
		if (MBR->_MBR_Partition_Type == 0x05 || MBR->_MBR_Partition_Type == 0x0f)
		{
			if (ReadSectorBuffer[0] == 0 && ReadSectorBuffer[1] == 0 && ReadSectorBuffer[2] == 0 && ReadSectorBuffer[3] == 0)
			{				
				(*LiAddr) = ((*LiAddr) + ((DWORD64)MBR->_MBR_Sec_pre_pa)*512);				
				GetMbrStartAddr(hDevice, start_sq, &ReadSectorBuffer[0], LiAddr,PtitionIdetifi);
			} 
			else
			{							
				(*LiAddr) = ((DWORD64)(MBR->_MBR_Sec_pre_pa)*512);							
				GetMbrStartAddr(hDevice, start_sq, &ReadSectorBuffer[0], LiAddr,PtitionIdetifi);
			}
		} 
		else if (MBR->_MBR_Partition_Type == 0x00)
		{
			return true;
		}
		else if (MBR->_MBR_Partition_Type == 0x07)
		{
			if (ReadSectorBuffer[0] == 0 && ReadSectorBuffer[1] == 0 && ReadSectorBuffer[2] == 0 && ReadSectorBuffer[3] == 0)
			{				
				(*PtitionIdetifi)++;
				start_sq.push_back(*PtitionIdetifi);
				start_sq.push_back((MBR->_MBR_Sec_pre_pa + (*LiAddr)/512));  			
			}
			else
			{
				(*PtitionIdetifi)++;
				start_sq.push_back(*PtitionIdetifi);
				start_sq.push_back(MBR->_MBR_Sec_pre_pa); 		
			}
		}
	}
	return true;
}

bool CDeletedFile::GetPititionName(vector<char>&chkdsk, vector<DWORD64>&dwDiskNumber)
{
	char szVol[7] = { '\\', '\\', '.', '\\',0,':',0};
	for (char i = 'a'; i < 'z'; i++)
	{
		szVol[4]=i;
		HANDLE hDrv = CreateFile(
			szVol, 
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL, 
			OPEN_EXISTING, 
			0, 
			NULL);
		if (hDrv != INVALID_HANDLE_VALUE)
		{
			DWORD dwBytes = 0;
			STORAGE_DEVICE_NUMBER pinfo;
			BOOL bRet = DeviceIoControl(
				hDrv, 
				IOCTL_STORAGE_GET_DEVICE_NUMBER, 
				NULL, 
				0,
				&pinfo, 
				sizeof(pinfo), 
				&dwBytes, 
				NULL);
			if (bRet)
			{
				chkdsk.push_back(i);
				dwDiskNumber.push_back(pinfo.PartitionNumber);

				CloseHandle(hDrv);  
				hDrv=NULL;
			}
			else
			{
				//CloseHandle(hDrv);
				//hDrv=NULL;
				CFuncs::WriteLogInfo(SLT_ERROR, "GetPititionName::DeviceIoControlʧ��!��error code %d %c",GetLastError(),i);
				//return false;
			}
		}
		
		CloseHandle(hDrv);
		hDrv=NULL;
	}
	return true;
}
bool  CDeletedFile::GetHostNtfsStartAddr(HANDLE hDevice, vector<DWORD64>& start_sq, UCHAR *ReadSectorBuffer)
{
	BOOL  BRet = FALSE;
	bool bRet = false;
	DWORD dwFileSize = NULL;
	DWORD BackBytesCount = NULL;
	DWORD SectorNum = NULL;
	DWORD64 LiAddr = NULL;
	LGPT_Heads GptHead = NULL;
	bool bReads = true;
	LGPT_FB_TABLE GptTable = NULL;

	DWORD partitiontype = NULL;
	DWORD LayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX)*20;
	PDRIVE_LAYOUT_INFORMATION_EX  LpDlie = (PDRIVE_LAYOUT_INFORMATION_EX)calloc(1,LayoutSize);
	if(NULL != LpDlie)
	{
		BRet = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, LpDlie, LayoutSize, &BackBytesCount, NULL);
		if (BRet)
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, "��������%lu", LpDlie->PartitionCount);
			partitiontype = LpDlie->PartitionStyle;
			switch(partitiontype)
			{
			case 0:
				CFuncs::WriteLogInfo(SLT_INFORMATION, "����������MBR");

				break;
			case 1:
				CFuncs::WriteLogInfo(SLT_INFORMATION, "����������GPT");

				break;
			case 2:
				CFuncs::WriteLogInfo(SLT_INFORMATION, "Partition not formatted in either of the recognized formats��MBR or GPT");

				break;
			}
		}
		else 
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr DeviceIoControl ��ȡ��������ʧ�ܣ� errorId = %d", GetLastError());	
			free(LpDlie);
			LpDlie = NULL;
			return false;
		}
	}
	else
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr LpDlie calloc�����ȡ������Ϣ�ڴ�ʧ�ܣ� errorId = %d", GetLastError());
		return false;
	}

	free(LpDlie);
	LpDlie = NULL;

	dwFileSize = GetFileSize(hDevice, NULL);
	CFuncs::WriteLogInfo(SLT_INFORMATION, "GetHostNtfsStartAddr GetFileSize �����ܴ�С��%lu", dwFileSize);

	bRet = ReadSQData(hDevice, ReadSectorBuffer, SECTOR_SIZE, 0, &BackBytesCount);	
	if(!bRet)
	{			
		CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr ReadSQData::��ȡ��0��������ȡMBR��GPT��Ϣʧ��!");
		return false;	
	}

	while(bRet && (SectorNum < 40960))
	{
		if(ReadSectorBuffer[510] == 0x55 && ReadSectorBuffer[511] == 0xAA)
		{
			if(partitiontype == 0)
			{
				LiAddr = SectorNum*512;
				CFuncs::WriteLogInfo(SLT_INFORMATION, "��ȡ��MBR���̵�MBR����");


			}else if (partitiontype == 1)
			{

				CFuncs::WriteLogInfo(SLT_INFORMATION, "��ȡ��GPT���̵ı���MBR����");
				SectorNum++;
				BRet = ReadFile(hDevice, ReadSectorBuffer, SECTOR_SIZE, &BackBytesCount, NULL);
				if(!BRet)
				{		
					CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr : ReadFile::��ȡMBR����������һ����ʧ��!");
					return false;	
				}
			}
			break;
		}
		BRet = ReadFile(hDevice, ReadSectorBuffer, SECTOR_SIZE, &BackBytesCount, NULL);
		if(!BRet)
		{		
			CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr : ReadFile::��ȡMBR����������һ����ʧ��!");
			return false;	
		}
		
		SectorNum++;
	}
	if(partitiontype == 0)
	{
		DWORD PititionIdetifi = NULL;
		if (!GetMbrStartAddr(hDevice, start_sq, &ReadSectorBuffer[0], &LiAddr, &PititionIdetifi))
		{

			CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr GetMbrStartAddr Ѱ��MBR��ʼ����ʧ��!!");
		}
		else
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, "�ɹ��ҵ�MBR����,һ��%d������", start_sq.size()/2);
		}

	}
	else if (partitiontype == 1)
	{
		DWORD GptIdentif = NULL;
		GptHead = (LGPT_Heads)&ReadSectorBuffer[0];
		if (GptHead->_Singed_name == 0x5452415020494645)
		{
			CFuncs::WriteLogInfo(SLT_INFORMATION, "����GPTͷ��");

		}
		else
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr �����������GPTͷ��!!");
			return false;
		}
		GptHead = NULL;

		while(bReads)
		{
			BRet = ReadFile(hDevice, ReadSectorBuffer, SECTOR_SIZE, &BackBytesCount, NULL);
			if(!BRet)
			{		
				CFuncs::WriteLogInfo(SLT_ERROR, "GetHostNtfsStartAddr :ReadFile::��ȡGPT��Ϣʧ��!");
				return false;	
			}
			
			SectorNum++;
			GptTable = (LGPT_FB_TABLE)&ReadSectorBuffer[0];
			for (int i = 0; (GptTable->_GUID_TYPE[0] != 0) && (i < 4); i++)
			{
				GptIdentif++;
				if (GptTable->_GUID_TYPE[0] == 0x4433b9e5ebd0a0a2)
				{
					start_sq.push_back(GptIdentif);
					start_sq.push_back(GptTable->_FB_Start_SQ);
				}
				if (i < 3)
				{
					GptTable++;
				}
			}
			if (GptTable->_FB_Start_SQ == 0)
			{

				CFuncs::WriteLogInfo(SLT_INFORMATION, "GPT�б����");
				bReads =  false;
			}
		}
		GptTable = NULL;
		CFuncs::WriteLogInfo(SLT_INFORMATION, "һ����ȡ��GPT����%d��",start_sq.size()/2);

	}
	
	return true;
}
bool CDeletedFile::GetMFTAddr(HANDLE hDevice, DWORD64 start_sq, vector<LONG64>& HVStarMFTAddr,
	vector<DWORD64>& HVStarMFTLen, UCHAR *HostPatitionCuNum, UCHAR *PatitionBuffer)
{
	bool bRet = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase[30] = {NULL};
	UCHAR *H80_data = NULL;
	LONG64 H80_datarun[100] = {NULL};
	DWORD H80_datarun_len[100] = {NULL};
	/************************************************************************/
	/*�ҵ���ʼ��MFT�ļ���¼��ַ                                                                     */
	/************************************************************************/

	memset(PatitionBuffer, 0, FILE_SECTOR_SIZE);
	DWORD64 StarMFTAddr = NULL;
	bRet = ReadSQData(hDevice, PatitionBuffer, SECTOR_SIZE, start_sq*SECTOR_SIZE, &BackBytesCount);		
	if(!bRet)
	{	
		CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::ReadSQData:��ȡ����NTFS��һ����ʧ��!,��ȡMFT��ʼ��ַʧ��!");
		return false;	
	}
	LNTFS_TABLES first_ntfs_dbr = (LNTFS_TABLES)&PatitionBuffer[0];
	*HostPatitionCuNum = first_ntfs_dbr->_Single_Cu_Num;
	StarMFTAddr = first_ntfs_dbr->_MFT_Start_CU;
	if (NULL == StarMFTAddr)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::StarMFTAddr:��õ���ʼMFT��ַΪ��!");
		return false;
	}
	/************************************************************************/
	/*     ������ʼ��MFT�ļ���¼��ַ���ҵ���һ��MFT�ļ���¼����ȡH80�����е�h80��ַ�ͳ���       */
	/************************************************************************/

	memset(PatitionBuffer, 0, FILE_SECTOR_SIZE);
	bRet = ReadSQData(hDevice, PatitionBuffer, FILE_SECTOR_SIZE, (start_sq*SECTOR_SIZE + StarMFTAddr * (*HostPatitionCuNum) * SECTOR_SIZE), &BackBytesCount);		
	if(!bRet)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::ReadSQData:��ȡ����NTFS��һ����ʧ��!,��ȡMFT��ʼ��ַʧ��!");
		return false;	
	}

	File_head_recod = (LFILE_Head_Recoding)&PatitionBuffer[0];
	if(File_head_recod->_FILE_Index == 0x454c4946)
	{
		RtlCopyMemory(&PatitionBuffer[510], (&File_head_recod->_Update_Sequence_Number[0]+2), 2);//������������	
		RtlCopyMemory(&PatitionBuffer[1022], (&File_head_recod->_Update_Sequence_Number[0]+4), 2);
		ATTriBase[0] = (LATTRIBUTE_HEADS)&PatitionBuffer[File_head_recod->_First_Attribute_Dev[0]];
		for (DWORD i_attr=0; i_attr < 25; i_attr++)
		{
			if(ATTriBase[i_attr]->_Attr_Type != 0xffffffff)
			{
				if (ATTriBase[i_attr]->_Attr_Type == 0x80)
				{
					if (ATTriBase[i_attr]->_PP_Attr == 0x01)
					{
						H80_data = (UCHAR*)&ATTriBase[i_attr][0];
						DWORD OFFSET = ATTriBase[i_attr]->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0];
						if (H80_data[OFFSET] != 0 && H80_data[OFFSET] < 0x50)
						{

							for (int i_ha0 = 0; i_ha0 < 100; i_ha0++)
							{
								if (H80_data[OFFSET] > 0 && H80_data[OFFSET] < 0x50)
								{
									UCHAR adres_fig=H80_data[OFFSET]>>4;
									UCHAR len_fig=H80_data[OFFSET]&0xf;
									for(int w = len_fig; w > 0; w--)
									{
										H80_datarun_len[i_ha0] = H80_datarun_len[i_ha0] | (H80_data[OFFSET+w] << (8*(w-1)));
									}
									if (H80_datarun_len[i_ha0] > 0)
									{
										HVStarMFTLen.push_back(H80_datarun_len[i_ha0]);
									}
									else
									{
										CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr:H80_datarun_len:��H80��ַ�������ݶ�Ϊ0!");
										return false;
									}

									for (int w = adres_fig; w > 0; w--)
									{
										H80_datarun[i_ha0] = H80_datarun[i_ha0] | (H80_data[OFFSET+w+len_fig] << (8*(w-1)));
									}
									if (H80_data[OFFSET+adres_fig+len_fig] > 127)
									{
										if (adres_fig == 3)
										{
											H80_datarun[i_ha0] = ~(H80_datarun[i_ha0]^0xffffff);
										}
										if (adres_fig == 2)
										{											
											H80_datarun[i_ha0] = ~(H80_datarun[i_ha0]^0xffff);
										}
									} 
									if (0 == i_ha0)
									{
										HVStarMFTAddr.push_back(H80_datarun[i_ha0]);
									}
									if (i_ha0>0)
									{
										H80_datarun[i_ha0] = H80_datarun[HVStarMFTAddr.size()-1] + H80_datarun[i_ha0];
										HVStarMFTAddr.push_back(H80_datarun[i_ha0]);
									}
									OFFSET = OFFSET + adres_fig + len_fig + 1;
								}
								else
								{
									break;
								}
							}
						}
					}
					else
					{
						CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::��H80û���κε�ַ��ֻ������!");
						return false;
					}
				}
				if (ATTriBase[i_attr]->_Attr_Length < 1024 && ATTriBase[i_attr]->_Attr_Length > 0)
				{
					ATTriBase[i_attr+1] = (LATTRIBUTE_HEADS)((UCHAR*)ATTriBase[i_attr] + ATTriBase[i_attr]->_Attr_Length);
				} 
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetMFTAddr::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase[i_attr]->_Attr_Length);
					return false;
				}
			}
			else if (ATTriBase[i_attr]->_Attr_Type == 0xffffffff)
			{
				memset(PatitionBuffer, 0, FILE_SECTOR_SIZE);
				break;
			}
			else if(ATTriBase[i_attr]->_Attr_Type > 0xff && ATTriBase[i_attr]->_Attr_Type < 0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetMFTAddr:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
				return false;
			}
		}
	}
	else
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "GetMFTAddr::File_head_recod->_FILE_Index:��MFT�ļ���¼ͷ����־����ʧ��!");
		return false;
	}
	return true;
}
bool  CDeletedFile::GetH80DataRunAddr(HANDLE Drive,UCHAR *CacenBuff,DWORD64 Start_NTFS_Addr,LONG64 Start_MftCU,UCHAR CU_NUM,DWORD Refernum
	,DWORD *H20FileRefer,UCHAR *H20Buff,DWORD fNameNum,const char* fName, vector<DWORD> fNamelen, string& fileList,DWORD *ParentMft)
{
	bool Ret = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase[30] = {NULL};
	LAttr_30H H30 = NULL;
	LAttr_20H H20 = NULL;
	UCHAR *H30_NAMES = NULL;
	bool Found = false;
	UCHAR *H80_data = NULL;
	(*H20FileRefer) = NULL;
	(*ParentMft) = NULL;

	memset(CacenBuff,0,FILE_SECTOR_SIZE);
	Ret=ReadSQData(Drive,CacenBuff, FILE_SECTOR_SIZE, (Start_NTFS_Addr * SECTOR_SIZE + Start_MftCU * CU_NUM * SECTOR_SIZE+FILE_SECTOR_SIZE * Refernum),
		&BackBytesCount);		
	if(!Ret)
	{			
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr::ReadSQData��ȡ����Start_CUʧ��!"));
		return false;	
	}

	File_head_recod = (LFILE_Head_Recoding)&CacenBuff[0];
	

	
	
	
	if (File_head_recod->_FILE_Index != 0x454c4946 && File_head_recod->_FILE_Index > 0)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, "Ѱ�ҵ������ļ���¼ͷ��������,��������:%lu",File_head_recod->_FILE_Index);
		return false;
	} 

	if(File_head_recod->_FILE_Index == 0x454c4946 && File_head_recod->_Flags[0] == 0)
	{
		RtlCopyMemory(&CacenBuff[510], &CacenBuff[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
		RtlCopyMemory(&CacenBuff[1022],&CacenBuff[File_head_recod->_Update_Sequence_Number[0]+4], 2);

		ATTriBase[0] = (LATTRIBUTE_HEADS)&CacenBuff[File_head_recod->_First_Attribute_Dev[0]];

		for (DWORD i_attr = 0; i_attr < 30; i_attr++)
		{
			if(ATTriBase[i_attr]->_Attr_Type != 0xffffffff)
			{
				if (ATTriBase[i_attr]->_Attr_Type == 0x20)
				{
					DWORD h20Length = NULL;
					switch(ATTriBase[i_attr]->_PP_Attr)
					{
					case 0:
						if (ATTriBase[i_attr]->_AttrName_Length == 0)
						{
							h20Length = 24;
						} 
						else
						{
							h20Length = 24 + 2*ATTriBase[i_attr]->_AttrName_Length;
						}
						break;
					case 0x01:
						if (ATTriBase[i_attr]->_AttrName_Length == 0)
						{
							h20Length = 64;
						} 
						else
						{
							h20Length = 64 + 2*ATTriBase[i_attr]->_AttrName_Length;
						}
						break;
					}
					if (ATTriBase[i_attr]->_PP_Attr == 0)
					{
						while (h20Length != ATTriBase[i_attr]->_Attr_Length)
						{
							H20 = (LAttr_20H)((UCHAR*)&ATTriBase[i_attr][0]+h20Length);
							if (H20->_H20_TYPE == 0x80)
							{
								(*H20FileRefer) = H20->_H20_FILE_Reference_Num.LowPart;
								break;
							}
							if(H20->_H20_Attr_Name_Length*2 > 0)
							{
								if ((H20->_H20_Attr_Name_Length*2+26)%8 != 0)
								{
									h20Length += (((H20->_H20_Attr_Name_Length * 2 + 26) / 8) * 8 + 8);
								}
								else if ((H20->_H20_Attr_Name_Length * 2 + 26) % 8 == 0)
								{
									h20Length += (H20->_H20_Attr_Name_Length * 2 + 26);
								}
							}
							else
							{
								h20Length += 32;
							}

						}
					}
					else if (ATTriBase[i_attr]->_PP_Attr == 1)
					{
						UCHAR *H20Data = NULL;
						DWORD64 H20DataRun = NULL;
						H20Data = (UCHAR*)&ATTriBase[i_attr][0];
						DWORD H20Offset = ATTriBase[i_attr]->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0];
						if (H20Data[H20Offset] != 0 && H20Data[H20Offset] < 0x50)
						{
							UCHAR adres_fig = H20Data[H20Offset] >> 4;
							UCHAR len_fig = H20Data[H20Offset] & 0xf;
							for (int w = adres_fig; w > 0; w--)
							{
								H20DataRun = H20DataRun | (H20Data[H20Offset+w+len_fig] << (8*(w-1)));
							}
						}

						memset(H20Buff, 0, SECTOR_SIZE * CU_NUM);
						Ret = ReadSQData(Drive,H20Buff, SECTOR_SIZE * CU_NUM, Start_NTFS_Addr * SECTOR_SIZE+
							H20DataRun * CU_NUM * SECTOR_SIZE,
							&BackBytesCount);		
						if(!Ret)
						{			
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr::ReadSQData��ȡ����Start_CUʧ��!"));
							return false;	
						}	
						h20Length = 0;
						H20 = (LAttr_20H)&H20Buff[h20Length];
						while (H20->_H20_TYPE != 0)
						{
							H20 = (LAttr_20H)&H20Buff[h20Length];
							if (H20->_H20_TYPE == 0x80)
							{
								(*H20FileRefer) = H20->_H20_FILE_Reference_Num.LowPart;
								break;
							}
							else if(H20->_H20_TYPE == 0)
							{
								break;
							}
							if(H20->_H20_Attr_Name_Length*2 > 0)
							{
								if ((H20->_H20_Attr_Name_Length*2+26) % 8 != 0)
								{
									h20Length += (((H20->_H20_Attr_Name_Length*2+26) / 8) * 8 + 8);
								}
								else if ((H20->_H20_Attr_Name_Length*2+26) % 8 == 0)
								{
									h20Length += (H20->_H20_Attr_Name_Length*2+26);
								}
							}
							else
							{
								h20Length += 32;
							}
						}
					}
				}
				if (!Found)
				{
					if (ATTriBase[i_attr]->_Attr_Type == 0x30)
					{
						switch(ATTriBase[i_attr]->_PP_Attr)
						{
						case 0:
							if (ATTriBase[i_attr]->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[i_attr][0]+24);
								H30_NAMES = (UCHAR*)&ATTriBase[i_attr][0]+24+66;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[i_attr][0]+24+2*ATTriBase[i_attr]->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[i_attr][0]+24+2*ATTriBase[i_attr]->_AttrName_Length+66;
							}
							break;
						case 0x01:
							if (ATTriBase[i_attr]->_AttrName_Length == 0)
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[i_attr][0]+64);
								H30_NAMES = (UCHAR*)&ATTriBase[i_attr][0]+64+66;
							} 
							else
							{
								H30 = (LAttr_30H)((UCHAR*)&ATTriBase[i_attr][0]+64+2*ATTriBase[i_attr]->_AttrName_Length);
								H30_NAMES = (UCHAR*)&ATTriBase[i_attr][0]+64+2*ATTriBase[i_attr]->_AttrName_Length+66;
							}
							break;
						}

						if (H30->_H30_FILE_Name_Length*2 > 0)
						{
							for(int namelen = 0; namelen < (H30->_H30_FILE_Name_Length*2); namelen ++)
							{
								int indx_n = 0;
								for(DWORD num = 0 ; num < fNameNum; num++)
								{

									for (UCHAR len = 0; len < fNamelen[num]; len++)
									{
										if (H30_NAMES[namelen+(len*2)] == fName[len+indx_n])
										{
											Found = true;
										}
										else
										{
											Found = false;
											break;
										}
									}

									if (fName[indx_n+fNamelen[num]] == ';' )
									{
										indx_n += (fNamelen[num]+1);
									}

									if (Found)
									{
										if ((namelen+fNamelen[num]*2) == (H30->_H30_FILE_Name_Length*2))
										{
											
											
											CFuncs::WriteLogInfo(SLT_INFORMATION, "���ļ���¼�ο�����:%lu",File_head_recod->_FR_Refer);
											
											(*ParentMft) = File_head_recod->_FR_Refer;
											fileList.append("FileName:");
					
											wchar_t * WirteName = new wchar_t[(H30->_H30_FILE_Name_Length*2)+1];
											if (NULL == WirteName)
											{
												CFuncs::WriteLogInfo(SLT_ERROR, _T("VirtualVmwareCheck:new:WirteName ���������ڴ�ʧ��!"));
												return false;
											}
											memset(WirteName,0,((H30->_H30_FILE_Name_Length*2)+1)*2);
											for (DWORD NameIndex = 0; NameIndex < (H30->_H30_FILE_Name_Length * 2); NameIndex += 2)
											{

												RtlCopyMemory(&WirteName[ NameIndex / 2], &H30_NAMES[NameIndex],2);
											}
											
											wstring wFileName = wstring((wchar_t*)&WirteName[0],(H30->_H30_FILE_Name_Length*2));
											delete WirteName;
											WirteName = NULL;
											string strFileName = CUrlConver::WstringToString(wFileName);
											fileList.append(strFileName);
											
											fileList.append(",");
											fileList.append("FileCreateTime:");
											string strCreateTime;
											if(!FileTimeConver(&H30->_H30_FILE_Built_TM[0], strCreateTime))
											{
												CFuncs::WriteLogInfo(SLT_ERROR, "GetH80DataRunAddr FileTimeConver ת������ʱ��ʧ��");
												return false;
											}
											fileList.append(strCreateTime);
											fileList.append(",");
											fileList.append("FileModifyTime:");
											string strModifyTime;
											if(!FileTimeConver(&H30->_H30_FILE_Recise_TM[0], strModifyTime))
											{
												CFuncs::WriteLogInfo(SLT_ERROR, "GetH80DataRunAddr FileTimeConver ת���޸�ʱ��ʧ��");
												return false;
											}
											fileList.append(strModifyTime);
											fileList.append(",");
											fileList.append("FileLastVistTime:");
											string strLastVisitTime;
											if(!FileTimeConver(&H30->_H30_FILE_Visit_LTM[0], strLastVisitTime))
											{
												CFuncs::WriteLogInfo(SLT_ERROR, "GetH80DataRunAddr FileTimeConver ת��������ʱ��ʧ��");
												return false;
											}
											fileList.append(strLastVisitTime);
											fileList.append(",");
											fileList.append("FileSize:");
											wstring wstrFileSize((wchar_t*)&H30->_H30_FILE_TSize[0],8);
											string strFileSize = CUrlConver::WstringToString(wstrFileSize);
											fileList.append(strFileSize);
											fileList.append(",");
											

											if ((*H20FileRefer) > 0)
											{
												if ((*H20FileRefer) != File_head_recod->_FR_Refer)
												{
													CFuncs::WriteLogInfo(SLT_INFORMATION, "���ļ���¼H80�ض�λ��H20�У��ض�λ�ļ��ο�����:%lu",(*H20FileRefer));
													return true;																									
												}
												else
												{
													(*H20FileRefer) = NULL;//��ͬ�ľ�û�ض�λ������Ϊ��
													Found = true;	
												}
											}
											else
											{
												Found = true;
											}

										}else
										{
											Found = false;
										}
									}

									if (Found)
									{
										break;
									}
								}	
								if (Found)
								{
									break;
								}
							}												
						}
					}
				}
				if (Found)
				{
					if (ATTriBase[i_attr]->_Attr_Type == 0x80)
					{

						if (ATTriBase[i_attr]->_PP_Attr == 0x01)
						{
							H80_data = (UCHAR*)&ATTriBase[i_attr][0];
							DWORD OFFSET = ATTriBase[i_attr]->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0];

							fileList.append("FileAddr:");
							UCHAR *H80Tem = (UCHAR*)malloc((ATTriBase[i_attr]->_Attr_Length-64)*2 + 17);
							if (NULL == H80Tem)
							{
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr malloc:H80Temʧ��!"));
								return false;
							}
							memset(H80Tem, 0, (ATTriBase[i_attr]->_Attr_Length-64)*2 + 17);
							UCHAR *JH80Tem = (UCHAR*)malloc((ATTriBase[i_attr]->_Attr_Length - 64) + 9);
							if (NULL == JH80Tem)
							{
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr malloc:JH80Temʧ��!"));
								return false;
							}
							memset(JH80Tem, 0, ((ATTriBase[i_attr]->_Attr_Length - 64) + 9));
							RtlCopyMemory(&JH80Tem[0], &H80_data[48], 8);
							RtlCopyMemory(&JH80Tem[8], &H80_data[OFFSET], (ATTriBase[i_attr]->_Attr_Length - 64));
							/*	UCHAR* pH80_data = H80_data + OFFSET;
							printf("%x\n", H80_data + OFFSET);
							for (DWORD i =0; i < (ATTriBase[i_attr]->_Attr_Length-64) ; i++)
							{
							printf("%0.2x",pH80_data[i]);
							}
							printf("\n");*/
							if(CBase64::Encode((const unsigned char*)JH80Tem, ((ATTriBase[i_attr]->_Attr_Length - 64) + 8), &H80Tem[0]
							, (ATTriBase[i_attr]->_Attr_Length-64)*2 + 17) < 0)
							{
								free(H80Tem);
								H80Tem = NULL;
								free(JH80Tem);
								JH80Tem = NULL;
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr:Encodeת��ʧ��!"));
								return false;
							}
							
							/*printf("%s\n", H80Tem);*/
							/*	int len = strlen((const char*)H80Tem);
							UCHAR *H80Temj = (UCHAR*)malloc(len + 1);
							if (NULL == H80Temj)
							{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr:malloc:H80Temʧ��!"));
							return false;
							}
							memset(H80Temj, 0, len + 1);
							if(CBase64::Decode(H80Tem, len, H80Temj, len) < 0)
							{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr:Decode����ʧ��!"));
							return false;
							}*/
							/*		printf("%x\n", H80Temj);
							for (DWORD i =0; i < len ; i++)
							{
							printf("%0.2x",H80Temj[i]);
							}*/
							/*printf("\n");*/
							
							/*fileList.append((char*)H80Tem, (ATTriBase[i_attr]->_Attr_Length-64)*2);*/
							
							fileList.append((char*)H80Tem, strlen((char*)H80Tem));
							fileList.append(",");
							free(JH80Tem);
							JH80Tem = NULL;
							free(H80Tem);
							H80Tem = NULL;

						}
						else if(ATTriBase[i_attr]->_PP_Attr == 0)
						{
							fileList.append("AddrData:");
							H80_data=(UCHAR*)&ATTriBase[i_attr][0];	
							UCHAR *H80Tem = (UCHAR *)malloc((ATTriBase[i_attr]->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length)*2+1);
							if (NULL == H80Tem)
							{
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr:malloc:H80Temʧ��!"));
								return false;
							}
							memset(H80Tem, 0, (ATTriBase[i_attr]->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length)*2+1);

							UCHAR* pBuff = H80_data + 24;
							if(CBase64::Encode((const unsigned char*)H80_data + 24, ATTriBase[i_attr]->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length, H80Tem
								, (ATTriBase[i_attr]->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length)*2) < 0)
							{
								free(H80Tem);
								H80Tem = NULL;
								CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr:Encodeת��ʧ��!"));
								return false;
							}
							/*	int len1 = strlen((char*)H80Tem);
							UCHAR *H80Temj = (UCHAR*)malloc(len1 + 1);
							if (NULL == H80Temj)
							{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr:malloc:H80Temʧ��!"));
							return false;
							}
							memset(H80Temj, 0, len1 + 1);
							if(CBase64::Decode(H80Tem, len1, H80Temj, len1) < 0)
							{
							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr:Decode����ʧ��!"));
							return false;
							}
							string strData = string((char*)H80Tem, (ATTriBase[i_attr]->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length)*2);*/
					/*		fileList.append((char*)H80Tem, (ATTriBase[i_attr]->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length)*2 );*/
							fileList.append((char*)H80Tem, strlen((char*)H80Tem));
							free(H80Tem);
							H80Tem = NULL;
							fileList.append(",");	
							
						}
					}
				}
				if (ATTriBase[i_attr]->_Attr_Length < 1024 && ATTriBase[i_attr]->_Attr_Length > 0)
				{
					ATTriBase[i_attr+1] = (LATTRIBUTE_HEADS)((UCHAR*)ATTriBase[i_attr]+ATTriBase[i_attr]->_Attr_Length);
				} 
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase[i_attr]->_Attr_Length);
					return false;
				}
			}
			else if (ATTriBase[i_attr]->_Attr_Type == 0xffffffff)
			{
				if (!Found)
				{
					(*H20FileRefer) = 0;
				}
				memset(CacenBuff, 0, FILE_SECTOR_SIZE);
				break;
			}
			else if(ATTriBase[i_attr]->_Attr_Type > 0xff && ATTriBase[i_attr]->_Attr_Type < 0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH80DataRunAddr:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
				return false;
			}
		}
	}
	return true;
}
bool  CDeletedFile::GetH20FileReferH80Addr(HANDLE Drive,UCHAR *CacenBuff,DWORD64 StartNtfsAddr,LONG64 StartMftAddr,string& fileList)
{
	DWORD BackBytesCount = NULL;
	bool bRet = false;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase[30] = {NULL};
	UCHAR *H80_data = NULL;
	
	memset(CacenBuff,0,FILE_SECTOR_SIZE);
	bRet = ReadSQData(Drive,CacenBuff, FILE_SECTOR_SIZE, StartNtfsAddr * SECTOR_SIZE+StartMftAddr* SECTOR_SIZE,
		&BackBytesCount);		
	if(!bRet)
	{			
		CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH20FileReferH80Addr::ReadSQData��ȡʧ��!"));
		return false;	
	}

	File_head_recod = (LFILE_Head_Recoding)&CacenBuff[0];

	if (File_head_recod->_FILE_Index != 0x454c4946 && File_head_recod->_FILE_Index > 0)
	{
		CFuncs::WriteLogInfo(SLT_INFORMATION, "GetH20FileReferH80Addr:Ѱ�ҵ������ļ���¼ͷ��������,��������:%lu",File_head_recod->_FILE_Index);
		return false;
	} 
	else if(File_head_recod->_FILE_Index == 0x454c4946 && File_head_recod->_Flags[0] != 0)
	{
		RtlCopyMemory(&CacenBuff[510], &CacenBuff[File_head_recod->_Update_Sequence_Number[0]+2], 2);//������������	
		RtlCopyMemory(&CacenBuff[1022],&CacenBuff[File_head_recod->_Update_Sequence_Number[0]+4], 2);
		ATTriBase[0] = (LATTRIBUTE_HEADS)&CacenBuff[File_head_recod->_First_Attribute_Dev[0]];
		for (DWORD i_attr = 0; i_attr < 25; i_attr++)
		{
			if(ATTriBase[i_attr]->_Attr_Type !=0xffffffff)
			{
				if (ATTriBase[i_attr]->_Attr_Type == 0x80)
				{
					if (ATTriBase[i_attr]->_PP_Attr == 0x01)
					{
						H80_data = (UCHAR*)&ATTriBase[i_attr][0];
						DWORD OFFSET = ATTriBase[i_attr]->TWOATTRIBUTEHEAD.NP_head._NPN_RunList_Offset[0];
						fileList.append("FileAddr:");
						fileList.append((char*)&H80_data[OFFSET],(ATTriBase[i_attr]->_Attr_Length-64));
						fileList.append(",");
						
					}
					else if(ATTriBase[i_attr]->_PP_Attr == 0)
					{
						fileList.append("AddrData:");
						H80_data=(UCHAR*)&ATTriBase[i_attr][0];	
						fileList.append((char*)&H80_data[24],ATTriBase[i_attr]->TWOATTRIBUTEHEAD.P_head._PN_AttrBody_Length);
						fileList.append(",");
					}
				}
				if (ATTriBase[i_attr]->_Attr_Length < 1024 && ATTriBase[i_attr]->_Attr_Length > 0)
				{
					ATTriBase[i_attr+1] = (LATTRIBUTE_HEADS)((UCHAR*)ATTriBase[i_attr]+ATTriBase[i_attr]->_Attr_Length);
				} 
				else
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH20FileReferH80Addr::������:%lu,ATTriBase[i_attr]->_Attr_Length���ȹ����Ϊ0!")
						,ATTriBase[i_attr]->_Attr_Length);
					return false;
				}
			}else if (ATTriBase[i_attr]->_Attr_Type == 0xffffffff)
			{

				memset(CacenBuff, 0, FILE_SECTOR_SIZE);
				break;
			}
			else if(ATTriBase[i_attr]->_Attr_Type > 0xff && ATTriBase[i_attr]->_Attr_Type < 0xffffffff)
			{
				CFuncs::WriteLogInfo(SLT_ERROR, _T("GetH20FileReferH80Addr:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
				return false;
			}
		}
	}
	return true;
}
bool CDeletedFile::GetFileNameAndPath(HANDLE P_Driver,DWORD64 StartNTFSaddr,vector<LONG64> StartMFTaddr,vector<DWORD64> StartMFTaddrLen,UCHAR CuNumber
	,DWORD ParentMFT,UCHAR *CacheBuffer,string& fileLists,const char* chkdk)
{
	DWORD MFTnumber = NULL;
	bool  bRet = false;
	DWORD BackBytesCount = NULL;
	LFILE_Head_Recoding File_head_recod = NULL;
	LATTRIBUTE_HEADS ATTriBase[30] = {NULL};
	LAttr_30H H30 = NULL;
	UCHAR *H30_NAMES = NULL;
	string strTempPath;

	File_head_recod = (LFILE_Head_Recoding)&CacheBuffer[0];
	ATTriBase[0] = (LATTRIBUTE_HEADS)&CacheBuffer[56];

	MFTnumber = ParentMFT;
	//fileLists.append("FilePath:");
	
	
	
	DWORD numbers=NULL;
	while (MFTnumber != 5 && MFTnumber != 0)
	{
		if (numbers > 50)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, "GetFileNameAndPath:numbers ·���ļ�����50��������!");
			return false;
		}
		numbers++;
		DWORD64 MftLenAdd = NULL;
		LONG64 MftAddr = NULL;

		for (DWORD FMft = 0; FMft < StartMFTaddrLen.size(); FMft++)
		{
			if ((MFTnumber*2) <= (StartMFTaddrLen[FMft]*CuNumber+MftLenAdd))
			{
				MftAddr = (StartMFTaddr[FMft]*CuNumber+((MFTnumber*2)-MftLenAdd));
				break;
			} 
			else
			{
				MftLenAdd += (StartMFTaddrLen[FMft]*CuNumber);
			}
		}
		if (StartMFTaddrLen.size() == NULL)
		{
			MftAddr = StartMFTaddr[0]*CuNumber;
		}
		//�ȶ�ȡ�����ļ���¼,����
		bRet = ReadSQData(P_Driver,&CacheBuffer[0],FILE_SECTOR_SIZE,
			StartNTFSaddr * SECTOR_SIZE+MftAddr*SECTOR_SIZE,
			&BackBytesCount);		
		if(!bRet)
		{			
			CFuncs::WriteLogInfo(SLT_ERROR, _T("GetFileNameAndPath:ReadSQData::��ȡ����MFT�ο���ʧ��!"));
			return false;	
		}	
		if (File_head_recod->_FILE_Index != 0x454c4946 && File_head_recod->_FILE_Index > 0)
		{
			CFuncs::WriteLogInfo(SLT_ERROR, _T("�ҵ������ļ���¼����!"));
			return false;
		} 
		else if(File_head_recod->_FILE_Index == 0x454c4946)
		{
			RtlCopyMemory(&CacheBuffer[510],&(File_head_recod->_Update_array[0]),2);	
			RtlCopyMemory(&CacheBuffer[1022],&(File_head_recod->_Update_array[2]),2);
			for (int i_attr = 0; i_attr < 20; i_attr++)
			{
				if(ATTriBase[i_attr]->_Attr_Type != 0xffffffff)
				{
					if (ATTriBase[i_attr]->_Attr_Type == 0x30)
					{
						switch(ATTriBase[i_attr]->_PP_Attr)
						{
						case 0:
							if (ATTriBase[i_attr]->_AttrName_Length == 0)
							{
								H30=(LAttr_30H)((UCHAR*)&ATTriBase[i_attr][0]+24);
								H30_NAMES=(UCHAR*)&ATTriBase[i_attr][0]+24+66;
							} 
							else
							{
								H30=(LAttr_30H)((UCHAR*)&ATTriBase[i_attr][0]+24+2*ATTriBase[i_attr]->_AttrName_Length);
								H30_NAMES=(UCHAR*)&ATTriBase[i_attr][0]+24+2*ATTriBase[i_attr]->_AttrName_Length+66;
							}
							break;
						case 0x01:
							if (ATTriBase[i_attr]->_AttrName_Length==0)
							{
								H30=(LAttr_30H)((UCHAR*)&ATTriBase[i_attr][0]+64);
								H30_NAMES=(UCHAR*)&ATTriBase[i_attr][0]+64+66;
							} 
							else
							{
								H30=(LAttr_30H)((UCHAR*)&ATTriBase[i_attr][0]+64+2*ATTriBase[i_attr]->_AttrName_Length);
								H30_NAMES=(UCHAR*)&ATTriBase[i_attr][0]+64+2*ATTriBase[i_attr]->_AttrName_Length+66;
							}
							break;
						}

						RtlCopyMemory(&MFTnumber,&H30->_H30_Parent_FILE_Reference.LowPart,4);
						strTempPath.append("//");	
						if (!UnicodeToZifu(&H30_NAMES[0], strTempPath, H30->_H30_FILE_Name_Length*2))
						{

							CFuncs::WriteLogInfo(SLT_ERROR, _T("GetFileNameAndPath:Unicode_To_Zifu:ת��ʧ��!"));
							return false;
						}	
						
						
						break;													
					}
					if (ATTriBase[i_attr]->_Attr_Length < 1024 && ATTriBase[i_attr]->_Attr_Length > 0)
					{
						ATTriBase[i_attr+1] = (LATTRIBUTE_HEADS)((UCHAR*)ATTriBase[i_attr]+ATTriBase[i_attr]->_Attr_Length);
					} 
					else
					{								
						CFuncs::WriteLogInfo(SLT_ERROR, _T("GetFileNameAndPath:���Գ��ȹ���!,������:%lu"),ATTriBase[i_attr]->_Attr_Length);
						return false;
					}
				}
				else if (ATTriBase[i_attr]->_Attr_Type == 0xffffffff)
				{
					break;
				}
				else if(ATTriBase[i_attr]->_Attr_Type > 0xff && ATTriBase[i_attr]->_Attr_Type < 0xffffffff)
				{
					CFuncs::WriteLogInfo(SLT_ERROR, _T("GetFileNameAndPath:��ȡATTriBase[i_attr]->_Attr_Type����0xffffffff����"));
					return false;
				}
			}
		}
	}
	
	strTempPath.append("//");
	strTempPath += chkdk;//���̷�
	int laststring = (strTempPath.length() - 1);
	for (int i = (strTempPath.length()-1); i > 0; i--)
	{
		if (strTempPath[i] == '/' && strTempPath[i-1] == '/')
		{
			if ((laststring - i))
			{
				fileLists.append(&strTempPath[i + 1], (laststring - i));

				if (laststring == (strTempPath.length() - 1))
				{
					fileLists.append(":\\");
				}
				else
				{
					fileLists.append("\\");
				}
				laststring = (i - 2);
			}
			
		}
	}
	
	return true;
}
bool CDeletedFile::SearchDeletedFile(const char* partion, const char* fileType, string& fileLists)
{	
	DWORD  dwError=NULL;
	m_ParentDevice = CreateFile("\\\\.\\PhysicalDrive0",
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	if (m_ParentDevice == INVALID_HANDLE_VALUE) 
	{
		dwError = GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, _T("DataRecovery:CreateFile(\\\\.\\PhysicalDrive0ʧ��!,\
										   ���󷵻���: dwError = %d"), dwError);		
		return false;
	}
	/************************************************************************/
	/* ��ø���MBR��GPT������ʼ��ַ                                                                     */
	/************************************************************************/
	vector<DWORD64> vHostStarPartition;	//����vwctor���洢����������ʼ��ַ
	UCHAR *ReadSectorBuffer = (UCHAR*)malloc(FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (NULL == ReadSectorBuffer)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "DataRecovery:malloc ����ReadSectorBuffer��ַʧ��!,������%d", GetLastError());
		return false;
	}
	memset(ReadSectorBuffer, 0, FILE_SECTOR_SIZE + SECTOR_SIZE);
	if (!GetHostNtfsStartAddr(m_ParentDevice, vHostStarPartition, ReadSectorBuffer))
	{		
		CFuncs::WriteLogInfo(SLT_ERROR, _T("DataRecovery:GetHostNtfsStartAddr::����ȡ������Ϣʧ��!"));
		free(ReadSectorBuffer);
		ReadSectorBuffer=NULL;
		return false;
	} 
	//��ȡ�����Ŷ�Ӧ�ķ����̷�
	vector<char> v_chkdsk;//���̷�������
	vector<DWORD64> v_dwDiskNumber;//�洢�̷���Ӧ������
	DWORD64 PititionNuber = NULL;//�洢�����̷�����

	if(!GetPititionName(v_chkdsk, v_dwDiskNumber))
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("DataRecovery:GetPititionName::��ȡ������Ϣʧ��!"));	
		return false;
	}		
	for (DWORD diskNum = 0; diskNum < v_chkdsk.size(); diskNum++)
	{
		char* disk = &v_chkdsk[diskNum];
		if (0 == _strnicmp(partion, disk, 1))
		{
			PititionNuber = v_dwDiskNumber[diskNum];
			break;
		}	
	}
	
	CFuncs::WriteLogInfo(SLT_INFORMATION, _T("DataRecovery:��ȡ��������%llu!"),PititionNuber);
	//���ݶ�Ӧ�̷���ȡ����
	DWORD64 hostPatition = NULL;
	UCHAR m_HostPatitionCuNum = NULL;

	for (DWORD diskNum = 0; diskNum < vHostStarPartition.size(); diskNum += 2)
	{
		if (PititionNuber == vHostStarPartition[diskNum])
		{
			hostPatition = vHostStarPartition[diskNum+1];
			break;
		}
	}
	if (NULL == hostPatition)
	{
		CFuncs::WriteLogInfo(SLT_ERROR, _T("DataRecovery:hostPatition::���ݷ����Ż�ȡ��ʼ������ַΪ��!"));
		
		return false;
	}
	vector<LONG64> v_HostStarMFTAddr;
	vector<DWORD64> v_HostStarMFTLen;
	//��ȡ�˷������е�MFT��ַ
	if (!GetMFTAddr(m_ParentDevice, hostPatition, v_HostStarMFTAddr, v_HostStarMFTLen, &m_HostPatitionCuNum, ReadSectorBuffer))//����Ϊtrue
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "DataRecovery:GetMFTAddr:��ȡMFT��ʼ��ַʧ��");
		free(ReadSectorBuffer);
		ReadSectorBuffer=NULL;
		return false;
	}
	if (NULL == v_HostStarMFTAddr.size())
	{
		CFuncs::WriteLogInfo(SLT_ERROR, "DataRecovery:MFT��ַΪ0");
		free(ReadSectorBuffer);
		ReadSectorBuffer=NULL;
		return false;
	}
	vector<DWORD> RecoveryNameLen;//������������
	DWORD CheckNumber = NULL;//������������
	DWORD checkfist = NULL;
	//printf("\n");
	for (DWORD NameLen = 0; NameLen < strlen(fileType); NameLen++)
	{
		if (fileType[NameLen] == ';')
		{
			RecoveryNameLen.push_back(NameLen - checkfist);
			checkfist = NameLen+1;
			CheckNumber++;
		}
		if (NameLen == (strlen(fileType) - 1))
		{
			RecoveryNameLen.push_back(NameLen - checkfist + 1);
			CheckNumber++;
		}
	}
	DWORD ReferNum = NULL;
	DWORD H20FileRefer = NULL;
	DWORD index_parentmft = NULL;
	DWORD ParantMftNumber = NULL;
	UCHAR *H20Buff = (UCHAR*)malloc(FILE_SECTOR_SIZE * m_HostPatitionCuNum + SECTOR_SIZE);
	if (NULL == H20Buff)
	{
		dwError=GetLastError();
		CFuncs::WriteLogInfo(SLT_ERROR, "DataRecovery:malloc:����H20Buff�ڴ�ʧ��!, ���󷵻���: dwError = %d", dwError);
		return false;
	}
	memset(H20Buff, 0, FILE_SECTOR_SIZE * m_HostPatitionCuNum + SECTOR_SIZE);

	for (DWORD FileRecNum = 0; FileRecNum < v_HostStarMFTAddr.size(); FileRecNum++)
	{
		ReferNum = 0;
		while(GetH80DataRunAddr(m_ParentDevice, ReadSectorBuffer, hostPatition, v_HostStarMFTAddr[FileRecNum], m_HostPatitionCuNum
			,ReferNum, &H20FileRefer, H20Buff, CheckNumber, fileType, RecoveryNameLen, fileLists, &ParantMftNumber))
		{
			if (H20FileRefer > 0)
			{
				DWORD64 MftLen = NULL;
				DWORD64 StartMftRfAddr = NULL;

				for (DWORD FRN=0; FRN < v_HostStarMFTLen.size(); FRN++)
				{
					if ((H20FileRefer*2) < (MftLen+v_HostStarMFTLen[FRN] * m_HostPatitionCuNum))
					{
						StartMftRfAddr = v_HostStarMFTAddr[FRN] * m_HostPatitionCuNum+(H20FileRefer*2 - MftLen);
						break;
					} 
					else
					{ 
						MftLen += (v_HostStarMFTLen[FRN] * m_HostPatitionCuNum);
					}
				}
				if(!GetH20FileReferH80Addr(m_ParentDevice, ReadSectorBuffer, hostPatition, StartMftRfAddr, fileLists))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "DataRecovery:GetH20FileReferH80Addr:��ȡH20H80��ַʧ��ʧ��");
					break;
				}
			}

			if (ParantMftNumber > 0)
			{				
				fileLists.append("FilePath:");
				if (!GetFileNameAndPath(m_ParentDevice, hostPatition, v_HostStarMFTAddr, v_HostStarMFTLen, m_HostPatitionCuNum
					, ParantMftNumber, ReadSectorBuffer, fileLists,partion))
				{
					CFuncs::WriteLogInfo(SLT_ERROR, "DataRecovery:GetFileNameAndPath:��ȡ·��ʧ��!");
					free(ReadSectorBuffer);
					ReadSectorBuffer = NULL;
					free(H20Buff);
					H20Buff = NULL;
					return false;
				}	
				fileLists.append(";");
			}

			ReferNum++;
			if ((ReferNum * 2) > v_HostStarMFTLen[FileRecNum] * m_HostPatitionCuNum)
			{
				CFuncs::WriteLogInfo(SLT_INFORMATION, "���굱ǰMFT��ַ���е��ļ���¼!");
				break;
			}
		}
		CFuncs::WriteLogInfo(SLT_INFORMATION, "��Ѱ���ļ���¼����ReferNum��%lu",ReferNum);
	}


	free(ReadSectorBuffer);
	ReadSectorBuffer = NULL;
	free(H20Buff);
	H20Buff = NULL;

	vector<LONG64>().swap(v_HostStarMFTAddr);
	vector<DWORD64>().swap( v_HostStarMFTLen);
	vector<DWORD>().swap(RecoveryNameLen);

	return true;
}
