
#ifndef __CASTOR_HASH_H__
#define __CASTOR_HASH_H__

static inline uint32_t
Hash_Mix32(uint32_t n)
{
    n ^= n >> 16;
    n *= 0x85ebca6b;
    n ^= n >> 13;
    n *= 0xc2b2ae35;
    n ^= n >> 16;

    return n;
}

//----------

static inline uint64_t
Hash_Mix64(uint64_t n)
{
    n ^= n >> 33;
    n *= 0xff51afd7ed558ccdULL;
    n ^= n >> 33;
    n *= 0xc4ceb9fe1a85ec53ULL;
    n ^= n >> 33;

    return n;
}

#endif /* __CASTOR_HASH_H__ */

