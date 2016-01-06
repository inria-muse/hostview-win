#pragma once

#include "aclapi.h"
#include "tchar.h"

#define TEMP_PATH L"temp"

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
#define MessageQueryInstalledApps	14

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
		this->type = 0;
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
