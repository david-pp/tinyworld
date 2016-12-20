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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
 
static uint32_t
ketama_hash(const char *key, size_t key_length, uint32_t alignment)
{
    unsigned char results[16];

    md5_signature((unsigned char*)key, key_length, results);

    return ((uint32_t) (results[3 + alignment * 4] & 0xFF) << 24)
        | ((uint32_t) (results[2 + alignment * 4] & 0xFF) << 16)
        | ((uint32_t) (results[1 + alignment * 4] & 0xFF) << 8)
        | (results[0 + alignment * 4] & 0xFF);
}