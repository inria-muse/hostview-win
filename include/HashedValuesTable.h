#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <wincrypt.h>

template <class T>
class HashedValuesTable
{
public:
	HashedValuesTable() {
		_salt = NULL;
		_saltLen = 0;
		_hashSize = 0;
	}

	~HashedValuesTable() {

	}

	int addSalt(const unsigned char *salt, const int saltLen) {
		_saltLen = saltLen;
		_salt = (BYTE *)malloc(saltLen);
		if (_salt == NULL) return -1;
		memcpy(_salt, salt, saltLen);
	}

	//It empties the map of hashed values
	void clear() {
		//TODO iterate over elements to free memory
		_hashedValues.clear();
		if(_salt)free(_salt);
		_saltLen = 0;
		_hashSize = 0;
	}

	bool hasSalt() {
		return _salt != 0;
	}

	//APIs to add elements
	int getHash(const T &value, BYTE **output, unsigned long *outLen) {
		BYTE *hash;
		long len;
		int err = 0;
		auto storedValue = _hashedValues.find(value);
		if (storedValue == _hashedValues.end()) {
			std::array< BYTE, sizeof(T) > data = to_bytes(value);
			//Create data
			if (hasSalt()) {
				if (!(err = hashValueWithSalt(data.data(), sizeof(T), &hash))) {
					*output = hash;
					*outLen = _hashSize;
					return 0;
				}
				else {
					return err;
				}
					
			}
			else {
				if (!(err = hashValue(data.data(), sizeof(T), &hash))) {
					*output = hash;
					*outLen = _hashSize;
					return 0;
				}
				else {
					return err;
				}
			}
			_hashedValues.insert(std::make_pair(value, hash));
		}
		else {
			*output = storedValue->second;
			*outLen = _hashSize; 
			return 0;
		}

	}

	/*unsigned char* getStringHash(const std::string &value, unsigned char* = NULL) {

	}*/

	template<std::size_t SIZE>
	int getHash(const std::array<byte, SIZE> &value, BYTE **output, unsigned long *outLen) {
		BYTE *hash, *data;
		long len;
		int err = 0;
		auto storedValue = _hashedValues.find(value);
		if (storedValue == _hashedValues.end()) {
			data = (BYTE*)value.data();
			//Create data
			if (hasSalt()) {
				if (!(err = hashValueWithSalt(data, SIZE, &hash))) {
					*output = hash;
					*outLen = _hashSize;
					return 0;
				}
				else {
					return err;
				}
					
			}
			else {
				if (!(err = hashValueWithSalt(data, SIZE, &hash))) {
					*output = hash;
					*outLen = _hashSize;
					return 0;
				}
				else {
					return err;
				}
			}
			_hashedValues.insert(std:make_pair(value, hash));
		}
		else {
			*output = storedValue->second;
			*outLen = _hashSize; 
			return 0;
		}
	}

	int getHash(const std::string &value, BYTE **output, unsigned long *outLen) {
		BYTE *hash, *data;
		long len;
		int err = 0;
		auto storedValue = _hashedValues.find(value);
		if (storedValue == _hashedValues.end()) {
			data = (BYTE*)value.c_str();
			//Create data
			if (hasSalt()) {
				if (!(err = hashValueWithSalt(data, &hash))) {
					*output = hash;
					*outLen = _hashSize;
					return 0;
				}
				else {
					return err;
				}

			}
			else {
				if (!(err = hashValueWithSalt(data, &hash))) {
					*output = hash;
					*outLen = _hashSize;
					return 0;
				}
				else {
					return err;
				}
			}
			_hashedValues.insert(std:make_pair(value, hash));
		}
		else {
			*output = storedValue->second;
			*outLen = _hashSize;
			return 0;
		}
	}

	int get32Hash(const T &value, unsigned int* output) {
		int err = 0;
		unsigned long len;
		BYTE *hash = NULL; 
		if (!(err = getHash(value, &hash, &len))) {
			//Copies the last 4 bytes of the hash to the int output
			memcpy((void *)output, &(hash[_hashSize - 5]), 4);
		}
		else {
			return err;
		}
	}

	int get64Hash(const T &value, unsigned long* output) {
		int err = 0;
		unsigned long len;
		BYTE *hash = NULL;
		if (!(err = getHash(value, &hash, &len))) {
			//Copies the last 4 bytes of the hash to the int output
			memcpy((void *)output, &(hash[_hashSize - 9], 8))
		}
		else {
			return err;
		}
	}

private:
	//TODO reset to hash map when find out how to hash arrays
	//std::hash_map <T, unsigned char *> _hashedValues;
	std::map <T, unsigned char *> _hashedValues;
	int _saltLen;
	unsigned long _hashSize;
	BYTE *_salt;

