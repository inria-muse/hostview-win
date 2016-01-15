/**
* The MIT License (MIT)
*
* Copyright (c) 2015-2016 MUSE / Inria
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
**/
#include "StdAfx.h"

#include "KnownNetworks.h"
#include "Upload.h"

std::vector<std::tstring> knownBSSIDs;
std::vector<std::tstring> knownGUIDs;
std::vector<std::tstring> knownLabels;

CKnownNetworks::CKnownNetworks(void)
{
	LoadKnownNetworks();
}


CKnownNetworks::~CKnownNetworks(void)
{
}

void LoadNetworkInterface(TCHAR *szLine)
{
	TCHAR szGUID[MAX_PATH] = {0};
	TCHAR szBSSID[MAX_PATH] = {0};
	TCHAR szProfile[MAX_PATH] = {0};

	TCHAR *szCtx = NULL;
	TCHAR *szToken = _tcstok_s(szLine, _T(","), &szCtx);

	if (szToken)
	{
		_tcscpy_s(szGUID, szToken);

		szToken = _tcstok_s(NULL, _T(","), &szCtx);
		if (szToken)
		{
			_tcscpy_s(szBSSID, szToken);

			szToken = _tcstok_s(NULL, _T(","), &szCtx);
			if (szToken)
			{
				_tcscpy_s(szProfile, szToken);

				knownGUIDs.push_back(szGUID);
				knownBSSIDs.push_back(szBSSID);
				knownLabels.push_back(szProfile);
			}
		}
	}
}

void CKnownNetworks::LoadKnownNetworks()
{
	knownBSSIDs.clear();
	knownLabels.clear();
	knownGUIDs.clear();

	FILE *f = NULL;
	_tfopen_s(&f, _T(KNOWN_NETWORKS_FILE), _T("r"));
	if (f)
	{
		TCHAR szBuffer[1024] = {0};
		while (_fgetts(szBuffer, _countof(szBuffer), f))
		{
			TCHAR *comma = _tcsstr(szBuffer, _T(","));
			if (!comma)
			{
				continue;
			}

			// remove new line;
			if (szBuffer[_tcslen(szBuffer) - 1] == '\n')
			{
				szBuffer[_tcslen(szBuffer) - 1] = 0;
			}

			LoadNetworkInterface(szBuffer);
		}
		fclose(f);
	}
}

void CKnownNetworks::SaveKnownNetworks()
{
	FILE *f = NULL;

	_tfopen_s(&f, _T(KNOWN_NETWORKS_FILE), _T("w"));

	if (f)
	{
		for (size_t i = 0; i < knownBSSIDs.size(); i ++)
		{
			_ftprintf_s(f, _T("%s,%s,%s\n"), knownGUIDs[i].c_str(),
				knownBSSIDs[i].c_str(),
				knownLabels[i].c_str());
		}
		fclose(f);
	}
}

bool CKnownNetworks::IsKnown(const NetworkInterface &ni)
{
	TCHAR szSSID[MAX_PATH] = {0};
	_stprintf_s(szSSID, _T("%S"), ni.strSSID.c_str());
	for (size_t i = 0; i < knownBSSIDs.size(); i ++)
	{
		if ((ni.wireless && _tcscmp(szSSID, knownBSSIDs[i].c_str()) == 0)
			|| (ni.strBSSID == knownBSSIDs[i]))
		{
			return true;
		}
	}

	return false;
}

void CKnownNetworks::OnNetworkLabel(Message &message)
{
	// add label
	LoadNetworkInterface(message.szUser);

	// flush new data to the file
	SaveKnownNetworks();

	// do a copy of the file for upload
	CopyFileToSubmit(KNOWN_NETWORKS_FILE, true);
}

void LabelNetwork(const NetworkInterface &ni, TCHAR * szProfile)
{
	// TODO: should have a special message;
	Message message(MessageNetworkLabel);

	if (ni.wireless)
	{
		_stprintf_s(message.szUser, _T("%S,%S,%s"), ni.strName.c_str(),
			ni.strSSID.c_str(), szProfile);
	}
	else
	{
		_stprintf_s(message.szUser, _T("%S,%s,%s"), ni.strName.c_str(),
			ni.strBSSID.c_str(), szProfile);
	}

	SendServiceMessage(message);
}

// converts this interface to a command line;
void NetworkInterfaceToCommand(const NetworkInterface &ni, TCHAR *szCmdLine, size_t nSize)
{
	TCHAR szTempFile[MAX_PATH] = {0};

	_stprintf_s(szTempFile, _T("%s\\%d"), TEMP_PATH, GetTickCount());

	FILE *f = NULL;
	_tfopen_s(&f, szTempFile, _T("w"));

	if (f)
	{
		_ftprintf_s(f, _T("%s\n"), ni.strFriendlyName.c_str());
		_ftprintf_s(f, _T("%S\n"), ni.strSSID.c_str());
		_ftprintf_s(f, _T("%S\n"), ni.strName.c_str());
		_ftprintf_s(f, _T("%s\n"), ni.strBSSID.c_str());
		fclose(f);
	}

	_stprintf_s(szCmdLine, nSize, _T("HostView.exe /net:%s"), szTempFile);
}

// extracts the interface from the command line;
bool CommandToNetworkInterface(TCHAR szCmdLine[MAX_PATH], NetworkInterface * &pni)
{
	bool bResult = false;

	char szBuffer[MAX_PATH] = {0};
	TCHAR szLine[MAX_PATH] = {0};

	if (!_tcsstr(szCmdLine, _T("/net:")))
		return false;

	TCHAR *szTempFile = _tcsstr(szCmdLine, _T("/net:")) + _tcslen(_T("/net:"));

	FILE * f = NULL;
	_tfopen_s(&f, szTempFile, _T("r"));

	if (f)
	{
		pni = new NetworkInterface();

		_fgetts(szLine, _countof(szLine), f);
		szLine[_tcslen(szLine) - 1] = szLine[_tcslen(szLine) - 1] == '\n' ? 0 : szLine[_tcslen(szLine) - 1];
		pni->strFriendlyName = szLine;

		fgets(szBuffer, _countof(szBuffer), f);
		szBuffer[strlen(szBuffer) - 1] = szBuffer[strlen(szBuffer) - 1] == '\n' ? 0 : szBuffer[strlen(szBuffer) - 1];
		pni->strSSID = szBuffer;
		pni->wireless = strlen(szBuffer) > 0;

		fgets(szBuffer, _countof(szBuffer), f);
		szBuffer[strlen(szBuffer) - 1] = szBuffer[strlen(szBuffer) - 1] == '\n' ? 0 : szBuffer[strlen(szBuffer) - 1];
		pni->strName = szBuffer;

		_fgetts(szLine, _countof(szLine), f);
		szLine[_tcslen(szLine) - 1] = szLine[_tcslen(szLine) - 1] == '\n' ? 0 : szLine[_tcslen(szLine) - 1];
		pni->strBSSID = szLine;

		fclose(f);

		bResult = true;
	}

#ifndef _DEBUG
	DeleteFile(szTempFile);
#endif

	return bResult;
}

void ClearNetworkInterface(NetworkInterface * &pni)
{
	if (pni)
	{
		delete pni;
		pni = NULL;
	}
}

