#ifndef __COMMON_HASHKIT_H
#define __COMMON_HASHKIT_H

#include <stdlib.h>

#define KEY_LENGTH 8

#ifdef __cplusplus
extern "C" {
#endif

void genkey(unsigned char* key, const char* input, size_t inputlen, unsigned int id);
const char* key2text(unsigned char* key);

#ifdef __cplusplus
}
#endif

#endif // __COMMON_HASHKIT_H