/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NC_HASHKIT_H_
#define _NC_HASHKIT_H_

typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;


uint32_t hash_one_at_a_time(const char *key, size_t key_length);
void md5_signature(const unsigned char *key, unsigned int length, unsigned char *result);
uint32_t hash_md5(const char *key, size_t key_length);
uint32_t hash_crc16(const char *key, size_t key_length);
uint32_t hash_crc32(const char *key, size_t key_length);
uint32_t hash_crc32a(const char *key, size_t key_length);
uint32_t hash_fnv1_64(const char *key, size_t key_length);
uint32_t hash_fnv1a_64(const char *key, size_t key_length);
uint32_t hash_fnv1_32(const char *key, size_t key_length);
uint32_t hash_fnv1a_32(const char *key, size_t key_length);
uint32_t hash_hsieh(const char *key, size_t key_length);
uint32_t hash_jenkins(const char *key, size_t length);
uint32_t hash_murmur(const char *key, size_t length);

typedef uint32_t (*hash_function)(const char * key, size_t length);

#endif
