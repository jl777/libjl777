//
//  sha256.h
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifndef crypto777_sha256_h
#define crypto777_sha256_h

#include <stdint.h>

struct sha256_state { uint64_t length; uint32_t state[8],curlen; uint8_t buf[64]; };

#endif
