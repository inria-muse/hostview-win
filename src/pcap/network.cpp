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
#include "Capture.h"

#include <IPTypes.h>
#include <IPHlpApi.h>
#include <wlanapi.h>

#include <objbase.h>
#include <algorithm>

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))


#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/**
 * Queries Wlan Specific Details.
 **/
void QueryWlanAdapterInfo(GUID guid, NetworkInterface &networkInterface)
{
	DWORD dwVersion = 2;
	HANDLE hClient = NULL;

    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
    DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
	WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
	
	if (WlanOpenHandle(dwVersion, NULL, &dwVersion, &hClient) == ERROR_SUCCESS)
	{
		DWORD dwRes = WlanQueryInterface(hClient,
			&guid, wlan_intf_opcode_current_connection, NULL,
			&connectInfoSize, (PVOID *) &pConnectInfo, &opCode);

		if (dwRes == ERROR_SUCCESS)
		{
			networkInterface.strProfile = pConnectInfo->strProfileName;

			if (pConnectInfo->wlanAssociationAttributes.dot11Ssid.uSSIDLength)
			{
				// TODO: dangerous
				networkInterface.strSSID = reinterpret_cast<const char*>(pConnectInfo->wlanAssociationAttributes.dot11Ssid.ucSSID);
			}

			switch (pConnectInfo->wlanAssociationAttributes.dot11BssType)
			{
			case dot11_BSS_type_infrastructure:
				networkInterface.strBSSIDType = "infrastructure";
				break;
			case dot11_BSS_type_independent:
				networkInterface.strBSSIDType = "independent";
				break;
			default:
				networkInterface.strBSSIDType = "any";
				break;
			}

			// empty, will query now
			networkInterface.strBSSID = L"";

			for (int k = 0; k < sizeof (pConnectInfo->wlanAssociationAttributes.dot11Bssid); k++)
			{
				TCHAR szBSSID[5] = {0};
				if (k == 5)
				{
					_stprintf_s(szBSSID, L"%.2X", pConnectInfo->wlanAssociationAttributes.dot11Bssid[k]);
				}
				else
				{
					_stprintf_s(szBSSID, L"%.2X-", pConnectInfo->wlanAssociationAttributes.dot11Bssid[k]);
				}
				networkInterface.strBSSID += szBSSID;
			}
			
			switch (pConnectInfo->wlanAssociationAttributes.dot11PhyType)
			{
				case dot11_phy_type_fhss:
					networkInterface.strPHYType = "fhss";
					break;
				case dot11_phy_type_dsss:
					networkInterface.strPHYType = "dsss";
					break;
				case dot11_phy_type_irbaseband:
					networkInterface.strPHYType = "irbaseband";
					break;
				case dot11_phy_type_ofdm:
					networkInterface.strPHYType = "ofdm";
					break;
				case dot11_phy_type_hrdsss:
					networkInterface.strPHYType = "hrdsss";
					break;
				case dot11_phy_type_erp:
					networkInterface.strPHYType = "erp";
					break;
				case dot11_phy_type_ht:
					networkInterface.strPHYType = "802.11n";
					break;
				default:
					networkInterface.strPHYType = pConnectInfo->wlanAssociationAttributes.dot11PhyType;
					break;
			}

			networkInterface.phyIndex = pConnectInfo->wlanAssociationAttributes.uDot11PhyIndex;
			networkInterface.signal = pConnectInfo->wlanAssociationAttributes.wlanSignalQuality;
			networkInterface.rSpeed = pConnectInfo->wlanAssociationAttributes.ulRxRate;
			networkInterface.tSpeed = pConnectInfo->wlanAssociationAttributes.ulTxRate;
		}

		PULONG ulChannel = 0;
		DWORD dwSize = sizeof(ULONG);

		if (WlanQueryInterface(hClient, &guid, wlan_intf_opcode_channel_number, NULL, &dwSize, (PVOID *) &ulChannel, NULL) == ERROR_SUCCESS)
		{
			networkInterface.channel = *ulChannel;
		}

		PULONG ulRSSI = 0;
		if (WlanQueryInterface(hClient, &guid, wlan_intf_opcode_rssi, NULL, &dwSize, (PVOID *) &ulRSSI, NULL) == ERROR_SUCCESS)
		{
			networkInterface.rssi = *ulRSSI;
		}

		WlanCloseHandle(hClient, NULL);
		hClient = NULL;
	}
}

