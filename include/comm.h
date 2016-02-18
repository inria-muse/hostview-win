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

#include "aclapi.h"

#if defined(COMMLIBRARY_EXPORT) // inside DLL
#   define COMMAPI   __declspec(dllexport)
#else // outside DLL
#   define COMMAPI   __declspec(dllimport)
#endif  // COMMLIBRARY_EXPORT

/**
 * Simple Client - Server Communication
 **/
#define HOSTVIEW_PIPE _T("\\\\.\\pipe\\HostView")
#define HOSTVIEW_REG _T("Software\\HostView\\")

#define MessageError               -1
#define MessageResult               0
#define MessageStartCapture			1
#define MessageStopCapture			2
#define MessageUserActivity			3
#define MessageQueryStatus			4
#define MessageNetworkLabel			5
#define	MessageQuestionnaireShow	6
#define MessageQuestionnaireDone	7
#define MessageSpeakerUsage			8
#define MessageMicrophoneUsage		9
#define MessageMouseKeyUsage		10
#define MessageCameraUsage			11
#define MessageQueryLastApps		12
#define MessageQueryInstalledApps	13
#define MessageSuspend				14
#define MessageResume				15
#define MessageRestartSession       16
#define MessageCheckUpdate          17
#define MessageUpload               18

struct Message
{
	int type;
	
	// user activity
	DWORD dwPid;
	bool isFullScreen;
	bool isIdle;
	TCHAR szUser[MAX_PATH];

	Message()
	{
		this->type = MessageError;
		this->dwPid = 0;
		this->isFullScreen = false;
		this->isIdle = false;
		this->szUser[0] = 0;
	}

	Message(const Message& message)
	{
		this->type = message.type;

		_tcscpy_s(this->szUser, message.szUser);
		this->dwPid = message.dwPid;
		this->isFullScreen = message.isFullScreen;
		this->isIdle = message.isIdle;
	}

	Message(int type)
	{
		this->type = type;
		this->dwPid = 0;
		this->isFullScreen = false;
		this->isIdle = false;
		this->szUser[0] = 0;
	}

	Message(int type, TCHAR *szUser, DWORD dwPid, bool isFullScreen, bool isIdle)
	{
		this->type = type;

		_tcscpy_s(this->szUser, szUser);
		this->dwPid = dwPid;
		this->isFullScreen = isFullScreen;
		this->isIdle = isIdle;
	}
};


// client function
Message COMMAPI SendServiceMessage(const Message &message);

/**
 * Callback which receives messages from clients.
 **/
class CMessagesCallback
{
public:
	virtual Message OnMessage(Message &message) = 0;
};

DWORD WINAPI CommThreadProc(LPVOID lpParameter);

/**
 * Starts message listening.
 **/
bool COMMAPI StartServiceCommunication(CMessagesCallback& callback);
bool COMMAPI StopServiceCommunication();
