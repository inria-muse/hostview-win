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
#include "stdafx.h"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include "comm.h"

Message SendServiceMessage(const Message &message)
{
	Message result;

	do
	{
		HANDLE hPipe = CreateFile(HOSTVIEW_PIPE, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hPipe != INVALID_HANDLE_VALUE)
		{
			DWORD dwSize = sizeof(message);
			WriteFile(hPipe, &message, dwSize, &dwSize, NULL);

			dwSize = sizeof(result);
			ReadFile(hPipe, &result, dwSize, &dwSize, NULL);

			CloseHandle(hPipe);
			break;
		}
		else
		{
			if (GetLastError() == ERROR_PIPE_BUSY)
			{
				if (!WaitNamedPipe(HOSTVIEW_PIPE, NMPWAIT_USE_DEFAULT_WAIT))
				{
					continue;
				}
			}
			else
			{
				// real error
				break;
			}
		}
	}
	while (true);

	return result;
}


HANDLE hThread = NULL;
bool bClosing = false;

DWORD WINAPI CommThreadProc(LPVOID lpParameter)
{
	CMessagesCallback *pCallback = (CMessagesCallback *) lpParameter;

	if (!pCallback)
		return 0;

	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;

	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);

	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = &sd;

	HANDLE hPipe = CreateNamedPipe(HOSTVIEW_PIPE, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 0, 0, 0, &sa);
	
	DWORD dwErr = GetLastError();
	printf("CreateNamedPipe-> 0x%X, last error: %d\r\n", hPipe, dwErr);

	OVERLAPPED ov = {0};
	ov.hEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

	if (hPipe != INVALID_HANDLE_VALUE)
	{
		Message message;
		DWORD dwSize = sizeof(message);

		while (!bClosing)
		{

			DisconnectNamedPipe(hPipe);
			FlushFileBuffers(hPipe);
			DisconnectNamedPipe(hPipe);
			ResetEvent(ov.hEvent);

			ConnectNamedPipe(hPipe, &ov);


			while (!bClosing)
			{
				if (WaitForSingleObject(ov.hEvent, 500) == WAIT_OBJECT_0)
				{
					if (ReadFile(hPipe, &message, dwSize, &dwSize, NULL))
					{
						if (dwSize == sizeof(message))
						{
							Message result = pCallback->OnMessage(message);
							dwSize = sizeof(result);
							WriteFile(hPipe, &result, dwSize, &dwSize, NULL);
						}
					}
					break;
				}
				else
				{
					// TODO: other stuff?
					continue;
				}
			}
		}

		CloseHandle(hPipe);

		return true;
	}

	return false;
}

/**
 * Starts message listening.
 **/
bool StartServiceCommunication(CMessagesCallback& callback)
{
	if (!hThread)
	{
		bClosing = false;
		hThread = CreateThread(NULL, NULL, CommThreadProc, &callback, NULL, NULL);
		return true;
	}
	return false;
}

bool StopServiceCommunication()
{
	if (hThread != NULL)
	{
		bClosing = true;
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
		hThread = NULL;
		return true;
	}

	return false;
}

