#include "SecureHash.h"

template <typename T, uint32 N>
char(&ArrayCountHelper(const T(&)[N]))[N + 1];

#define ARRAY_COUNT( array ) (sizeof(ArrayCountHelper(array)) - 1)

/** Global maps of filename to hash value */
std::map<std::string, uint8*> FSHA1::FullFileSHAHashMap;
std::map<std::string, uint8*> FSHA1::ScriptSHAHashMap;

// Rotate x bits to the left
#ifndef ROL32
#ifdef _MSC_VER
#define ROL32(_val32, _nBits) _rotl(_val32, _nBits)
#else
#define ROL32(_val32, _nBits) (((_val32)<<(_nBits))|((_val32)>>(32-(_nBits))))
#endif
#endif

#if PLATFORM_LITTLE_ENDIAN
#define SHABLK0(i) (m_block->l[i] = (ROL32(m_block->l[i],24) & 0xFF00FF00) | (ROL32(m_block->l[i],8) & 0x00FF00FF))
#else
#define SHABLK0(i) (m_block->l[i])
#endif

#define SHABLK(i) (m_block->l[i&15] = ROL32(m_block->l[(i+13)&15] ^ m_block->l[(i+8)&15] \
	^ m_block->l[(i+2)&15] ^ m_block->l[i&15],1))

// SHA-1 rounds
#define _R0(v,w,x,y,z,i) { z+=((w&(x^y))^y)+SHABLK0(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30); }
#define _R1(v,w,x,y,z,i) { z+=((w&(x^y))^y)+SHABLK(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30); }
#define _R2(v,w,x,y,z,i) { z+=(w^x^y)+SHABLK(i)+0x6ED9EBA1+ROL32(v,5); w=ROL32(w,30); }
#define _R3(v,w,x,y,z,i) { z+=(((w|x)&y)|(w&x))+SHABLK(i)+0x8F1BBCDC+ROL32(v,5); w=ROL32(w,30); }
#define _R4(v,w,x,y,z,i) { z+=(w^x^y)+SHABLK(i)+0xCA62C1D6+ROL32(v,5); w=ROL32(w,30); }

FSHA1::FSHA1()
{
	m_block = (SHA1_WORKSPACE_BLOCK *)m_workspace;

	Reset();
}

FSHA1::~FSHA1()
{
	Reset();
}

void FSHA1::Reset()
{
	// SHA1 initialization constants
	m_state[0] = 0x67452301;
	m_state[1] = 0xEFCDAB89;
	m_state[2] = 0x98BADCFE;
	m_state[3] = 0x10325476;
	m_state[4] = 0xC3D2E1F0;

	m_count[0] = 0;
	m_count[1] = 0;
}

void FSHA1::Transform(uint32 *state, const uint8 *buffer)
{
	// Copy state[] to working vars
	uint32 a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];

	memcpy(m_block, buffer, 64);

	// 4 rounds of 20 operations each. Loop unrolled.
	_R0(a, b, c, d, e, 0); _R0(e, a, b, c, d, 1); _R0(d, e, a, b, c, 2); _R0(c, d, e, a, b, 3);
	_R0(b, c, d, e, a, 4); _R0(a, b, c, d, e, 5); _R0(e, a, b, c, d, 6); _R0(d, e, a, b, c, 7);
	_R0(c, d, e, a, b, 8); _R0(b, c, d, e, a, 9); _R0(a, b, c, d, e, 10); _R0(e, a, b, c, d, 11);
	_R0(d, e, a, b, c, 12); _R0(c, d, e, a, b, 13); _R0(b, c, d, e, a, 14); _R0(a, b, c, d, e, 15);
	_R1(e, a, b, c, d, 16); _R1(d, e, a, b, c, 17); _R1(c, d, e, a, b, 18); _R1(b, c, d, e, a, 19);
	_R2(a, b, c, d, e, 20); _R2(e, a, b, c, d, 21); _R2(d, e, a, b, c, 22); _R2(c, d, e, a, b, 23);
	_R2(b, c, d, e, a, 24); _R2(a, b, c, d, e, 25); _R2(e, a, b, c, d, 26); _R2(d, e, a, b, c, 27);
	_R2(c, d, e, a, b, 28); _R2(b, c, d, e, a, 29); _R2(a, b, c, d, e, 30); _R2(e, a, b, c, d, 31);
	_R2(d, e, a, b, c, 32); _R2(c, d, e, a, b, 33); _R2(b, c, d, e, a, 34); _R2(a, b, c, d, e, 35);
	_R2(e, a, b, c, d, 36); _R2(d, e, a, b, c, 37); _R2(c, d, e, a, b, 38); _R2(b, c, d, e, a, 39);
	_R3(a, b, c, d, e, 40); _R3(e, a, b, c, d, 41); _R3(d, e, a, b, c, 42); _R3(c, d, e, a, b, 43);
	_R3(b, c, d, e, a, 44); _R3(a, b, c, d, e, 45); _R3(e, a, b, c, d, 46); _R3(d, e, a, b, c, 47);
	_R3(c, d, e, a, b, 48); _R3(b, c, d, e, a, 49); _R3(a, b, c, d, e, 50); _R3(e, a, b, c, d, 51);
	_R3(d, e, a, b, c, 52); _R3(c, d, e, a, b, 53); _R3(b, c, d, e, a, 54); _R3(a, b, c, d, e, 55);
	_R3(e, a, b, c, d, 56); _R3(d, e, a, b, c, 57); _R3(c, d, e, a, b, 58); _R3(b, c, d, e, a, 59);
	_R4(a, b, c, d, e, 60); _R4(e, a, b, c, d, 61); _R4(d, e, a, b, c, 62); _R4(c, d, e, a, b, 63);
	_R4(b, c, d, e, a, 64); _R4(a, b, c, d, e, 65); _R4(e, a, b, c, d, 66); _R4(d, e, a, b, c, 67);
	_R4(c, d, e, a, b, 68); _R4(b, c, d, e, a, 69); _R4(a, b, c, d, e, 70); _R4(e, a, b, c, d, 71);
	_R4(d, e, a, b, c, 72); _R4(c, d, e, a, b, 73); _R4(b, c, d, e, a, 74); _R4(a, b, c, d, e, 75);
	_R4(e, a, b, c, d, 76); _R4(d, e, a, b, c, 77); _R4(c, d, e, a, b, 78); _R4(b, c, d, e, a, 79);

	// Add the working vars back into state
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
}

