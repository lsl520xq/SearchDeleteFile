// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� SEARCHDELETEFILE_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// SEARCHDELETEFILE_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef SEARCHDELETEFILE_EXPORTS
#define SEARCHDELETEFILE_API EXTERN_C __declspec(dllexport)
#else
#define SEARCHDELETEFILE_API EXTERN_C __declspec(dllimport)
#endif


#include "../Common/Funcs.h"
#include "DeletedFile.h"
#include "FileRecovery.h"

#define VL_MAGIC_NUMBER (0x23E72DAC)

SEARCHDELETEFILE_API char* SearchDeletedFileInfo(const int magic, const char* diskPartition, const char* fileType);

SEARCHDELETEFILE_API void DestroyBuffer(char* buff);

SEARCHDELETEFILE_API int DeletedFileRecovery(const int magic, const char* fileName, const char* fileAddr, const char* diskPartition, const char* recoveryPath);


