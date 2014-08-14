//
//  libjl777.h
//  libtest
//
//  Created by jl777 on 8/13/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef libtest_libjl777_h
#define libtest_libjl777_h

#include <stdint.h>

int libjl777_start(void **coinptrs,char *JSON_or_fname);
char *libjl777_JSON(char *JSONstr);
int32_t libjl777_broadcast(void **coinptrs,uint8_t *packet,int32_t len,uint64_t txid,int32_t duration);
char *libjl777_gotpacket(uint8_t *packet,int32_t len,uint64_t txid,int32_t duration);


#endif
