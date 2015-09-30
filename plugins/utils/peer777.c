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

struct crypto777_node *crypto777_findpeer(struct crypto777_node *nn,int32_t dir,char *type,char *ipaddr,uint16_t port)
{
    int32_t i;
    struct crypto777_node *peer;
    struct crypto777_transport *transport;
    for (i=0; i<nn->numpeers; i++)
        if ( (peer= nn->peers[i]) != 0 )
        {
            transport = &peer->transport;
            if ( strcmp(transport->ipaddr,ipaddr) == 0 && transport->port == port )
            {
                if ( (dir > 0 && strcmp(nn->transport.type,type) == 0) || (dir < 0 && strcmp(transport->type,type) == 0) )
                    return(peer);
            }
        }
    return(0);
}

int32_t crypto777_peer(struct crypto777_node *nn,struct crypto777_node *peer)
{
    struct crypto777_peer *ptr;
    int32_t err=0,err2=0,timeout = 1;//,insock;
    struct crypto777_transport *transport = &nn->transport;
    if ( nn->numpeers < MAXPEERS )
    {
        ptr = &transport->peers[transport->numpeers++];
        printf("%p peer.(%s).%d\n",peer->ip_port,peer->ip_port,peer->transport.port);
        strcpy(ptr->type,peer->transport.type), strcpy(ptr->ip_port,peer->ip_port), ptr->port = peer->transport.port;
        //if ( nn_connect(transport->bus,peer->ip_port) < 0 )
        //    printf("error %d nn_connect.%d bus (%s) | %s\n",err,transport->bus,peer->ip_port,nn_strerror(nn_errno())), getchar();
        if ( (ptr->subsock= nn_socket(AF_SP,NN_SUB)) < 0 || (err= nn_setsockopt(ptr->subsock,NN_SUB,NN_SUB_SUBSCRIBE,"",0)) < 0 || (err2= nn_connect(ptr->subsock,ptr->ip_port)) < 0 )
        {
            printf("error %d too many subs.%d nodeid.%d peerid.%d err.%s err.%d err2.%d ip_port.(%s)\n",ptr->subsock,nn->numpeers,nn->nodeid,peer->nodeid,nn_strerror(nn_errno()),err,err2,ptr->ip_port);
            if ( ptr->subsock >= 0 )
                nn_shutdown(ptr->subsock,0);
            return(-1);
        }
        if ( nn_setsockopt(ptr->subsock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
            printf("error setting recv timeout to %d\n",timeout);
        /*if ( (ptr->pairsock= nn_socket(AF_SP,NN_PAIR)) < 0 )
         printf("error %d nn_socket err.%s\n",ptr->pairsock,nn_strerror(nn_errno()));
         else if ( (err= nn_bind(ptr->pairsock,transport->ip_port)) < 0 )
         printf("error %d nn_bind.%d (%s) | %s\n",err,ptr->pairsock,transport->ip_port,nn_strerror(nn_errno()));
         else if ( nn_connect(ptr->pairsock,peer->ip_port) < 0 )
         printf("error %d nn_connect.%d pairsock (%s) | %s\n",err,ptr->pairsock,peer->ip_port,nn_strerror(nn_errno()));
         if ( nn_setsockopt(ptr->pairsock,NN_SOL_SOCKET,NN_RCVTIMEO,&timeout,sizeof(timeout)) < 0 )
         printf("error setting recv timeout to %d\n",timeout);
         
         if ( (insock= nn_socket(AF_SP,NN_PAIR)) < 0 )
         printf("error %d nn_socket err.%s\n",insock,nn_strerror(nn_errno()));
         else if ( (err= nn_bind(insock,peer->ip_port)) < 0 )
         printf("error %d nn_bind.%d (%s) | %s\n",err,insock,peer->ip_port,nn_strerror(nn_errno()));
         else if ( nn_connect(insock,nn->ip_port) < 0 )
         printf("error %d nn_connect.%d insock (%s) | %s\n",err,insock,nn->ip_port,nn_strerror(nn_errno()));
         else peer->transport.insocks[peer->transport.numpairs++] = insock;*/
        set_bloombits(&nn->blooms[0],peer->nxt64bits);
        nn->peers[nn->numpeers++] = peer;
        return(nn->numpeers);
    } else return(-1);
}


int32_t crypto777_depth(struct crypto777_node *bestpeers[],struct crypto777_node *nn,int32_t depth,uint64_t dest)
{
    struct crypto777_node *_bestpeers[MAXPEERS],*peer;
    int32_t i,n,vals[MAXPEERS],bestdepth = -1;
    memset(bestpeers,0,sizeof(*bestpeers) * nn->numpeers);
    for (i=0; i<nn->numpeers; i++)
    {
        peer = nn->peers[i];
        if ( peer->nxt64bits == dest )
        {
            bestpeers[0] = nn->peers[i];
            return(depth);
        }
    }
    if ( depth < MAXHOPS )
    {
        for (i=0; i<nn->numpeers; i++)
        {
            peer = nn->peers[i];
            if ( in_bloombits(&peer->blooms[depth+1],dest) == 0 && (vals[i]= crypto777_depth(_bestpeers,peer,depth+1,dest)) >= 0 )
            {
                set_bloombits(&nn->blooms[MAXHOPS-vals[i]],dest);
                if ( bestdepth < 0 || vals[i] < bestdepth )
                {
                    bestdepth = vals[i];
                    return(vals[i]);
                }
            }
            else set_bloombits(&peer->blooms[depth+1],dest);
        }
        for (i=n=0; i<nn->numpeers; i++)
            if ( vals[i] == bestdepth )
                bestpeers[n++] = nn->peers[i];
    }
    if ( bestdepth < 0 )
        set_bloombits(&nn->blooms[depth],dest);
    return(bestdepth);
}

int32_t _search_uint32(uint32_t *buf,int32_t n,uint32_t val)
{
    int32_t j;
    for (j=0; j<n; j++)
        if ( buf[j] == val || buf[j] == 0 )
            break;
    return(j);
}

uint64_t crypto777_checkfifo(int32_t actionflag,struct crypto777_node *nn,uint8_t *buf,int32_t len,uint32_t crc)
{
    int32_t i,j,n = (int32_t)(sizeof(nn->peerfifo[0])/sizeof(nn->peerfifo[0][0]));
    uint64_t mask = 0;
    return(-1);
    for (i=j=0; i<=nn->numpeers; i++)
    {
        j = _search_uint32(nn->peerfifo[i],n,crc);
        if ( j == n || (nn->peerfifo[i][j] == 0 && (actionflag & (1LL << i)) != 0) )
            mask |= (1LL << i);
        else
        {
            for (j=n-1; j>0; j>>=1)
                nn->peerfifo[i][j] = nn->peerfifo[i][j >> 1];
        }
        nn->peerfifo[i][0] = crc;
    }
    if ( 0 && nn->nodeid == 0 )
        printf("actionflag.%llx mask.%llx n.%d crc.%u | numpackets.%d j.%d\n",(long long)actionflag,(long long)mask,n,crc,numpackets,j);
    return(mask);
}

void select_peers(struct crypto777_node *nn)
{
    static int counts[NETWORKSIZE],didinit;
    int32_t j,k,p,i;
    for (i=0; i<MAXPEERS; i++)
        p = (nn->nodeid+(NETWORKSIZE/MAXPEERS)*i+1) % NETWORKSIZE, crypto777_peer(nn,Network[p]), counts[p]++, didinit++;
    
    for (j=0; j<MAXPEERS; j++)
    {break;
        p = (j == 0) ? ((nn->nodeid + NETWORKSIZE/2) % NETWORKSIZE) : (rand() % NETWORKSIZE);
        //p = ((nn->nodeid + j*(NETWORKSIZE/MAXPEERS - 1)) % NETWORKSIZE);
        if ( j > 0 )
        {
            for (k=0; k<j; k++)
                if ( nn->peers[k] != 0 && nn->peers[k]->nodeid == p )
                    break;
        } else k = 0;
        if ( p != nn->nodeid && k == j && counts[p] < MAXPEERS+2 )
        {
            crypto777_peer(nn,Network[p]);
            counts[p]++;
            didinit++;
            //if ( (i= _search_uint32(disconnected,num,p)) != num )
            //    disconnected[i] = disconnected[--num];
        }
        else j--;
    }
    if ( didinit == MAXPEERS * NETWORKSIZE )
    {
        for (i=0; i<NETWORKSIZE; i++)
            printf("%d ",counts[i]);
        printf("connection counts\n");
    }
}

int32_t update_peers(struct crypto777_node *nn,struct crypto777_block *block,uint64_t peermetrics[][MAXPEERS],int32_t numpeers,int32_t packet_leverage)
{
    int32_t i;
    for (i=0; i<numpeers; i++)
        if ( block->gen.metric > peermetrics[block->blocknum][i] )
            break;
    if ( i != nn->numpeers )
    {
        crypto777_broadcast(nn,(uint8_t *)block,block->blocksize,packet_leverage,0);
        return(i);
    }
    return(-1);
}