	int hashValueWithSalt(BYTE *data, unsigned int dataSize, BYTE ** hash) {
		HCRYPTPROV hProv = 0;
		HCRYPTHASH hHash = 0;
		BYTE *pbHash = NULL;
		DWORD dwHashLen;

		//BYTE * pbBuffer = NULL;
		DWORD dwCount;
		DWORD i;
		unsigned long bufLen = 0;

		//Create context and hash object
		if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0)) {
			return GetLastError();
		}
		if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
			return GetLastError();
		}

		//Create empty buffer for hash
		//bufLen = strlen(data);
		//pbBuffer = (BYTE*)malloc(bufLen + 1);
		//memset(pbBuffer, 0, bufLen + 1);



		//Copy data to has into a new buffer (why?)
		/*for (i = 0; i < bufLen; i++) {
			pbBuffer[i] = (BYTE)data[i];
		}*/
		
		//Pass salt to hash
		if (!CryptHashData(hHash, _salt, _saltLen, 0)) {
			return GetLastError();
		}

		//Pass data to hash
		if (!CryptHashData(hHash, data, dataSize, 0)) {
			return GetLastError();
		}

		//Get the size of the hashed data and create proper buffer to contain it
		if (!_hashSize) {
			dwCount = sizeof(DWORD);
			if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&dwHashLen, &dwCount, 0)) {
				return GetLastError();
			}
			_hashSize = dwHashLen;
		}
		else {
			dwHashLen = _hashSize;
		}

		//create buffer where to place the hashed data
		if ((pbHash = (unsigned char*)malloc(dwHashLen)) == NULL) {
			return GetLastError();
		}
		memset(pbHash, 0, dwHashLen);

		//Get the hashed data
		if (!CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &dwHashLen, 0)) {
			return GetLastError();
		}

		*hash = pbHash;

		if (hHash) CryptDestroyHash(hHash);
		if (hProv) CryptReleaseContext(hProv, 0);

		return 0;
	}

	int hashValue(BYTE *data, unsigned int dataSize, BYTE ** hash) {
		HCRYPTPROV hProv = 0;
		HCRYPTHASH hHash = 0;
		BYTE *pbHash = NULL;
		DWORD dwHashLen;

		//BYTE * pbBuffer = NULL;
		DWORD dwCount;
		DWORD i;
		unsigned long bufLen = 0;

		//Create context and hash object
		if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
			return GetLastError();
		}
		if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
			return GetLastError();
		}

		//Create empty buffer for hash
		//bufLen = strlen(data);
		//pbBuffer = (BYTE*)malloc(bufLen + 1);
		//memset(pbBuffer, 0, bufLen + 1);



		//Copy data to has into a new buffer (why?)
		/*for (i = 0; i < bufLen; i++) {
			pbBuffer[i] = (BYTE)data[i];
		}*/

		//Pass data to hash
		if (!CryptHashData(hHash, data, dataSize, 0)) {
			return GetLastError();
		}

		//Get the size of the hashed data and create proper buffer to contain it
		if (!_hashSize) {
			dwCount = sizeof(DWORD);
			if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&dwHashLen, &dwCount, 0)) {
				return GetLastError();
			}
			_hashSize = dwHashLen;
		}
		else {
			dwHashLen = _hashSize;
		}

		//Create the buffer where to place the hashed data
		if ((pbHash = (unsigned char*)malloc(dwHashLen)) == NULL) {
			return GetLastError();
		}
		memset(pbHash, 0, dwHashLen);

		//Get the hashed data
		if (!CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &dwHashLen, 0)) {
			return GetLastError();
		}

		*hash = pbHash;

		if (hHash) CryptDestroyHash(hHash);
		if (hProv) CryptReleaseContext(hProv, 0);

		return 0;
	}

	static std::array< BYTE, sizeof(T) >  to_bytes(const T& object)
	{
		std::array< BYTE, sizeof(T) > bytes;

		const BYTE* begin = reinterpret_cast< const byte* >(std::addressof(object));
		const BYTE* end = begin + sizeof(T);
		std::copy(begin, end, std::begin(bytes));

		return bytes;
	}
};