// do not remove #undefs, or you can get name collision with libc++'s <functional>
#undef _R0
#undef _R1
#undef _R2
#undef _R3
#undef _R4

// Use this function to hash in binary data
void FSHA1::Update(const uint8 *data, uint32 len)
{
	uint32 i, j;

	j = (m_count[0] >> 3) & 63;

	if ((m_count[0] += len << 3) < (len << 3)) m_count[1]++;

	m_count[1] += (len >> 29);

	if ((j + len) > 63)
	{
		i = 64 - j;
		memcpy(&m_buffer[j], data, i);
		Transform(m_state, m_buffer);

		for (; i + 63 < len; i += 64) Transform(m_state, &data[i]);

		j = 0;
	}
	else i = 0;

	memcpy(&m_buffer[j], &data[i], len - i);
}

// Use this function to hash in strings
void FSHA1::UpdateWithString(const char *String, uint32 Length)
{
	Update((const uint8*)String, Length * sizeof(char));
}

void FSHA1::Final()
{
	uint32 i;
	uint8 finalcount[8];

	for (i = 0; i < 8; i++)
	{
		finalcount[i] = (uint8)((m_count[((i >= 4) ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255); // Endian independent
	}

	Update((uint8*)"\200", 1);

	while ((m_count[0] & 504) != 448)
	{
		Update((uint8*)"\0", 1);
	}

	Update(finalcount, 8); // Cause a SHA1Transform()

	for (i = 0; i < 20; i++)
	{
		m_digest[i] = (uint8)((m_state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
	}
}

// Get the raw message digest
void FSHA1::GetHash(uint8 *puDest)
{
	memcpy(puDest, m_digest, 20);
}

/**
* Calculate the hash on a single block and return it
*
* @param Data Input data to hash
* @param DataSize Size of the Data block
* @param OutHash Resulting hash value (20 byte buffer)
*/
void FSHA1::HashBuffer(const void* Data, uint32 DataSize, uint8* OutHash)
{
	// do an atomic hash operation
	FSHA1 Sha;
	Sha.Update((const uint8*)Data, DataSize);
	Sha.Final();
	Sha.GetHash(OutHash);
}

void FSHA1::HMACBuffer(const void* Key, uint32 KeySize, const void* Data, uint32 DataSize, uint8* OutHash)
{
	const uint8 BlockSize = 64;
	const uint8 HashSize = 20;
	uint8 FinalKey[BlockSize];

	// Fit 'Key' into a BlockSize-aligned 'FinalKey' value
	if (KeySize > BlockSize)
	{
		HashBuffer(Key, KeySize, FinalKey);

		memset(FinalKey + HashSize, 0, BlockSize - HashSize);
	}
	else if (KeySize < BlockSize)
	{
		memcpy(FinalKey, Key, KeySize);
		memset(FinalKey + KeySize, 0, BlockSize - KeySize);
	}
	else
	{
		memcpy(FinalKey, Key, KeySize);
	}


	uint8 OKeyPad[BlockSize];
	uint8 IKeyPad[BlockSize];

	for (int32 i = 0; i<BlockSize; i++)
	{
		OKeyPad[i] = 0x5C ^ FinalKey[i];
		IKeyPad[i] = 0x36 ^ FinalKey[i];
	}


	// Start concatenating/hashing the pads/data etc: Hash(OKeyPad + Hash(IKeyPad + Data))
	uint8* IKeyPad_Data = new uint8[ARRAY_COUNT(IKeyPad) + DataSize];

	memcpy(IKeyPad_Data, IKeyPad, ARRAY_COUNT(IKeyPad));
	memcpy(IKeyPad_Data + ARRAY_COUNT(IKeyPad), Data, DataSize);


	uint8 IKeyPad_Data_Hash[HashSize];

	HashBuffer(IKeyPad_Data, ARRAY_COUNT(IKeyPad) + DataSize, IKeyPad_Data_Hash);

	delete[] IKeyPad_Data;


	uint8 OKeyPad_IHash[ARRAY_COUNT(OKeyPad) + HashSize];

	memcpy(OKeyPad_IHash, OKeyPad, ARRAY_COUNT(OKeyPad));
	memcpy(OKeyPad_IHash + ARRAY_COUNT(OKeyPad), IKeyPad_Data_Hash, HashSize);


	// Output the final hash
	HashBuffer(OKeyPad_IHash, ARRAY_COUNT(OKeyPad_IHash), OutHash);
}


// /**
// * Shared hashes.sha reading code (each platform gets a buffer to the data,
// * then passes it to this function for processing)
// */
// void FSHA1::InitializeFileHashesFromBuffer(uint8* Buffer, int32 BufferSize, bool bDuplicateKeyMemory)
// {
// 	// the start of the file is full file hashes
// 	bool bIsDoingFullFileHashes = true;
// 	// if it exists, parse it
// 	int32 Offset = 0;
// 	while (Offset < BufferSize)
// 	{
// 		// format is null terminated string followed by hash
// 		char* Filename = (char*)Buffer + Offset;
// 
// 		// make sure it's not an empty string (this could happen with an empty hash file)
// 		if (Filename[0])
// 		{
// 			// skip over the file
// 			Offset += strlen(Filename) + 1;
// 
// 			// if we hit the magic separator between sections
// 			if (strcmp(Filename, HASHES_SHA_DIVIDER) == 0)
// 			{
// 				// switch to script sha
// 				bIsDoingFullFileHashes = false;
// 
// 				// don't process a hash for this special case
// 				continue;
// 			}
// 
// 			// duplicate the memory if needed (some hash sources are always loaded, ie in the executable,
// 			// so no need to duplicate that memory, we can just point into the middle of it)
// 			uint8* Hash;
// 			if (bDuplicateKeyMemory)
// 			{
// 				Hash = (uint8*)malloc(20);
// 				memcpy(Hash, Buffer + Offset, 20);
// 			}
// 			else
// 			{
// 				Hash = Buffer + Offset;
// 			}
// 
// 			// offset now points to the hash data, so save a pointer to it
// 			(bIsDoingFullFileHashes ? FullFileSHAHashMap : ScriptSHAHashMap).Add(ANSI_TO_TCHAR(Filename), Hash);
// 
// 			// move the offset over the hash (always 20 bytes)
// 			Offset += 20;
// 		}
// 	}
// 
// 	// we should be exactly at the end
// 	assert(Offset == BufferSize);
// 
// }


// /**
// * Gets the stored SHA hash from the platform, if it exists. This function
// * must be able to be called from any thread.
// *
// * @param Pathname Pathname to the file to get the SHA for
// * @param Hash 20 byte array that receives the hash
// * @param bIsFullPackageHash true if we are looking for a full package hash, instead of a script code only hash
// *
// * @return true if the hash was found, false otherwise
// */
// bool FSHA1::GetFileSHAHash(const TCHAR* Pathname, uint8 Hash[20], bool bIsFullPackageHash)
// {
// 	// look for this file in the hash
// 	uint8** HashData = (bIsFullPackageHash ? FullFileSHAHashMap : ScriptSHAHashMap).Find(FPaths::GetCleanFilename(Pathname).ToLower());
// 
// 	// do we want a copy?
// 	if (HashData && Hash)
// 	{
// 		// return it
// 		memcpy(Hash, *HashData, 20);
// 	}
// 
// 	// return true if we found the hash
// 	return HashData != NULL;
// }
