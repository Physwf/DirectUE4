#pragma once

#include "UnrealMath.h"

#include <map>
#include <string>

typedef union
{
	uint8  c[64];
	uint32 l[16];
} SHA1_WORKSPACE_BLOCK;

/** Stores an SHA hash generated by FSHA1. */
class FSHAHash
{
public:
	uint8 Hash[20];

	FSHAHash()
	{
		memset(Hash, 0, sizeof(Hash));
	}

// 	inline FString ToString() const
// 	{
// 		return BytesToHex((const uint8*)Hash, sizeof(Hash));
// 	}
// 	void FromString(const FString& Src)
// 	{
// 		check(Src.Len() == 40);
// 		HexToBytes(Src, Hash);
// 	}

	friend bool operator==(const FSHAHash& X, const FSHAHash& Y)
	{
		return memcmp(&X.Hash, &Y.Hash, sizeof(X.Hash)) == 0;
	}

	friend bool operator!=(const FSHAHash& X, const FSHAHash& Y)
	{
		return memcmp(&X.Hash, &Y.Hash, sizeof(X.Hash)) != 0;
	}

	friend uint32 GetTypeHash(FSHAHash const& InKey);
};

class FSHA1
{
public:

	enum { DigestSize = 20 };
	// Constructor and Destructor
	FSHA1();
	~FSHA1();

	uint32 m_state[5];
	uint32 m_count[2];
	uint32 __reserved1[1];
	uint8  m_buffer[64];
	uint8  m_digest[20];
	uint32 __reserved2[3];

	void Reset();

	// Update the hash value
	void Update(const uint8 *data, uint32 len);

	// Update the hash value with string
	//void UpdateWithString(const TCHAR *data, uint32 len);

	// Finalize hash and report
	void Final();

	// Report functions: as pre-formatted and raw data
	void GetHash(uint8 *puDest);

	/**
	* Calculate the hash on a single block and return it
	*
	* @param Data Input data to hash
	* @param DataSize Size of the Data block
	* @param OutHash Resulting hash value (20 byte buffer)
	*/
	static void HashBuffer(const void* Data, uint32 DataSize, uint8* OutHash);

	/**
	* Generate the HMAC (Hash-based Message Authentication Code) for a block of data.
	* https://en.wikipedia.org/wiki/Hash-based_message_authentication_code
	*
	* @param Key		The secret key to be used when generating the HMAC
	* @param KeySize	The size of the key
	* @param Data		Input data to hash
	* @param DataSize	Size of the Data block
	* @param OutHash	Resulting hash value (20 byte buffer)
	*/
	static void HMACBuffer(const void* Key, uint32 KeySize, const void* Data, uint32 DataSize, uint8* OutHash);

	/**
	* Shared hashes.sha reading code (each platform gets a buffer to the data,
	* then passes it to this function for processing)
	*
	* @param Buffer Contents of hashes.sha (probably loaded from an a section in the executable)
	* @param BufferSize Size of Buffer
	* @param bDuplicateKeyMemory If Buffer is not always loaded, pass true so that the 20 byte hashes are duplicated
	*/
	//static void InitializeFileHashesFromBuffer(uint8* Buffer, int32 BufferSize, bool bDuplicateKeyMemory = false);

	/**
	* Gets the stored SHA hash from the platform, if it exists. This function
	* must be able to be called from any thread.
	*
	* @param Pathname Pathname to the file to get the SHA for
	* @param Hash 20 byte array that receives the hash
	* @param bIsFullPackageHash true if we are looking for a full package hash, instead of a script code only hash
	*
	* @return true if the hash was found, false otherwise
	*/
	//static bool GetFileSHAHash(const TCHAR* Pathname, uint8 Hash[20], bool bIsFullPackageHash = true);
private:
	// Private SHA-1 transformation
	void Transform(uint32 *state, const uint8 *buffer);

	// Member variables
	uint8 m_workspace[64];
	SHA1_WORKSPACE_BLOCK *m_block; // SHA1 pointer to the byte array above

	/** Global map of filename to hash value, filled out in InitializeFileHashesFromBuffer */
	static std::map<std::string, uint8*> FullFileSHAHashMap;

	/** Global map of filename to hash value, but for script-only SHA hashes */
	static std::map<std::string, uint8*> ScriptSHAHashMap;
};