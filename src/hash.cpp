#include "hash.h"

uint32_t HashDJB2(const void* buf, uint32_t numBytes) {
    uint32_t hash = 0x1505; /* djb2 offset */
    const char* bytes = reinterpret_cast<const char*>(buf);

    for (uint32_t i = 0U; i < numBytes; i++)
    {
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(bytes[i]);
    }

    return hash;
}

uint64_t HashFNV1A_64(const void* buf, uint32_t numBytes) {
    uint64_t hash = 0xCBF29CE484222325; /* FNV-1a (64-bit) offset */
    static const uint64_t prime = 0x00000100000001B3; /* FNV' = 2^40 + 2^8 + 0xB3 */
    const char* bytes = reinterpret_cast<const char*>(buf);

    for (uint32_t i = 0U; i < numBytes; i++) {
        hash = prime * (hash ^ static_cast<unsigned char>(bytes[i]));
    }

    return hash;
}
