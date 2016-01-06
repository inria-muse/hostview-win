#ifndef __HANDLES_H_
#define __HANDLES_H_

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <stdio.h>

#ifndef NTSTATUS 
#define NTSTATUS LONG
#endif

#define NT_SUCCESS(x) ((x) >= 0)
#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004

#define SystemHandleInformation 16
#define ObjectBasicInformation 0
#define ObjectNameInformation 1
#define ObjectTypeInformation 2

typedef NTSTATUS (WINAPI *_NtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
    );
typedef NTSTATUS (WINAPI *_NtDuplicateObject)(
    HANDLE SourceProcessHandle,
    HANDLE SourceHandle,
    HANDLE TargetProcessHandle,
    PHANDLE TargetHandle,
    ACCESS_MASK DesiredAccess,
    ULONG Attributes,
    ULONG Options
    );
typedef NTSTATUS (WINAPI *_NtQueryObject)(
    HANDLE ObjectHandle,
    ULONG ObjectInformationClass,
    PVOID ObjectInformation,
    ULONG ObjectInformationLength,
    PULONG ReturnLength
    );

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _SYSTEM_HANDLE
{
    ULONG ProcessId;
    BYTE ObjectTypeNumber;
    BYTE Flags;
    USHORT Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, *PSYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
    ULONG HandleCount;
    SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef enum _POOL_TYPE
{
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS
} POOL_TYPE, *PPOOL_TYPE;

typedef struct _OBJECT_TYPE_INFORMATION
{
    UNICODE_STRING Name;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccess;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    USHORT MaintainTypeList;
    POOL_TYPE PoolType;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;

PVOID GetLibraryProcAddress(PSTR LibraryName, PSTR ProcName)
{
    return GetProcAddress(GetModuleHandleA(LibraryName), ProcName);
}

#include <map>
#include <vector>
using namespace std;

#define HANDLE_TYPE_FILE	28 

DWORD QueryOpenedHandle(TCHAR *szHandleName)
{
	DWORD dwResult = -1;

	_NtQuerySystemInformation NtQuerySystemInformation = 
		(_NtQuerySystemInformation) GetLibraryProcAddress("ntdll.dll", "NtQuerySystemInformation");
	_NtDuplicateObject NtDuplicateObject =
		(_NtDuplicateObject) GetLibraryProcAddress("ntdll.dll", "NtDuplicateObject");
	_NtQueryObject NtQueryObject =
		(_NtQueryObject) GetLibraryProcAddress("ntdll.dll", "NtQueryObject");
	NTSTATUS status;
	PSYSTEM_HANDLE_INFORMATION handleInfo;
	ULONG handleInfoSize = 0x10000;
	ULONG i;

	handleInfo = (PSYSTEM_HANDLE_INFORMATION) malloc(handleInfoSize);

	while ((status = NtQuerySystemInformation(SystemHandleInformation,
		handleInfo, handleInfoSize, NULL)) == STATUS_INFO_LENGTH_MISMATCH)
	{
		handleInfo = (PSYSTEM_HANDLE_INFORMATION) realloc(handleInfo, handleInfoSize *= 2);
	}

	/* NtQuerySystemInformation stopped giving us STATUS_INFO_LENGTH_MISMATCH. */
	if (!NT_SUCCESS(status))
	{
		printf("NtQuerySystemInformation failed!\n");
		return dwResult;
	}

	// group by procs
	map<ULONG, vector<SYSTEM_HANDLE>> handles;

	for (i = 0; i < handleInfo->HandleCount; i++)
	{
		SYSTEM_HANDLE handle = handleInfo->Handles[i];
		if (handle.ObjectTypeNumber == HANDLE_TYPE_FILE)
			handles[handle.ProcessId].push_back(handle);
	}

	for (map<ULONG, vector<SYSTEM_HANDLE>>::iterator it = handles.begin(); it != handles.end(); it ++)
	{
		DWORD pid = it->first;
		DWORD dwSession = 0;
		if (!ProcessIdToSessionId(pid, &dwSession) || !dwSession)
			continue;

		HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
		if (!processHandle)
			continue;

		if (dwResult != -1)
			break;

		for (i = 0; i < it->second.size(); i ++)
		{
			if (dwResult != -1)
				break;

			SYSTEM_HANDLE handle = it->second[i];

			if (!handle.GrantedAccess)
				continue;

			HANDLE dupHandle = NULL;
			PVOID objectNameInfo;
			UNICODE_STRING objectName;
			ULONG returnLength;

			/* Duplicate the handle so we can query it. */
			if (!NT_SUCCESS(NtDuplicateObject(processHandle, (HANDLE) handle.Handle, GetCurrentProcess(), &dupHandle, 0, 0, DUPLICATE_SAME_ACCESS)))
			{
				continue;
			}

			/* Query the object name (unless it has an access of 
			   0x0012019f, on which NtQueryObject could hang. */
			if (handle.GrantedAccess == 0x0012019f || GetFileType(dupHandle) == FILE_TYPE_PIPE)
			{
				/* We have the type, so display that. */
				CloseHandle(dupHandle);
				continue;
			}

			objectNameInfo = malloc(0x1000);

			if (!NT_SUCCESS(NtQueryObject(dupHandle, ObjectNameInformation, objectNameInfo, 0x1000, &returnLength)))
			{
				/* Reallocate the buffer and try again. */
				objectNameInfo = realloc(objectNameInfo, returnLength);
				if (!NT_SUCCESS(NtQueryObject(dupHandle, ObjectNameInformation, objectNameInfo, returnLength, NULL)))
				{
					/* We have the type name, so just display that. */
					free(objectNameInfo);
					CloseHandle(dupHandle);
					continue;
				}
			}

			/* Cast our buffer into an UNICODE_STRING. */
			objectName = *(PUNICODE_STRING) objectNameInfo;

			/* Print the information! */
			if (objectName.Length)
			{
				if (_tcscmp(szHandleName, objectName.Buffer) == 0)
				{
					dwResult = pid;
				}
			}

			free(objectNameInfo);
			CloseHandle(dupHandle);
		}

		CloseHandle(processHandle);
	}

	free(handleInfo);
	return dwResult;
}

#endif
