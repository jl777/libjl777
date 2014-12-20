
//
//  lotto.h
//
//  Created by jl777 on 12/20/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef LOTTO_H
#define LOTTO_H

int32_t update_lotto_transactions(char *refNXTaddr,char *assetidstr)
{
    char assetid[1024],cmd[1024],*jsonstr;
    int32_t iter,i,n = 0;
    cJSON *item,*json,*array;
    if ( refNXTaddr == 0 || assetidstr == 0 )
    {
        printf("illegal refNXTaddr.(%s)\n",refNXTaddr);
        return(0);
    }
    for (iter=0; iter<2; iter++)
    {
        sprintf(cmd,"%s=%s&account=%s&asset=%s",_NXTSERVER,(iter == 0) ? "getAssetTransfers" : "getTrades",refNXTaddr,assetidstr);
        if ( (jsonstr= issue_NXTPOST(0,cmd)) != 0 )
        {
            if ( (json= cJSON_Parse(jsonstr)) != 0 )
            {
                if ( (array= cJSON_GetObjectItem(json,"transactions")) != 0 && is_cJSON_Array(array) != 0 && (n= cJSON_GetArraySize(array)) > 0 )
                {
                    for (i=0; i<n; i++)
                    {
                        process_lotto(refNXTaddr,assetidstr,0,cp);
                    }
                }
                free_json(json);
            }
            free(jsonstr);
        } else printf("iter.%d error with update_lotto_transactions.(%s)\n",iter,cmd);
    }
    return(n);
}

#endif

