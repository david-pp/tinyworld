
#include "hashkit.h"
#include "nc_hashkit.h"
#include <string.h>
#include <stdio.h>

const char* key2text(unsigned char* key)
{
    static char text[256] = "";

    size_t i = 0;
    for (i = 0; i < KEY_LENGTH; i++)
        snprintf(&text[2*i], 1, "%2X", key[i]);

    return text;
}

void genkey(unsigned char * key, const char* input, size_t inputlen, unsigned int id)
{
    hash_function functions[] = {
        hash_one_at_a_time,
        hash_crc16,
        hash_crc32,
        hash_crc32a,
        hash_fnv1_64,
        hash_fnv1a_64,
        hash_fnv1_32,
        hash_fnv1a_32,
        hash_hsieh,
        hash_jenkins,
        hash_murmur,
    };

    if (sizeof(functions)/sizeof(hash_function) >= 10)
    {
        hash_function hash = functions[id%10];
        if (hash)
        {
            uint32_t v1 = hash((const char*)input, inputlen);
            uint32_t v2 = v1 ^ 0xabcdef;

            memcpy(key, &v1, KEY_LENGTH/2);
            memcpy(key+KEY_LENGTH/2, &v2, KEY_LENGTH/2);
            return;
        }
    }

    memcpy(key, input, KEY_LENGTH);
    return;
}