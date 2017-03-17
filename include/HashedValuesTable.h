#pragma once

#include <string>
#include <array>
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

	void removeSalt() {
		if (_salt) {
			_saltLen = 0;
			free(_salt);
			_salt = NULL;
		}
	}

	//It empties the map of hashed values
	void clear() {
		//TODO iterate over elements to free memory
		for (std::map <T, BYTE *>::iterator it = _hashedValues.begin; it != _hashedValues.end; it++)
			free(*it->second);
		_hashedValues.clear();
		if(_salt)free(_salt);
		_saltLen = 0;
		_hashSize = 0;
	}

	bool hasSalt() {
		return _salt != NULL;
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
			if (!(err = hashValue(data.data(), sizeof(T), &hash))) {
				_hashedValues.insert(std::make_pair(value, hash));
				*output = hash;
				*outLen = _hashSize;
			}
		}
		else {
			*output = storedValue->second;
			*outLen = _hashSize; 
		}
		return err;
	}

	template<std::size_t SIZE>
	int getHashBytes(const std::array<byte, SIZE> &value, BYTE **output, unsigned long *outLen) {
		BYTE *hash, *data;
		long len;
		int err = 0;
		std::map<std::array<byte, SIZE>, unsigned char *>::iterator storedValue = _hashedValues.find(value);
		if (storedValue == _hashedValues.end()) {
			data = (BYTE*)value.data();
			//Create data
			if (!(err = hashValue(data, SIZE, &hash))) {
				_hashedValues.insert(std::make_pair(value, hash));
				*output = hash;
				*outLen = _hashSize;
			}
		}
		else {
			*output = storedValue->second;
			*outLen = _hashSize;
		}
		return err;
	}

	int getHashString(const std::string &value, BYTE **output, unsigned long *outLen) {
		BYTE *hash, *data;
		long len;
		int err = 0;
		auto storedValue = _hashedValues.find(value);
		if (storedValue == _hashedValues.end()) {
			data = (BYTE*)value.c_str();
			//Create data
			if (!(err = hashValue(data, value.length(), &hash))) {
				_hashedValues.insert(std::make_pair(value, hash));
				*output = hash;
				*outLen = _hashSize;
			}
		}
		else {
			*output = storedValue->second;
			*outLen = _hashSize;
		}
		return err;
	}

	int getHashWString(const std::wstring &value, BYTE **output, unsigned long *outLen) {
		BYTE *hash, *data;
		long len;
		int err = 0;
		auto storedValue = _hashedValues.find(value);
		if (storedValue == _hashedValues.end()) {
			data = (BYTE*)value.c_str();
			if (!(err = hashValue(data, value.length()*sizeof(wchar_t), &hash))) {
				_hashedValues.insert(std::make_pair(value, hash));
				*output = hash;
				*outLen = _hashSize;
				return 0;
			}
		}
		else {
			*output = storedValue->second;
			*outLen = _hashSize;
		}
		return err;
	}

	//Requires output to be already initialized
	int get32Hash(const T &value, unsigned int* output) {
		int err = 0;
		unsigned long len = 0;
		BYTE *hash = NULL;
		if (output == NULL) return -1;
		if (!(err = getHash(value, &hash, &len))) {
			//Copies the last 4 bytes of the hash to the int output
			memcpy((void *)output, &(hash[_hashSize - 5]), 4);
			return 0;
		}
		return err;
	}

	//Requires output to be already initialized
	int get64Hash(const T &value, unsigned long* output) {
		int err = 0;
		unsigned long len = 0;
		BYTE *hash = NULL;
		if (output == NULL) return -1;
		if (!(err = getHash(value, &hash, &len))) {
			//Copies the last 8 bytes of the hash to the int output
			memcpy((void *)output, &(hash[_hashSize - 9], 8));
		}
		return err;	
	}

	int getHexHash(const T &value, char** output, unsigned long *outLen) {
		int err = 0;
		unsigned long len = 0;
		BYTE *hash = NULL;
		if (output == NULL) return -1;
		if (!(err = getHash(value, &hash, &len))) {
			//Creates a string based hash
			*output = (char*)malloc(_hashSize * 2);
			for (int j = 0; j < _hashSize; j++)
				sprintf(&((*output)[2 * j]), "%02X", hash[j]);
		}
		return err;
	}

private:
	//TODO reset to hash map when find out how to hash arrays
	//std::hash_map <T, unsigned char *> _hashedValues;
	std::map <T, BYTE *> _hashedValues;
	int _saltLen;
	unsigned long _hashSize;
	BYTE *_salt;

	//Deprecated. Update if needed
	int hashValueWithSalt(BYTE *data, unsigned int dataSize, BYTE ** hash) {
		HCRYPTPROV hProv = 0;
		HCRYPTHASH hHash = 0;
		BYTE *pbHash = NULL;
		DWORD dwHashLen;

		//BYTE * pbBuffer = NULL;
		DWORD dwCount;
		DWORD i;
		unsigned long bufLen = 0;

		//Create context and hash object for SHA256
		if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
			return GetLastError();
		}
		if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
			return GetLastError();
		}
		
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

	//Deprecated. Update if needed
	int hashValueWithoutSalt(BYTE *data, unsigned int dataSize, BYTE ** hash) {
		HCRYPTPROV hProv = 0;
		HCRYPTHASH hHash = 0;
		BYTE *pbHash = NULL;
		DWORD dwHashLen;

		//BYTE * pbBuffer = NULL;
		DWORD dwCount;
		DWORD i;
		unsigned long bufLen = 0;

		//Create context and hash object for SHA256
		if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
			return GetLastError();
		}
		if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
			return GetLastError();
		}

		//Pass data to hash
		if (!CryptHashData(hHash, data, dataSize, 0)) {
			return GetLastError();
		}

		//Get the size of the hashed data and create proper buffer to contain it
		dwCount = sizeof(DWORD);
		if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&dwHashLen, &dwCount, 0)) {
			return GetLastError();
		}
		_hashSize = dwHashLen;

		//Create the buffer where to place the hashed data
		if ((pbHash = (BYTE*)malloc(dwHashLen)) == NULL) {
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

		BYTE * pbBuffer = NULL;
		DWORD dwCount;
		DWORD i;

		//Create context and hash object for SHA256
		if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
			return GetLastError();
		}
		if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
			return GetLastError();
		}

		//Copy data to has into a new buffer
		/*pbBuffer = (BYTE*)malloc(dataSize + 1);
		memset(pbBuffer, 0, dataSize + 1);
		for (i = 0; i < dataSize; i++) {
			pbBuffer[i] = (BYTE)data[i];
		}*/

		//Pass data to hash
		if (!CryptHashData(hHash, data, dataSize, 0)) {
			//if (!CryptHashData(hHash, pbBuffer, dataSize, 0)) {
			return GetLastError();
		}

		//If salt has been set add it to the data to hash
		if (hasSalt()) {
			if (!CryptHashData(hHash, _salt, _saltLen, 0)) {
				return GetLastError();
			}
		}

		//Get the size of the hashed data and create proper buffer to contain it
		//TODO this could be skipped as the hash size should not change over time
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
		if ((pbHash = (BYTE*)malloc(dwHashLen)) == NULL) {
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

	static std::array< BYTE, sizeof(T) > to_bytes(const T& object)
	{
		std::array< BYTE, sizeof(T) > bytes;

		const BYTE* begin = reinterpret_cast< const byte* >(std::addressof(object));
		const BYTE* end = begin + sizeof(T);
		std::copy(begin, end, std::begin(bytes));

		return bytes;
	}
};