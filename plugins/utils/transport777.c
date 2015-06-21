//
//  transport777.c
//  crypto777
//
//  Created by James on 4/9/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//
#include "crypto777.h"

char *conv_ipv6(char *ipv6addr)
{
    static unsigned char IPV4CHECK[10]; // 80 ZERO BITS for testing
    char ipv4str[4096];
    struct sockaddr_in6 ipv6sa;
    in_addr_t *ipv4bin;
    unsigned char *bytes;
    int32_t isok;
    strcpy(ipv4str,ipv6addr);
    //isok = !uv_inet_pton(AF_INET,(const char*)ipv6addr,&ipv6sa.sin6_addr);
    //printf("isok.%d\n",isok);
    isok = inet_pton(AF_INET6,ipv6addr,&ipv6sa.sin6_addr);
    if ( isok == 0 )
    {
        bytes = ((struct sockaddr_in6 *)&ipv6sa)->sin6_addr.s6_addr;
        if ( memcmp(bytes,IPV4CHECK,sizeof(IPV4CHECK)) != 0 ) // check its IPV4 really
        {
            bytes += 12;
            ipv4bin = (in_addr_t *)bytes;
#ifndef _WIN32
            if ( inet_ntop(AF_INET,ipv4bin,ipv4str,sizeof(ipv4str)) == 0 )
#endif
                isok = 0;
        } else isok = 0;
    }
    if ( isok != 0 )
        strcpy(ipv6addr,ipv4str);
    return(ipv6addr); // it is ipv4 now
}

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