void QueryAdapterInfo(PIP_ADAPTER_ADDRESSES pCurrAddresses, NetworkInterface &networkInterface)
{
	int i = 0;
	IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
	networkInterface.strDescription = pCurrAddresses->Description;
	networkInterface.strFriendlyName = pCurrAddresses->FriendlyName;
	networkInterface.strName = pCurrAddresses->AdapterName;
	networkInterface.strDnsSuffix = pCurrAddresses->DnsSuffix;

	std::string &str = networkInterface.strName;
	str.erase(std::remove(str.begin(), str.end(), '{'), str.end());
	str.erase(std::remove(str.begin(), str.end(), '}'), str.end());

	IP_ADAPTER_GATEWAY_ADDRESS *pGateway = pCurrAddresses->FirstGatewayAddress;
	if (pGateway)
	{
		for (i = 0; pGateway != NULL; i++)
		{
			TCHAR szAddr[MAX_PATH] = {0};
			DWORD dwSize = _countof(szAddr);

			if (WSAAddressToString(pGateway->Address.lpSockaddr, pGateway->Address.iSockaddrLength, NULL, szAddr, &dwSize))
			{
				break;
			}
			else
			{
				networkInterface.gateways.push_back(szAddr);

				ULONG ulMACAddr[2] = {0}, ulSize = 6;
				ULONG addr = ((sockaddr_in *)pGateway->Address.lpSockaddr)->sin_addr.S_un.S_addr;

				if (pGateway->Address.lpSockaddr->sa_family == AF_INET
						&& SendARP(addr, 0, ulMACAddr, &ulSize) == NO_ERROR)
				{
					// send ARP to get "BSSID"
					LPBYTE pBuffer = (LPBYTE) ulMACAddr; 
					TCHAR szMac[MAX_PATH] = {0};
					_stprintf_s(szMac, _T("%02X-%02X-%02X-%02X-%02X-%02X"), pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], pBuffer[4], pBuffer[5]);
					networkInterface.strBSSID = szMac;
				}
			}

			pGateway = pGateway->Next;
		}
	}

	pDnServer = pCurrAddresses->FirstDnsServerAddress;
	if (pDnServer)
	{
		for (i = 0; pDnServer != NULL; i++)
		{
			TCHAR szAddr[MAX_PATH] = {0};
			DWORD dwSize = _countof(szAddr);

			if (WSAAddressToString(pDnServer->Address.lpSockaddr, pDnServer->Address.iSockaddrLength, NULL, szAddr, &dwSize))
			{
				break;
			}
			else
			{
				networkInterface.dnses.push_back(szAddr);
			}

			pDnServer = pDnServer->Next;
		}
	}

	IP_ADAPTER_UNICAST_ADDRESS_LH *pUnicast = pCurrAddresses->FirstUnicastAddress;
	if (pUnicast)
	{
		for (int i = 0; pUnicast != NULL; i ++)
		{
			TCHAR szAddr[MAX_PATH] = {0};
			DWORD dwSize = _countof(szAddr);

			if (WSAAddressToString(pUnicast->Address.lpSockaddr, pUnicast->Address.iSockaddrLength, NULL, szAddr, &dwSize))
			{
				break;
			}
			else
			{
				networkInterface.ips.push_back(szAddr);
			}

			pUnicast = pUnicast->Next;
		}
	}

	if (pCurrAddresses->PhysicalAddressLength != 0)
	{
		for (i = 0; i < (int) pCurrAddresses->PhysicalAddressLength; i++)
		{
			TCHAR szMac[5] = {0};
			if (i == (pCurrAddresses->PhysicalAddressLength - 1))
			{
				_stprintf_s(szMac, _T("%.2X"), (int) pCurrAddresses->PhysicalAddress[i]);
			}
			else
			{
				_stprintf_s(szMac, _T("%.2X-"), (int) pCurrAddresses->PhysicalAddress[i]);
			}
			networkInterface.strMac += szMac;
		}
	}

	networkInterface.tSpeed = pCurrAddresses->TransmitLinkSpeed;
	networkInterface.rSpeed = pCurrAddresses->ReceiveLinkSpeed;

	// TODO: we assume we've queried just eth and wlan

	if (pCurrAddresses->IfType == IF_TYPE_IEEE80211)
	{
		networkInterface.wireless = true;

		TCHAR szAdapter[MAX_PATH] = {0};
		_stprintf_s(szAdapter, _T("%S"), pCurrAddresses->AdapterName);

		GUID guid;
		IIDFromString(szAdapter, &guid);
		QueryWlanAdapterInfo(guid, networkInterface);
	}
}

