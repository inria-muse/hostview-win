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
#pragma once

#include <tchar.h>
#include <vector>
#include <string>

#include "comm.h"
#include "Capture.h"

#if defined(KNOWNLIBRARY_EXPORT) // inside DLL
#   define KNOWNAPI   __declspec(dllexport)
#else // outside DLL
#   define KNOWNAPI   __declspec(dllimport)
#endif  // KNOWNLIBRARY_EXPORT

#ifndef tstring
#define tstring wstring
#endif

#define KNOWN_NETWORKS_FILE ".\\temp\\networks"

// sends a message to the service to label a network
void KNOWNAPI LabelNetwork(const NetworkInterface &ni, TCHAR *szProfile);

// converts this interface to a command line;
void KNOWNAPI NetworkInterfaceToCommand(const NetworkInterface &ni, TCHAR *szCmdLine, size_t nSize);

// extracts the interface from the command line;
bool KNOWNAPI CommandToNetworkInterface(TCHAR szCmdLine[MAX_PATH], NetworkInterface * &pni);
// needed to clear what was allocated by previous function due to diff runtimes
void KNOWNAPI ClearNetworkInterface(NetworkInterface * &pni);

class KNOWNAPI CKnownNetworks
{
public:
	CKnownNetworks(void);
	~CKnownNetworks(void);

	void LoadKnownNetworks();
	void SaveKnownNetworks();

	void OnNetworkLabel(TCHAR *szGUID, TCHAR *szBSSID, TCHAR *szLabel);
	bool IsKnown(const NetworkInterface &ni);
};

