#pragma once

#include <vector>

#define ConnectionTimeout 500

#if defined(UPDATELIBRARY_EXPORT) // inside DLL
#   define UPDATEAPI   __declspec(dllexport)
#else // outside DLL
#   define UPDATEAPI   __declspec(dllimport)
#endif  // UPDATELIBRARY_EXPORT

UPDATEAPI BOOL CheckForUpdate();
UPDATEAPI BOOL DownloadUpdate(TCHAR *szUpdatePath, DWORD dwSize);
UPDATEAPI BOOL GetProductVersion(char *szProductVersion, DWORD dwSize);

UPDATEAPI BOOL DownloadFile(TCHAR *szUrl, TCHAR *szFile);

/**
 * Given a relative file path it pulls it from UpdateLocation
 **/
UPDATEAPI VOID PullFile(TCHAR *szFilePath);

extern "C" UPDATEAPI bool QueryPublicInfo(std::vector<std::string> * &info);
extern "C" UPDATEAPI void FreePublicInfo(std::vector<std::string> * &info);
