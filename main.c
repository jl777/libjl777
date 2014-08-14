//
//  main.c
//  libtest
//
//  Created by jl777 on 8/13/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libjl777.h"

char *confjson = "{\
\"MAINNET\":1,\"MIN_NXTCONFIRMS\":3,\
\"active\":[\"BTCD\"],\
\"coins\":[\
{\"name\":\"BTCD\",\"maxevolveiters\":10,\"useaddmultisig\":1,\"nohexout\":1,\"conf\":\"/Users/jimbolaptop/Library/Application Support/BitcoinDark/BitcoinDark.conf\",\"backupdir\":\"/Users/jimbolaptop/backups\",\"asset\":\"11060861818140490423\",\"minconfirms\":10,\"estblocktime\":60,\"rpc\":\"127.0.0.1:14632\",\"ciphers\":[{\"skipjack\":\"LA98Vs3sS6UtdiSaAYwvfgt5GseCVkAJ\"},{\"aes\":\"RUFrkuGAUuv8wsoiNwCvXenjxAfAgsTdAt\"},{\"blowfish\":\"RVHigwQquJR9cA6R6M143H6ZiPep7S9Udt\"}],\"clonesmear\":1,\"pubaddr\":\"RUHAPSpJDHeFgFd1J34WHJ69TpkMBsWtBt\",\"privacyServer\":\"127.0.0.1\"}]\
}";

int main(int argc,const char *argv[])
{
    void **coinptrs;
    coinptrs = calloc(1,sizeof(*coinptrs));
    libjl777_start(coinptrs,confjson);
    while ( 1 )
        sleep(60);
    return(0);
}

