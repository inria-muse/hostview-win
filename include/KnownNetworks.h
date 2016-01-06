#pragma once

#include "comm.h"
#include "Capture.h"
#include <stdio.h>

#if defined(KNOWNLIBRARY_EXPORT) // inside DLL
#   define KNOWNAPI   __declspec(dllexport)
#else // outside DLL
#   define KNOWNAPI   __declspec(dllimport)
#endif  // KNOWNLIBRARY_EXPORT

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

	void OnNetworkLabel(Message &message);
	bool IsKnown(const NetworkInterface &ni);
};

