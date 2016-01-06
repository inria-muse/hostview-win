/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 Muse / INRIA
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
#include "Capture.h"

const char * inet_ntop2(int af, const void *src, char *dst, int cnt)
{
	struct sockaddr_in srcaddr;
	memset(&srcaddr, 0, sizeof(srcaddr));
	memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));
	srcaddr.sin_family = af;

	struct sockaddr_in6 srcaddr6;
	memset(&srcaddr6, 0, sizeof(srcaddr6));
	memcpy(&(srcaddr6.sin6_addr), src, sizeof(srcaddr6.sin6_addr));
	srcaddr6.sin6_family = af;

	struct sockaddr * pAddr = (struct sockaddr *) &srcaddr;
	DWORD dwSize = sizeof(srcaddr);

	if (af == AF_INET6)
	{
		pAddr = (struct sockaddr *) &srcaddr6;
		dwSize = sizeof(srcaddr6);
	}

	if (WSAAddressToStringA(pAddr, dwSize, 0, dst, (LPDWORD) &cnt) != 0)
	{
		DWORD dwErr = WSAGetLastError();
		return NULL;
	}
	return dst;
}

pcap_t* GetLiveSource(const char *adapterId = "interactive")
{
	bool interactive = strcmp(adapterId, "interactive") == 0;

	pcap_t *adhandle = NULL;
	pcap_if_t *alldevs = NULL, *d = NULL;
	int i = 0, inum = 0;
	char errbuf[PCAP_ERRBUF_SIZE] = {0};

	/* Retrieve the device list on the local machine */
	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
		return NULL;
	}

	/* Print the list */
	for(d = alldevs; d; d = d->next)
	{
		i ++;
		interactive && printf("%d. %s", i, d->name);
		
		if (strstr(d->name, adapterId))
		{
			inum = i;
		}

		if (d->description)
		{
			interactive && printf(" (%s)\n", d->description);
		}
		else
		{
			interactive && printf(" (No description available)\n");
		}
	}
	
	if(i == 0)
	{
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
		return NULL;
	}

	interactive && printf("Enter the interface number (1-%d):", i);

	if (interactive)
	{
		char buffer[MAX_PATH] = {0};
		fgets(buffer, _countof(buffer), stdin);
		inum = atoi(buffer);
	}

	if(inum < 1 || inum > i)
	{
		interactive && printf("\nInterface number out of range.\n");

		/* Free the device list */
		pcap_freealldevs(alldevs);
		return NULL;
	}
	
	/* Jump to the selected adapter */
	for(d = alldevs, i = 0; i < inum - 1 ; d = d->next, i ++);

	/* Open the adapter */
	if ((adhandle = pcap_open_live(d->name,	// name of the device
							 65536,			// portion of the packet to capture. 
											// 65536 grants that the whole packet will be captured on all the MACs.
							 1,				// promiscuous mode (nonzero means promiscuous)
							 1000,			// read timeout
							 errbuf			// error buffer
							 )) == NULL)
	{
		fprintf(stderr,"\nUnable to open the adapter. %s is not supported by WinPcap\n", d->name);
		pcap_freealldevs(alldevs);
		return NULL;
	}

	pcap_freealldevs(alldevs);

	return adhandle;
}

unsigned char* ReadDnsName(unsigned char* reader, unsigned char* buffer, int* count)
{
	unsigned char *name;
	unsigned int p=0,jumped=0,offset;
	int i , j;
 
	*count = 1;
	name = (unsigned char*)malloc(256);
 
	name[0]='\0';
 
	// read the names in 3www6google3com format
	while(*reader!=0)
	{
		if(*reader>=192)
		{
			offset = (*reader)*256 + *(reader+1) - 49152; // 49152 = 11000000 00000000 ;)
			reader = buffer + offset - 1;
			jumped = 1; // we have jumped to another location so counting wont go up!
		}
		else
		{
			name[p++]=*reader;
		}
 
		reader=reader+1;
 
		if(jumped==0) *count = *count + 1; // if we havent jumped to another location then we can count up
	}
 
	name[p]='\0'; // string complete
	if(jumped==1) 
	{
		*count = *count + 1; // number of steps we actually moved forward in the packet
	}
 
	// now convert 3www6google3com0 to www.google.com
	for(i=0;i<(int)strlen((const char*)name);i++)
	{
		p=name[i];
		for(j=0;j<(int)p;j++)
		{
			name[i]=name[i+1];
			i=i+1;
		}
		name[i]='.';
	}

	name[i-1]='\0'; //remove the last dot
	return name;
}

