/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include "crypto777.h"

void crypto777_user(struct crypto777_user *user,uint64_t nxt64bits,int32_t numchallenges,uint32_t range)
{
    SaM_Initialize(&user->XORpad);
    user->nxt64bits = nxt64bits;
    memset(&user->shared_SaM,0,sizeof(user->shared_SaM));
    //if ( (user->numchallenges= numchallenges) != 0 )
    //    user->challenges = realloc(user->challenges,numchallenges * sizeof(*user->challenges));
    //user->range = range;
}

int32_t crypto777_link(uint64_t mynxt64bits,struct crypto777_link *conn,bits384 nxtpass,uint64_t other64bits,int32_t numchallenges,uint32_t range)
{
    char NXTACCTSECRET[1024];
    int32_t haspubkey = 0;
    uint64_t nxt64bits = 0;
    if ( conn->my64bits != mynxt64bits || conn->shared_curve25519.txid == 0 )
    {
        conn->my64bits = mynxt64bits;
        init_hexbytes_noT(NXTACCTSECRET,nxtpass.bytes,sizeof(nxtpass));
        conn->shared_curve25519 = calc_sharedsecret(&nxt64bits,&haspubkey,(uint8_t *)NXTACCTSECRET,(int32_t)strlen(NXTACCTSECRET),other64bits);
        if ( haspubkey != 0 )
        {
            crypto777_user(&conn->send,nxt64bits,numchallenges,range);
            //conn->send.supernonce = (uint32_t)time(NULL);
            crypto777_user(&conn->recv,other64bits,0,0);
            conn->send.shared_SaM.sig = conn->recv.shared_SaM.sig = conn->shared_curve25519; // jl777: until quantum resistant is ready
            return(0);
        } else printf("crypto777_link cant create connection to %llu without pubkey\n",(long long)other64bits);
        return(-1);
    }
    else
    {
        //printf("now.%lu prev.%u\n",time(NULL),conn->send.prevtimestamp);
        while ( time(NULL) == conn->send.prevtimestamp || time(NULL) == conn->recv.prevtimestamp )
            msleep(250);
        return(0);
    }
}

