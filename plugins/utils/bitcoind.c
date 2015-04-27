//
//  bitcoind.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifdef DEFINES_ONLY
#ifndef crypto777_bitcoind_h
#define crypto777_bitcoind_h
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "uthash.h"
#include "utils777.c"
#include "msig.c"


#endif
#else
#ifndef crypto777_bitcoind_c
#define crypto777_bitcoind_c

#ifndef crypto777_bitcoind_h
#define DEFINES_ONLY
#include "bitcoind.c"
#undef DEFINES_ONLY
#endif


#endif
#endif