// DNS parsing (should be a plug in)
// return whether or not to include this in the payload
bool parseDNS(int nProtocol, char *szSrc, char *szDest, u_short sp, u_short dp, u_char * data, CCaptureCallback &callback)
{
	if (sp == 53 || dp == 53)
	{
		struct DNS_HDR *dns = (struct DNS_HDR*) data;
		struct QUESTION *qinfo = NULL;

		char qname[260] = {0};
		int qlen = 0;
		unsigned char*p = &data[sizeof(DNS_HDR)];

		while (*p != 0)
		{
			int length = *p;
			p ++;

			qname[qlen ++] = '0' + length;
			while (length > 0)
			{
				qname[qlen ++] = *p ++;
				length --;

				if (qlen >= _countof(qname))
				{
					// malformed DNS packet;
					return true;
				}
			}
		}
		qname[qlen] = 0;

		unsigned char * reader = &data[sizeof(struct DNS_HDR) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];
 
		//reading answers
		int stop=0;
 
		int answers = ntohs(dns->ans_count);

		for (int i = 0; i < answers; i ++)
		{
			unsigned char *name = ReadDnsName(reader, data, &stop);
			reader = reader + stop;

			R_DATA * resource = (struct R_DATA*) reader;
			reader = reader + sizeof(struct R_DATA);

			int rtype = ntohs(resource->type);
			if (rtype == 1)
			{
				// ipv4
				char ipFinal[MAX_PATH] = {0};
				inet_ntop2(AF_INET, reader, ipFinal, _countof(ipFinal));
				reader += 4;

				callback.OnDNSAnswer(nProtocol, szSrc, sp, szDest, dp, rtype, ipFinal, (char *) name);
			}
			else if (rtype == 28)
			{
				// ipv6
				char ipFinal[MAX_PATH] = {0};
				inet_ntop2(AF_INET6, reader, ipFinal, _countof(ipFinal));
				reader += 16;

				callback.OnDNSAnswer(nProtocol, szSrc, sp, szDest, dp, rtype, ipFinal, (char *) name);
			}
			else if (rtype == 15)
			{
				// MX, skip preference
				reader += 2;
				unsigned char *p = ReadDnsName(reader, data, &stop);
				reader += stop;

				callback.OnDNSAnswer(nProtocol, szSrc, sp, szDest, dp, rtype, (char *) p, (char *) name);
			}
			else if (rtype == 12 || rtype == 5 || rtype == 2)
			{
				// PTR CNAME NS
				unsigned char *p = ReadDnsName(reader, data, &stop);
				reader += stop;

				callback.OnDNSAnswer(nProtocol, szSrc, sp, szDest, dp, rtype, (char *) p, (char *) name);
			}
			else
			{
				// printf("type %d unknown. will break;\r\n", rtype);
				break;
			}
		}

		return true;
	}
	return false;
}

extern bool parseHTTP(int nProtocol, char *szSrc, char *szDest, u_short sp, u_short dp, u_char *data, int packetSize, CCaptureCallback &callback);

std::map<std::string, HANDLE> threads;
std::map<std::string, pcap_t *> sources;
std::map<std::string, pcap_dumper_t*> files;
std::map<std::string, CCaptureCallback *> callbacks;
std::map<std::string, std::string> fnames;
std::set<std::string> adapterIds;

