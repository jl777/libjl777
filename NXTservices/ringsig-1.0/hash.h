// Copyright (c) 2011-2014 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stddef.h>

enum {
    HASH_SIZE = 32
};

#pragma pack(push, 1)
struct hash {
    unsigned char data[HASH_SIZE];
};
#pragma pack(pop)
_Static_assert(sizeof(struct hash) == HASH_SIZE, "Invalid structure size");

void cn_fast_hash(const void *data, size_t length, struct hash *hash);
