#include "stdafx.h"
#include "CppUnitTest.h"

#include <array>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <time.h>       /* time */

#include "HashedValuesTable.h"
//TODO move this to a new file
//#include "Upload.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace HostViewUniteTests
{		
	TEST_CLASS(HashTests)
	{
	public:
		
		TEST_METHOD(TestHashedMacAddresses)
		{
			HashedValuesTable< std::array<byte, 6> > hashedMACs;
			DWORD errNumber;

			char macSource[6], macDestination[6], newMacSource[6], newMacDestination[6], hexHash[256], hexMac[128];

			macSource[0] = 0x01;
			macSource[1] = 0xAB;
			macSource[2] = 0x12;
			macSource[3] = 0xE6;
			macSource[4] = 0x0F;
			macSource[5] = 0x66;

			macDestination[0] = 0x03;
			macDestination[1] = 0xFB;
			macDestination[2] = 0x22;
			macDestination[3] = 0xEE;
			macDestination[4] = 0x00;
			macDestination[5] = 0x88;

			BYTE *hash, *testHash;
			unsigned long hashLen;
			std::array<byte, 6> MACBuffer;
			MACBuffer = std::array<byte, 6>();
			std::copy(std::begin(macSource), std::end(macSource), std::begin(MACBuffer));
			wchar_t str[256]; for (int i = 0; i < 256; i++)str[i] = 0;
			
			if ((errNumber=hashedMACs.getHashBytes(MACBuffer, &hash, &hashLen))) {
				wsprintf(str, L"Something went wrong! Error code: %x \n", errNumber); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
				abort();
			}
			else if (hash == NULL || hashLen == 0) {
				wsprintf(str, L"Something went wrong! With no error code \n"); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
				abort();
			}

			//Output printing section start
			
			for (int j = 0; j < hashLen; j++)
				sprintf(&hexHash[2 * j], "%02X", hash[j]);
			hexHash[64] = 0;
			wsprintf(str, L"All good. New hash of size %lu\n", hashLen); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
			wsprintf(str, L"Hash is %S\n", hexHash); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
			memcpy(newMacSource, (void*)&(hash[hashLen - 7]), 6);
			for (int j = 0; j < 6; j++)
				sprintf(&hexMac[2 * j], "%02x", macSource[j]);
				//hexMac[j] = 'F';
			hexMac[12] = 0;
			wsprintf(str, L"Original mac is %S\n", hexMac); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
			for (int j = 0; j < 6; j++)
				sprintf(&hexMac[2 * j], "%02X", newMacSource[j]);
				//hexMac[j] = 'F';
			hexMac[12] = 0;
			wsprintf(str, L"New mac is %S\n", hexMac); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
			
			//Output printing section end

			//Test that the new output is still the same, i.e. it got store properly
			if ((errNumber = hashedMACs.getHashBytes(MACBuffer, &testHash, &hashLen))) {
				printf("Something went wrong\n");
				wsprintf(str, L"Something went wrong! Error code: %x \n", errNumber); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
				abort();
			}
			else if (hash == NULL || hashLen == 0) {
				wsprintf(str, L"Something went wrong! With no error code \n"); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
				abort();
			}

			for (int j = 0; j < hashLen; j++) {
				if (hash[j] != testHash[j]) {
					abort();
				}
			}


			std::copy(std::begin(macDestination), std::end(macDestination), std::begin(MACBuffer));
			MACBuffer = std::array<byte, 6>();
			hashedMACs.getHashBytes(MACBuffer, &hash, &hashLen);
			memcpy(newMacDestination, (void*)&hash[hashLen - 7], 6);
			int sameBytes = 0;
			for (int j = 0; j < hashLen; j++) {
				if (hash[j] == testHash[j]) {
					sameBytes++;
				}
			}

			wsprintf(str, L"****\nStarting batch test\n****\n\0"); OutputDebugString(str);

			//This loop generates a lot of random mac addresses
			// In each loop it extracts one hash it knows alread (i.e. the local mac address) and one random)
			srand(time(NULL));
			for (int i = 0; i < 100000; i++) {
				//hash src mac
				for (int j = 0; j < 6; j++)
					MACBuffer[j] = macSource[j];
				if ((errNumber = hashedMACs.getHashBytes(MACBuffer, &hash, &hashLen))) {
					wsprintf(str, L"Something went wrong! Error code: %x \n", errNumber); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
					abort();
				}
				else if (hash == NULL || hashLen == 0) {
					wsprintf(str, L"Something went wrong! With no error code \n"); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
					abort();
				}

				wsprintf(str, L"Loop %d. Working with source.\nNew hash of size %lu\n", i, hashLen); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;

				
				for (int j = 0; j < hashLen; j++)
					sprintf(&hexHash[2 * j], "%02X", hash[j]);
				hexHash[64] = 0;
				wsprintf(str, L"Loop %d. Working with source.\nNew hash of size %lu\n",i,hashLen); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
				memcpy(newMacSource, (void*)&(hash[hashLen - 7]), 6);
				for (int j = 0; j < 6; j++)
					sprintf(&hexMac[2 * j], "%02x", macSource[j]);
				hexMac[12] = 0;
				wsprintf(str, L"Original mac is %S\n", hexMac); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
				for (int j = 0; j < 6; j++)
					sprintf(&hexMac[2 * j], "%02X", newMacSource[j]);
				hexMac[12] = 0;
				wsprintf(str, L"New mac is %S\n", hexMac); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
				
				//randomize mac
				for (int j = 0; j < 6; j++)
					macDestination[j] = rand() % 256;
				//std::copy(std::begin(macDestination), std::end(macDestination), std::begin(MACBuffer));
				MACBuffer = std::array<byte, 6>();
				for (int j = 0; j < 6; j++)
					MACBuffer[j] = macDestination[j];
				//hash
				if ((errNumber = hashedMACs.getHashBytes(MACBuffer, &hash, &hashLen))) {
					wsprintf(str, L"Something went wrong! Error code: %x \n", errNumber); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
					abort();
				}
				else if (hash == NULL || hashLen == 0) {
					wsprintf(str, L"Something went wrong! With no error code \n"); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
					abort();
				}
				
				for (int j = 0; j < hashLen; j++)
					sprintf(&hexHash[2 * j], "%02X", hash[j]);
				hexHash[64] = 0;
				wsprintf(str, L"Working with the destination.\nNew hash of size %lu\n", hashLen); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
				memcpy(newMacDestination, (void*)&(hash[hashLen - 7]), 6);
				for (int j = 0; j < 6; j++)
					sprintf(&hexMac[2 * j], "%02x", macDestination[j]);
				hexMac[12] = 0;
				wsprintf(str, L"Original mac is %S\n", hexMac); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
				for (int j = 0; j < 6; j++)
					sprintf(&hexMac[2 * j], "%02X", newMacDestination[j]);
				hexMac[12] = 0;
				wsprintf(str, L"New mac is %S\n", hexMac); OutputDebugString(str); for (int i = 0; i < 256; i++)str[i] = 0;
				
			}
		}

		TEST_METHOD(TestHashedIpAddresses)
		{
			HashedValuesTable<unsigned int> hashedIPv4s;
			DWORD errNumber;

			char hexHash[65];
			unsigned int ipSource = 5435734, ipDestination = 1122356, newIpSource, newIpDestination;

			BYTE *hash, *testHash;
			unsigned long hashLen;


			if ((errNumber = hashedIPv4s.getHash(ipSource, &hash, &hashLen))) {
				printf("Something went wrong\n");
				wchar_t str[256]; wsprintf(str, L"Something went wrong! Error code: %x \n", errNumber); OutputDebugString(str);
				abort();
			}

			//Output printing section start
			for (int j = 0; j < hashLen; j++)
				sprintf(&hexHash[2 * j], "%02X", hash[j]);
			hexHash[64] = 0;
			wchar_t str4[256]; wsprintf(str4, L"All good. New hash of size %lu\n", hashLen); OutputDebugString(str4);
			wchar_t str5[256]; wsprintf(str5, L"Hash is %S\n", hexHash); OutputDebugString(str5);
			memcpy(&newIpSource, (void*)&(hash[hashLen - 5]), 4);
			wchar_t str2[256]; wsprintf(str2, L"Original ip is %d\n", ipSource); OutputDebugString(str2);
			wchar_t str3[256]; wsprintf(str3, L"New ip is %d\n", newIpSource); OutputDebugString(str3);
			//Output printing section end

			//Test that the new output is still the same, i.e. it got store properly
			if ((errNumber = hashedIPv4s.getHash(ipSource, &testHash, &hashLen))) {
				wchar_t str[256]; wsprintf(str, L"Something went wrong! Error code: %x \n", errNumber); OutputDebugString(str);
				abort();
			}

			for (int j = 0; j < hashLen; j++) {
				if (hash[j] != testHash[j]) {
					abort();
				}
			}


			hashedIPv4s.getHash(ipDestination, &hash, &hashLen);
			memcpy(&newIpDestination, (void*)&hash[hashLen - 5], 4);
			int sameBytes = 0;
			for (int j = 0; j < hashLen; j++) {
				if (hash[j] == testHash[j]) {
					sameBytes++;
				}
			}
			if (sameBytes == hashLen) abort();

			wsprintf(str2, L"****\nStarting batch test\n****\n\0"); OutputDebugString(str2);

			//This loop generates a lot of random mac addresses
			// In each loop it extracts one hash it knows alread (i.e. the local mac address) and one random)
			srand(time(NULL));
			//define default source mac address 0x01AB12E60F66
			ipSource = 5483578934;
			//std::copy(std::begin(macSource), std::end(macSource), std::begin(MACBuffer));
			for (int i = 0; i < 100000; i++) {
				//hash src mac
				if ((errNumber = hashedIPv4s.getHash(ipSource, &hash, &hashLen))) {
					wchar_t str[256]; wsprintf(str, L"Something went wrong! Error code: %x \n", errNumber); OutputDebugString(str);
					abort();
				}
				else if (hash == NULL || hashLen == 0) {
					wchar_t str[256]; wsprintf(str, L"Something went wrong! With no error code \n"); OutputDebugString(str);
					abort();
				}
				wsprintf(str4, L"Loop %d. New hash of size %lu\n", i, hashLen); OutputDebugString(str4);
				memcpy(&newIpSource, (void*)&(hash[hashLen - 5]), 4);
				wsprintf(str2, L"Original ip is %u\n", ipSource); OutputDebugString(str2);
				wsprintf(str3, L"New ip is %u\n", newIpSource); OutputDebugString(str3);
				//Output printing section end

				ipDestination = rand();
				hashedIPv4s.getHash(ipDestination, &hash, &hashLen);
				wsprintf(str4, L"Destination. New hash of size %lu\n", hashLen); OutputDebugString(str4);
				memcpy(&newIpDestination, (void*)&hash[hashLen - 5], 4);
				wsprintf(str2, L"Original ip is %u\n", ipDestination); OutputDebugString(str2);
				wsprintf(str3, L"New ip is %u\n", newIpDestination); OutputDebugString(str3);
			}
		}

		TEST_METHOD(TestHashedStrings)
		{
			HashedValuesTable<std::string> hashedStrings;
			DWORD errNumber;

			char hexHash[256];
			std::string toHash("Something that shoult be hashed");
			std::string toHashForCheck("Something else shoult be hashed");
			
			BYTE *hash, *testHash;
			unsigned long hashLen;

			wchar_t str[256];


			if ((errNumber = hashedStrings.getHashString(toHash, &hash, &hashLen))) {
				wsprintf(str, L"Something went wrong! Error code: %x \n", errNumber); OutputDebugString(str);
				abort();
			}

			//Output printing section start
			for (int j = 0; j < hashLen; j++)
				sprintf(&hexHash[2 * j], "%02X", hash[j]);
			hexHash[64] = 0;
			wchar_t str4[256]; wsprintf(str4, L"All good. New hash of size %lu\n", hashLen); OutputDebugString(str4);
			wchar_t str5[256]; wsprintf(str5, L"Hash is %S\n", hexHash); OutputDebugString(str5);
			//Output printing section end

			//Test that the new output is still the same, i.e. it got store properly
			if ((errNumber = hashedStrings.getHashString(toHash, &testHash, &hashLen))) {
				wchar_t str[256]; wsprintf(str, L"Something went wrong! Error code: %x \n", errNumber); OutputDebugString(str);
				abort();
			}

			for (int j = 0; j < hashLen; j++) {
				if (hash[j] != testHash[j]) {
					abort();
				}
			}

			hashedStrings.getHashString(toHashForCheck, &hash, &hashLen);
			for (int j = 0; j < hashLen; j++)
				sprintf(&hexHash[2 * j], "%02X", hash[j]);
			hexHash[64] = 0;
			wsprintf(str5, L"Hash is %S\n", hexHash); OutputDebugString(str5);
			int sameBytes = 0;
			for (int j = 0; j < hashLen; j++) {
				if (hash[j] == testHash[j]) {
					sameBytes++;
				}
			}
			if (sameBytes == hashLen) abort();
		}

	};

	//TEST_CLASS(HTTPTESTS)
	//{
	//public:

	//	TEST_METHOD(TestCurlUpload)
	//	{
	//		wchar_t str[256];
	//		CUpload *m_upload = new CUpload();
	//		if (!m_upload->TrySubmit("Ciao")) {
				// no need to retry - either this was a success or then max retries was reached
	//			wsprintf(str, L"Succesfully uploaded all files\n"); OutputDebugString(str);
	//			delete m_upload;
	//			m_upload = NULL;
	//		} else{
	//			abort();
	//		}
	//	}

	//};
}