void OnPacketCallback(u_char *p, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	ETHER_HDR *pEthernet = (ETHER_HDR *) pkt_data;
	int nType = ntohs(pEthernet->type);

	if (nType != 0x0800 && nType != 0x86DD)
	{
		// not worth processing
		return;
	}
	CCaptureCallback &callback = *callbacks[(char *) p];

	callback.OnProcessPacketStart();

	char szSrc[MAX_PATH] = {0}, szDest[MAX_PATH] = {0};
	int nProtocol = 0;
	int nIPLength = 0;
	int nCapLength = sizeof(ETHER_HDR);

	if (nType == 0x0800)
	{
		// ipv4
		IPV4_HDR * pIPv4 = (IPV4_HDR *) (pkt_data + sizeof(ETHER_HDR));

		if (pIPv4->ip_version == 4)
		{
			inet_ntop2(AF_INET, &pIPv4->ip_srcaddr, szSrc, _countof(szSrc));
			inet_ntop2(AF_INET, &pIPv4->ip_destaddr, szDest, _countof(szDest));

			nProtocol = pIPv4->ip_protocol;
			nIPLength = pIPv4->ip_header_len * 4;
		}
	}
	else if (nType == 0x86DD)
	{
		// ipv6
		IPV6_HDR *pIPv6 = (IPV6_HDR *) (pkt_data + sizeof(ETHER_HDR));

		inet_ntop2(AF_INET6, &pIPv6->ipv6_srcaddr, szSrc, _countof(szSrc));
		inet_ntop2(AF_INET6, &pIPv6->ipv6_destaddr, szDest, _countof(szDest));

		nProtocol = pIPv6->ipv6_nexthdr;
		nIPLength = sizeof(IPV6_HDR);

		if (nProtocol == IPPROTO_FRAGMENT)
		{
			IPV6_FRAGMENT_HDR * pFragment = (IPV6_FRAGMENT_HDR *) pIPv6 + sizeof(IPV6_HDR);

			nProtocol = pFragment->ipv6_frag_nexthdr;
			nIPLength += sizeof(IPV6_FRAGMENT_HDR);
		}
	}

	nCapLength += nIPLength;

	switch (nProtocol)
	{
	case IPPROTO_TCP:
		{
			TCP_HDR * pTCP = (TCP_HDR *) (pkt_data + sizeof(ETHER_HDR) + nIPLength);

			u_short sp = ntohs(pTCP->source_port);
			u_short dp = ntohs(pTCP->dest_port);

			nCapLength += max(pTCP->data_offset * 4, sizeof(TCP_HDR));
			callback.OnTCPPacket(szSrc, sp, szDest, dp, header->caplen);

			// HTTP parsing
			parseHTTP(nProtocol, szSrc, szDest, sp, dp, ((u_char*)pkt_data + nCapLength), header->caplen - nCapLength, callback);

			// DNS parsing
			if (parseDNS(nProtocol, szSrc, szDest, sp, dp, ((u_char*)pkt_data + nCapLength), callback))
			{
				// include whole dns
				nCapLength = header->caplen;
			}
		}
		break;
	case IPPROTO_UDP:
		{
			UDP_HDR *pUDP = (UDP_HDR *) (pkt_data + sizeof(ETHER_HDR) + nIPLength);
			u_short sp = ntohs(pUDP->src_portno);
			u_short dp = ntohs(pUDP->dst_portno);

			nCapLength += sizeof(UDP_HDR);
			callback.OnUDPPacket(szSrc, sp, szDest, dp, header->caplen);

			// DNS parsing
			if (parseDNS(nProtocol, szSrc, szDest, sp, dp, ((u_char*)pkt_data + nCapLength), callback))
			{
				// include whole dns
				nCapLength = header->caplen;
			}
		}
		break;
	case IPPROTO_IGMP:
		{
			nCapLength = header->caplen;
			callback.OnIGMPPacket(szSrc, szDest, header->caplen);
		}
		break;
	case IPPROTO_ICMP:
		{
			nCapLength = header->caplen;
			callback.OnICMPPacket(szSrc, szDest, header->caplen);
		}
		break;
	case IPPROTO_ICMPV6:
		{
			nCapLength = header->caplen;
			callback.OnICMPPacket(szSrc, szDest, header->caplen);
		}
		break;
	}

	((struct pcap_pkthdr *)header)->caplen = nCapLength;

	pcap_dump((u_char *) files[(char *) p], header, pkt_data);

	callback.OnProcessPacketEnd();
}