/**
 * Queries the set of active interfaces connected.
 **/
void QueryActiveInterfaces(std::set<NetworkInterface> &interfaces)
{
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;

	ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_SKIP_ANYCAST;
	ULONG family = AF_UNSPEC;

	PIP_ADAPTER_ADDRESSES pAddresses = NULL;
	ULONG outBufLen = 0;
	ULONG Iterations = 0;

	PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;

	// Allocate a 15 KB buffer to start with.
	outBufLen = WORKING_BUFFER_SIZE;

	do
	{
		pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
		if (pAddresses == NULL)
		{
			printf("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
			exit(1);
		}
		dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
		if (dwRetVal == ERROR_BUFFER_OVERFLOW)
		{
			FREE(pAddresses);
			pAddresses = NULL;
		}
		else
		{
			break;
		}

		Iterations++;
	} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

	if (dwRetVal == NO_ERROR)
	{
		pCurrAddresses = pAddresses;
		while (pCurrAddresses)
		{
			if (pCurrAddresses->IfType != IF_TYPE_SOFTWARE_LOOPBACK
				&& pCurrAddresses->OperStatus == IfOperStatusUp
				&& !(pCurrAddresses->Flags & IP_ADAPTER_NO_MULTICAST))
			{
				NetworkInterface networkInterface;
				QueryAdapterInfo(pCurrAddresses, networkInterface);
				interfaces.insert(networkInterface);
			}

			pCurrAddresses = pCurrAddresses->Next;
		}
	}
	else
	{
		// either ERROR_NO_DATA or an error;
	}

	if (pAddresses)
	{
		FREE(pAddresses);
	}
}

std::set<NetworkInterface> previousInterfaces;
/**
 * Checks whether there are differences in the connectivity between calls.
 **/
void CheckConnectedInterfaces(CInterfacesCallback *pCallback)
{
	std::set<NetworkInterface> currentInterfaces;
	QueryActiveInterfaces(currentInterfaces);

	for (std::set<NetworkInterface>::iterator it = previousInterfaces.begin();
		it != previousInterfaces.end(); it ++)
	{
		// previous interface no longer connected
		if (currentInterfaces.find(*it) == currentInterfaces.end())
		{
			pCallback->OnInterfaceDisconnected(*it);
		}
	}

	for (std::set<NetworkInterface>::iterator it = currentInterfaces.begin();
		it != currentInterfaces.end(); it ++)
	{
		// current interface is new
		if (previousInterfaces.find(*it) == previousInterfaces.end())
		{
			pCallback->OnInterfaceConnected(*it);
		}
	}

	previousInterfaces = currentInterfaces;
}

bool interfacesMonitorRunning = false;
HANDLE hInterfacesMonitor = NULL;

unsigned long g_pcapSizeLimit;
unsigned long g_interfaceMonitorTimeout;

DWORD WINAPI InterfacesMonitorThread(LPVOID lpParameter)
{
	CInterfacesCallback *pCallback = (CInterfacesCallback *) lpParameter;
	if (!pCallback)
		return 0;

	// First-time call;
	CheckConnectedInterfaces(pCallback);

	while (interfacesMonitorRunning)
	{
		OVERLAPPED overlap;
		overlap.hEvent = WSACreateEvent();

		HANDLE handle = NULL;
		NotifyAddrChange(&handle, &overlap);

		while (interfacesMonitorRunning)
		{
			if (WaitForSingleObject(overlap.hEvent, g_interfaceMonitorTimeout) == WAIT_OBJECT_0)
			{
				CheckConnectedInterfaces(pCallback);
				break;
			}
			else
			{
				// check files sizes;
				for (std::set<NetworkInterface>::iterator it = previousInterfaces.begin(); it != previousInterfaces.end(); it ++)
				{
					const char * szCaptureFile = GetCaptureFile(it->strName.c_str());
					if (szCaptureFile)
					{
						WIN32_FIND_DATAA data;
						HANDLE hFind = FindFirstFileA(szCaptureFile, &data);
						if (hFind != INVALID_HANDLE_VALUE)
						{
							__int64 fileSize = data.nFileSizeLow | (__int64)data.nFileSizeHigh << 32;
							FindClose(hFind);

							if (fileSize >= g_pcapSizeLimit)
							{
								pCallback->OnInterfaceRestarted(*it);
							}
						}
					}
				}
			}
		}
	}

	// TODO: last time call? disconnected
	return 0;
}

PCAPAPI bool StartInterfacesMonitor(CInterfacesCallback &callback, unsigned long pcapSizeLimit, unsigned long interfaceMonitorTimeout)
{
	g_pcapSizeLimit = pcapSizeLimit;
	g_interfaceMonitorTimeout = interfaceMonitorTimeout;

	if (!hInterfacesMonitor)
	{
		interfacesMonitorRunning = true;
		hInterfacesMonitor = CreateThread(NULL, NULL, InterfacesMonitorThread, &callback, NULL, NULL);
		return true;
	}
	else
	{
		return false;
	}
}

PCAPAPI bool StopInterfacesMonitor()
{
	if (hInterfacesMonitor)
	{
		interfacesMonitorRunning = false;
		WaitForSingleObject(hInterfacesMonitor, INFINITE);
		CloseHandle(hInterfacesMonitor);
		hInterfacesMonitor = NULL;

		previousInterfaces.clear();
		return true;
	}
	return false;
}

bool wifiMonitorRunning = false;
HANDLE hWifiMonitor = NULL;

unsigned long g_wifiMonitorTimeout;

DWORD WINAPI WifiMonitorThreadProc(LPVOID lpParameter)
{
	CWifiMonitor *pCallback = (CWifiMonitor *) lpParameter;

	if (!pCallback)
		return 0L;

	while (wifiMonitorRunning)
	{
		HANDLE hClient = NULL;
		DWORD dwMaxClient = 2;
		DWORD dwCurVersion = 0;
		WCHAR szGuid[39] = {0};

		PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
		PWLAN_INTERFACE_INFO pIfInfo = NULL;

		if (WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient) == ERROR_SUCCESS)
		{
			if (WlanEnumInterfaces(hClient, NULL, &pIfList) == ERROR_SUCCESS)
			{
				for (int i = 0; i < (int) pIfList->dwNumberOfItems; i++)
				{
					pIfInfo = (WLAN_INTERFACE_INFO *) &pIfList->InterfaceInfo[i];

					PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
					DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
					WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

					if (WlanQueryInterface(hClient, &pIfInfo->InterfaceGuid, wlan_intf_opcode_current_connection, NULL,
						&connectInfoSize, (PVOID *) &pConnectInfo, &opCode) != ERROR_SUCCESS)
						continue;

					int signal = pConnectInfo->wlanAssociationAttributes.wlanSignalQuality;
					long rSpeed = pConnectInfo->wlanAssociationAttributes.ulRxRate;
					long tSpeed = pConnectInfo->wlanAssociationAttributes.ulTxRate;

					DWORD dwSize = sizeof(ULONG);

					ULONG ulRSSI = 0;
					PULONG pRSSI = 0;
					if (WlanQueryInterface(hClient, &pIfInfo->InterfaceGuid, wlan_intf_opcode_rssi, NULL, &dwSize, (PVOID *) &pRSSI, NULL) == ERROR_SUCCESS)
					{
						ulRSSI = *pRSSI;
					}

					StringFromGUID2(pIfInfo->InterfaceGuid, (LPOLESTR) &szGuid, sizeof(szGuid)/sizeof(*szGuid));
					std::tstring strGuid = szGuid;

					pCallback->OnWifiStats(strGuid, tSpeed, rSpeed, signal, ulRSSI, pIfInfo->isState);
				}
			}

			if (pIfList != NULL)
			{
				WlanFreeMemory(pIfList);
				pIfList = NULL;
			}
			WlanCloseHandle(hClient, NULL);
			hClient = NULL;
		}

		unsigned long dwSlept = 0;
		while (dwSlept < g_wifiMonitorTimeout && wifiMonitorRunning)
		{
			Sleep(100);
			dwSlept += 100;
		}
	}

	return 0L;
}

PCAPAPI bool StartWifiMonitor(CWifiMonitor &callback, unsigned long wifiMonitorTimeout)
{
	g_wifiMonitorTimeout = wifiMonitorTimeout;

	if (!hWifiMonitor)
	{
		wifiMonitorRunning = true;
		hWifiMonitor = CreateThread(NULL, NULL, WifiMonitorThreadProc, &callback, NULL, NULL);
		return true;
	}
	return false;
}

PCAPAPI bool StopWifiMonitor()
{
	if (hWifiMonitor)
	{
		wifiMonitorRunning = false;
		WaitForSingleObject(hWifiMonitor, INFINITE);
		CloseHandle(hWifiMonitor);
		hWifiMonitor = NULL;
		return true;
	}
	return false;
}
