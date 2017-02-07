#include "stdafx.h"
#include "CppUnitTest.h"

#include <array>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "HashedValuesTable.h"


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

			char macSource[6], macDestination[6], newMacSource[6], newMacDestination[6], hexHash[65], hexMac[13];

			BYTE *hash, *testHash;
			unsigned long hashLen;
			std::array<byte, 6> MACBuffer;
			std::copy(std::begin(macSource), std::end(macSource), std::begin(MACBuffer));
			
			
			if ((errNumber=hashedMACs.getHash(MACBuffer, &hash, &hashLen))) {
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
			memcpy(newMacSource, (void*)&(hash[hashLen - 7]), 6);
			for (int j = 0; j < 6; j++)
				sprintf(&hexMac[2 * j], "%02x", macSource[j]);
				//hexMac[j] = 'F';
			hexMac[12] = 0;
			wchar_t str2[256]; wsprintf(str2, L"Original mac is %S\n", hexMac); OutputDebugString(str2);
			for (int j = 0; j < 6; j++)
				sprintf(&hexMac[2 * j], "%02X", newMacSource[j]);
				//hexMac[j] = 'F';
			hexMac[12] = 0;
			wchar_t str3[256]; wsprintf(str3, L"New mac is %S\n", hexMac); OutputDebugString(str3);
			//Output printing section end

			//Test that the new output is still the same, i.e. it got store properly
			if ((errNumber = hashedMACs.getHash(MACBuffer, &testHash, &hashLen))) {
				printf("Something went wrong\n");
				wchar_t str[256]; wsprintf(str, L"Something went wrong! Error code: %x \n", errNumber); OutputDebugString(str);
				abort();
			}

			for (int j = 0; j < hashLen; j++) {
				if (hash[j] != testHash[j]) {
					abort();
				}
			}


			std::copy(std::begin(macDestination), std::end(macDestination), std::begin(MACBuffer));
			hashedMACs.getHash(MACBuffer, &hash, &hashLen);
			memcpy(newMacDestination, (void*)&hash[hashLen - 7], 6);
			int sameBytes = 0;
			for (int j = 0; j < hashLen; j++) {
				if (hash[j] == testHash[j]) {
					sameBytes++;
				}
			}
		}

		TEST_METHOD(TestHashedIpAddresses)
		{
			HashedValuesTable<unsigned int> hashedIPv4s;
			DWORD errNumber;

			char hexHash[65], hexIp[9];
			unsigned int ipSource, ipDestination, newIpSource, newIpDestination;

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
			for (int j = 0; j < 4; j++)
				sprintf(hexIp + 2 * j, "%02X", (&ipSource)+j);
			hexIp[8] = 0;
			wchar_t str2[256]; wsprintf(str2, L"Original ip is %S\n", hexIp); OutputDebugString(str2);
			for (int j = 0; j < 6; j++)
				sprintf(hexIp + 2 * j, "%02X", (&newIpSource) + j);
			hexIp[8] = 0;
			wchar_t str3[256]; wsprintf(str3, L"New ip is %S\n", hexIp); OutputDebugString(str3);
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
		}

		TEST_METHOD(TestHashedStrings)
		{
			HashedValuesTable<std::string> hashedStrings;
			DWORD errNumber;

			char hexHash[65], hexIp[9];
			std::string toHash("Something that shoult be hashed");
			std::string toHashForCheck("Something else shoult be hashed");
			
			BYTE *hash, *testHash;
			unsigned long hashLen;


			if ((errNumber = hashedStrings.getHashString(toHash, &hash, &hashLen))) {
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
}