DWORD WINAPI CaptureThreadProc(LPVOID lpParameter)
{
	pcap_t *source = sources[(char *) lpParameter];
	return pcap_loop(source, 0, OnPacketCallback, (unsigned char *) lpParameter);
}

PCAPAPI bool StartCapture(CCaptureCallback &callback, __int64 timestamp, const char *adapterId)
{
	HANDLE hCaptureThread = threads[adapterId];
	if (!hCaptureThread)
	{
		pcap_t *source = GetLiveSource(adapterId);
		if (source)
		{
			sources[adapterId] = source;

			// ensures data directory
			CreateDirectory(_T("data"), NULL);

			char szFilename[MAX_PATH] = {0};
			sprintf_s(szFilename, "data/%llu_%s.pcap", timestamp, adapterId);
			fnames[adapterId] = szFilename;

			pcap_dumper_t *dumpfile = pcap_dump_open(source, szFilename);
			if (dumpfile)
			{
				files[adapterId] = dumpfile;
				callbacks[adapterId] = &callback;

				// make sure he'll live
				adapterIds.insert(adapterId);
				hCaptureThread = CreateThread(NULL, NULL, CaptureThreadProc, (LPVOID) adapterIds.find(adapterId)->c_str(), NULL, NULL);

				threads[adapterId] = hCaptureThread;
				return true;
			}
		}
	}
	return false;
}

PCAPAPI bool StopCapture(const char *adapterId)
{
	pcap_t *source = sources[adapterId];
	HANDLE hCaptureThread = threads[adapterId];
	pcap_dumper_t * dumpfile = files[adapterId];

	if (source != NULL)
	{
		pcap_breakloop(source);
		if (hCaptureThread != NULL)
		{
			WaitForSingleObject(hCaptureThread, INFINITE);
			CloseHandle(hCaptureThread);
			hCaptureThread = NULL;
			threads.erase(adapterId);
		}
		pcap_close(source);
		source = NULL;
		sources.erase(adapterId);

		if (dumpfile)
		{
			pcap_dump_close(dumpfile);
			dumpfile = NULL;
			files.erase(adapterId);
			callbacks.erase(adapterId);

			// ensures submit directory
			CreateDirectory(_T("submit"), NULL);

			std::string file = fnames[adapterId];

			if (file.find("data") != std::string::npos)
			{
				file.replace(file.find("data"), 4, "submit");
				MoveFileA(fnames[adapterId].c_str(), file.c_str());
			}
			else
			{
				// what TODO: ? - remove file?
			}

			fnames.erase(adapterId);
			return true;
		}
	}
	return false;
}

PCAPAPI bool StopAllCaptures()
{
	for (std::set<std::string>::iterator it = adapterIds.begin(); it != adapterIds.end(); it ++)
	{
		StopCapture(it->c_str());
	}

	adapterIds.clear();

	return true;
}

PCAPAPI bool IsCaptureRunning(const char *adapterId)
{
	pcap_t *source = sources[adapterId];
	HANDLE hCaptureThread = threads[adapterId];
	pcap_dumper_t * dumpfile = files[adapterId];

	return hCaptureThread != NULL
		&& source != NULL
		&& dumpfile != NULL;
}

PCAPAPI const char* GetCaptureFile(const char *source)
{
	return fnames[source].c_str();
}

