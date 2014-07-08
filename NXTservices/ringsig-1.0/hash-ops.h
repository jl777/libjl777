// Copyright (c) 2011-2014 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#pragma pack(push, 1)
union hash_state {
    uint8_t b[200];
    uint64_t w[25];
};
#pragma pack(pop)
_Static_assert(sizeof(union hash_state) == 200, "Invalid structure size");

void hash_permutation(union hash_state *state);
//void hash_process(union hash_state *state, const uint8_t *buf, size_t count);

enum {
    HASH_DATA_AREA = 136
};
