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
#ifdef _WIN32
#define in_addr_t uint32_t
#endif

struct crypto777_transport *crypto777_transport(struct crypto777_node *nn,char *ip_port,char *type)
{
    struct crypto777_transport *transport = &nn->transport;
    int32_t err;//,timeout = 10;
    if ( (transport->port= parse_ipaddr(transport->ipaddr,ip_port)) != 0 )
    {
        strcpy(transport->type,type);
        sprintf(transport->ip_port,"%s%s:%d",strcmp(type,"nano")==0?"tcp://":"",transport->ipaddr,transport->port);
        strcpy(nn->ip_port,transport->ip_port);
        if ( strcmp(type,"nano") == 0 )
        {
            /*if ( (transport->bus= nn_socket(AF_SP,NN_BUS)) < 0 )
             printf("error %d nn_socket err.%s\n",transport->bus,nn_strerror(nn_errno()));
             else if ( (err= nn_bind(transport->bus,transport->ip_port)) < 0 )
             printf("error %d nn_bind.%d bus.(%s) | %s\n",err,transport->bus,transport->ip_port,nn_strerror(nn_errno()));
             else if ( nn_setsockopt(transport->bus,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
             printf("error setting recv timeout to %d\n",timeout);*/
            
            if ( (transport->pub= nn_socket(AF_SP,NN_PUB)) < 0 )
                printf("error %d nn_socket err.%s\n",transport->pub,nn_strerror(nn_errno()));
            else if ( (err= nn_bind(transport->pub,transport->ip_port)) < 0 )
                printf("error %d nn_bind.%d pub.(%s) | %s\n",err,transport->pub,transport->ip_port,nn_strerror(nn_errno()));
        } else printf("unsupported crypto777_transport.(%s)\n",type);
    } else printf("couldnt get valid port from.(%s)\n",ip_port), transport = 0;
    return(transport);
}

int32_t crypto777_send(struct crypto777_transport *dest,uint8_t *msg,long len,uint32_t flags,struct crypto777_transport *src)
{
    int32_t i,retval = -1;
    src->numsent += src->numpeers;
    if ( dest == 0 )
    {
        if ( (retval= nn_send(src->pub,(const void *)msg,len,0)) == 0 )
            retval = (int32_t)len;
        return(retval);
    }
    if ( strcmp(dest->type,"nano") == 0 )
    {
        //if ( (retval= nn_send(src->bus,(const void *)msg,len,0)) == 0 )
        //    retval = (int32_t)len;
        //else
        if ( (retval= nn_send(src->pub,(const void *)msg,len,0)) == 0 )
            retval = (int32_t)len;
        else if ( 0 )
        {
            for (i=0; i<src->numpeers; i++)
                if ( strcmp(dest->ip_port,src->peers[i].ip_port) == 0 )
                {
                    if ( (retval= nn_send(src->peers[i].pairsock,(const void *)msg,len,0)) == 0 )
                        retval = (int32_t)len;
                    else break;
                }
        }
        printf("nn_send src.(%s) %ld -> dest(%s) retval.%d\n",src->ip_port,len,dest->ip_port,retval);
    }
    if ( retval == len )
        numxmit++, Totalxmit++;
    //if ( THROTTLE != 0 )
    //    usleep(THROTTLE * 1000);
    return(retval);
}

