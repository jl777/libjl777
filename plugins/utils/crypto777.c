/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Nxt software, including this file, may be copied, modified, propagated,    *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#ifndef crypto777_c
#define crypto777_c

#include "crypto777.h"

int numpackets,numxmit,Generator,Receiver;
long foundcount,Totalrecv,Totalxmit;
uint64_t currentsearch;
struct crypto777_node *Network[NETWORKSIZE];
struct consensus_model *MODEL;


uint64_t calc_generator(struct crypto777_generator *gen,uint64_t stake,char *email,uint32_t timestamp,uint64_t prevmetric,bits384 prevgen,uint64_t numrounds)
{
    uint64_t hit;
    if ( timestamp == 0 )
        timestamp = (uint32_t)time(NULL);
    memset(gen,0,sizeof(*gen));
    strncpy(gen->email,email,sizeof(gen->email)-1);
    gen->timestamp = timestamp;
    hit = SaM(&gen->hash,(uint8_t *)&prevgen,(int32_t)sizeof(prevgen),(uint8_t *)email,(int32_t)strlen(email));
    if ( (gen->hit= (stake / (hit + 1))) > MAX_CRYPTO777_HIT )
    {
        printf("clip to max\n");
        gen->hit = MAX_CRYPTO777_HIT;
    }
    gen->metric = (gen->hit + prevmetric);
    return(gen->metric);
}

uint64_t get_ledger_total(struct crypto777_ledger *ledger,struct crypto777_node *nn)
{
    uint64_t stake = 0;
    uint32_t i;
    if ( nn == 0 )
    {
        for (i=0; i<ledger->num; i++)
            stake += ledger->vals[i];
    }
    else stake = ledger->vals[nn->nodeid];
    return(stake);
}

struct crypto777_ledger *get_ledger(struct consensus_model *mp,uint32_t itemid)
{
    uint32_t i;
    for (i=0; i<mp->numledgers; i++)
        if ( mp->ledgers[i].itemid == itemid )
            return(&mp->ledgers[i]);
    return(0);
}

uint64_t calc_stake(struct crypto777_node *nn,struct consensus_model *mp)
{
    uint64_t stake = 0;
    uint32_t i,n;
    if ( mp->stakeid == MODEL_ROUNDROBIN )
        return(1000 * MAX_CRYPTO777_HIT);
    else if ( (n= mp->stakeledger.num) > 0 )
    {
        if ( nn == 0 )
        {
            for (i=0; i<n; i++)
                stake += mp->stakeledger.vals[i];
        }
        else stake = mp->stakeledger.vals[nn->nodeid];
    }
    return(stake);
}

int32_t calc_generators(struct crypto777_node *nn,uint64_t sortbuf[MAXPEERS+1][4],struct consensus_model *mp,uint32_t blocknum)
{
    struct crypto777_generator gen;
    struct crypto777_block *prev;
    struct crypto777_node *peer;
    uint64_t stake;
    int32_t i,n = 0;
    uint32_t timestamp = (uint32_t)time(NULL);
    if ( (prev= mp->blocks[blocknum-1]) == 0 )
        return(-1);
    for (i=0; i<=nn->numpeers; i++)
    {
        peer = (i < nn->numpeers) ? nn->peers[i] : nn;
        if ( peer != 0 )
        {
            stake = calc_stake(peer,mp);
            sortbuf[n][0] = calc_generator(&gen,stake,peer->email,timestamp,prev->gen.metric,prev->gen.hash,mp->POW_DIFF);
            sortbuf[n][1] = peer->nodeid;
            sortbuf[n][2] = gen.hit;
            sortbuf[n][3] = prev->gen.metric;
            n++;
        }
    }
    if ( n > 1 )
        revsort64s(&sortbuf[0][0],n,sizeof(sortbuf[0]));
    return(n);
}

struct crypto777_block *sign_block(struct crypto777_node *nn,struct crypto777_generator *gen,uint8_t *rawblock,int32_t rawblocklen,uint32_t blocknum,uint32_t timestamp)
{
    struct crypto777_block *block;
    uint32_t blocksize;
    bits384 blockhash;
    blocksize = (sizeof(struct crypto777_block) + rawblocklen);
    if ( (blocksize & 0xf) != 0 )
        blocksize += 0x10 - (blocksize & 0xf);
    block = calloc(1,blocksize);
    if ( timestamp == 0 )
        timestamp = (uint32_t)time(NULL);
    block->blocksize = blocksize, block->rawblocklen = rawblocklen, block->blocknum = blocknum, block->timestamp = timestamp;
    block->gen = *gen;
    if ( rawblock != 0 && rawblocklen != 0 )
        memcpy(block->rawblock,rawblock,rawblocklen);
    SaM(&blockhash,(uint8_t *)block,blocksize,0,0);
    block->hash = blockhash;
    //calc_SaM(&block->blocksig,blockhash.bytes,sizeof(blockhash),nn->broadcast.shared_curve25519.bytes,sizeof(bits256),SAM_MAGIC_NUMBER);
    block->sig.txid = nn->nodeid;
    return(block);
}

int32_t is_newhwm_block(struct crypto777_block **blocks,struct crypto777_block *block)
{
    if ( blocks[block->blocknum] == 0 || block->gen.metric > blocks[block->blocknum]->gen.metric )
        return(1);
    else return(0);
}

void update_block(struct consensus_model *model,struct crypto777_node *nn,struct crypto777_block **blocks,uint32_t blocknum,struct crypto777_block *block,uint32_t mytimestamp)
{
    int32_t lookback;
    if ( is_newhwm_block(blocks,block) != 0 )
    {
        if ( blocks[blocknum] != 0 )
            free(blocks[blocknum]);
        blocks[blocknum] = block;
        if ( blocknum > nn->blocknum )
            nn->blocknum = blocknum;
        if ( blocknum == (nn->lastblocknum + 1) || blocknum < nn->lastblocknum )
        {
            lookback = blocknum - 5;
            if ( lookback < 0 )
                lookback = 0;
            nn->lastblocknum2 = blocks[lookback]->blocknum, nn->blocktxid2 = blocks[lookback]->hash.txid;
            nn->lastblocknum = blocknum, nn->blocktxid = block->hash.txid;
        }
        nn->lastblocktimestamp = mytimestamp;
        update_peers(nn,block,model->peermetrics,nn->numpeers,model->packet_leverage);
    }
}

struct crypto777_block *generate_block(struct consensus_model *model,struct crypto777_node *nn,uint8_t *msg,int32_t len,uint32_t blocknum,uint32_t timestamp)
{
    struct crypto777_generator gen;
    struct crypto777_block *prev,*block = 0;
    if ( (prev= model->blocks[blocknum-1]) != 0 )
    {
        calc_generator(&gen,calc_stake(nn,model),nn->email,timestamp,prev->gen.metric,prev->gen.hash,model->POW_DIFF);
        //printf("node.%d GENERATE.%d %.8f (%.8f + prev %.8f)\n",nn->nodeid,blocknum,dstr(gen.metric),dstr(gen.hit),dstr(prev->gen.metric));
        block = sign_block(nn,&gen,msg,len,blocknum,timestamp);
        update_block(model,nn,model->blocks,blocknum,block,timestamp);
    }
    return(block);
}

int32_t validate_block(struct crypto777_block *block)
{
    if ( block->sig.txid < NETWORKSIZE && strcmp(Network[block->sig.txid]->email,block->gen.email) == 0 )
        return(0);
    printf("block->sig.txid %llu != block->gen.email.(%s)\n",(long long)block->sig.txid,block->gen.email);
    return(-1);
}

struct crypto777_block *parse_block777(struct consensus_model *model,struct crypto777_node *nn,struct crypto777_block *block,int32_t len,uint32_t peerid)
{
    char *blockstr = (char *)block->rawblock;
    uint64_t metric;
    int32_t bestacct;
    cJSON *json;
    uint32_t timestamp,blocknum = 0;
    if ( (bestacct= validate_block(block)) >= 0 && peerid < nn->numpeers && (json= cJSON_Parse(blockstr)) != 0 )
    {
        blocknum = juint(json,"blocknum");
        timestamp = juint(json,"timestamp");
        metric = get_API_nxt64bits(cJSON_GetObjectItem(json,"metric"));
        model->peermetrics[blocknum][peerid] = metric;
        model->peerblocknum[peerid] = blocknum;
        free_json(json);
        if ( block->blocknum == blocknum ) //  block->gen.metric == metric &&
            return(block);
        else printf("error parse.(%s) metric %.8f vs %.8f blocknum %u vs %u\n",blockstr,dstr(block->gen.metric),dstr(metric),block->blocknum,blocknum);
    }
    else
    {
        int i;
        for (i=0; i<len; i++)
            printf("%02x ",((uint8_t *)block)[i]);
        printf("block.%d (%s) peerid.%d numpeers.%d\n",block->blocknum,blockstr,peerid,nn->numpeers);
    }
    return(0);
}

cJSON *generators_json(struct consensus_model *model,struct crypto777_node *nn,uint64_t sortbuf[MAXPEERS+1][4],int32_t n,uint32_t blocknum,uint64_t mymetric)
{
    int32_t i;
    char numstr[64];
    cJSON *array,*json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"cmd",cJSON_CreateString("generators"));
    cJSON_AddItemToObject(json,"blocknum",cJSON_CreateNumber(blocknum));
    sprintf(numstr,"%llu",(long long)mymetric), cJSON_AddItemToObject(json,"metric",cJSON_CreateString(numstr));
    array = cJSON_CreateArray();
    for (i=0; i<n; i++)
        cJSON_AddItemToArray(array,cJSON_CreateNumber(sortbuf[i][1]));
    cJSON_AddItemToObject(json,"accts",array);
    array = cJSON_CreateArray();
    cJSON_AddItemToArray(array,cJSON_CreateNumber(dstr(sortbuf[0][0])));
    for (i=1; i<8&&blocknum>i; i++)
        if ( (blocknum - i) > 0 && model->blocks[blocknum-i] != 0 )
            cJSON_AddItemToArray(array,cJSON_CreateNumber(dstr(model->blocks[blocknum-i]->gen.metric)));
    cJSON_AddItemToObject(json,"metrics",array);
    return(json);
}

int32_t broadcast_if_generator(struct consensus_model *model,struct crypto777_node *nn,uint32_t blocknum)
{
    uint64_t sortbuf[MAXPEERS+1][4]; uint8_t msg[8192];
    struct crypto777_generator gen;
    struct crypto777_block *prev,*block;
    char *blockstr = 0;
    uint32_t mytimestamp;
    int32_t i,n,len,lag;
    cJSON *json;
    if ( (prev= model->blocks[blocknum-1]) == 0 )
        return(-1);
    if ( (n= calc_generators(nn,sortbuf,model,blocknum)) > 0 )
    {
        mytimestamp = (uint32_t)time(NULL);
        calc_generator(&gen,calc_stake(nn,model),nn->email,mytimestamp,prev->gen.metric,prev->gen.hash,model->POW_DIFF);
        if ( (json= generators_json(model,nn,sortbuf,n,blocknum,gen.metric)) != 0 )
        {
            blockstr = cJSON_Print(json);
            free_json(json);
            _stripwhite(blockstr,' ');
            strcpy((char *)msg,blockstr);
            //printf("node.%d (%s)\n",nn->nodeid,blockstr);
            free(blockstr);
        }
        len = (int32_t)strlen((char *)msg) + 1;
        lag = (mytimestamp - prev->timestamp);
        for (i=0; i<lag&&i<n; i++)
        {
            if ( sortbuf[i][1] == nn->nodeid )
            {
                if ( 0 && nn->nodeid == 0 )
                    printf("msg%-4d: (%s).%d\n",nn->nodeid,(char *)msg,len);
                block = generate_block(model,nn,msg,len,blocknum,mytimestamp);
                crypto777_broadcast(nn,(uint8_t *)block,block->blocksize,model->packet_leverage,0);
                nn->lastblocktimestamp = mytimestamp;
                Generator++;
                return(1);
            }
        }
    }
    return(0);
}

void processrecv(struct consensus_model *model,struct crypto777_node *nn,struct crypto777_packet *recv,uint32_t timestamp,int32_t peeri)
{
    struct crypto777_block *block,*ptr;
    if ( recv != 0 )
    {
        nn->transport.numrecv++;
        //SETBIT(Recvmasks[nn->nodeid],recv->nodeid);
        if ( (block= parse_block777(model,nn,(struct crypto777_block *)&recv->recvbuf[sizeof(struct SaMhdr)],recv->recvlen - sizeof(struct SaMhdr),peeri)) != 0 )
        {
            if ( is_newhwm_block(model->blocks,block) != 0 )
            {
                ptr = calloc(1,block->blocksize);
                memcpy(ptr,block,block->blocksize);
                update_block(model,nn,model->blocks,block->blocknum,ptr,timestamp);
                Receiver++;
                if ( nn->peers[peeri] == 0 || recv->nodeid != nn->peers[peeri]->nodeid )
                    printf("recv newhwm for block.%u sig.%llu gennode.%d from peer.%d %d\n",block->blocknum,(long long)block->sig.txid,recv->nodeid,peeri,nn->peers[peeri]->nodeid);
            }
        } else printf("node.%d error parsing block\n",nn->nodeid);
    }
}

struct crypto777_packet *nanorecv(int32_t *lenp,struct crypto777_node *nn,int32_t sock,int32_t peerid)
{
    uint32_t timestamp = 0;
    struct crypto777_packet *recv;
    uint8_t *buf = 0;
    *lenp = nn_recv(sock,&buf,NN_MSG,0);
    if ( *lenp >= 0 )
    {
        numpackets++, Totalrecv++;
        nn->recvcount++;
        if ( (recv= crypto777_packet(&nn->broadcast,&timestamp,(uint8_t *)buf,*lenp,0,0,0)) != 0 )
        {
            //printf("(%s): broadcast RECEIVED %d | (%s) recv.%d total.%ld\n",nn->ip_port,*lenp,&buf[sizeof(struct SaMhdr)],recv->recvlen,Totalrecv);
            recv->nodeid = peerid;
            return(recv);
        }
        else printf("ERROR: (%s): recv.%d total.%ld\n",nn->ip_port,*lenp,Totalrecv);
    }
    return(0);
}

int32_t process_recvs(struct consensus_model *model,struct crypto777_node *nn,uint32_t timestamp)
{
    struct crypto777_packet *recv = 0;
    int32_t peerid,i,len,flag = 0;
    if ( (recv= queue_dequeue(&nn->recvQ,0)) != 0 )
        processrecv(model,nn,recv,timestamp,-1), free(recv), flag++;
    else
    {
        for (i=0; i<nn->transport.numpeers; i++)
        {
            peerid = nn->peers[i] !=0 ? nn->peers[i]->nodeid : -1;
            if ( (recv= nanorecv(&len,nn,nn->transport.peers[i].subsock,peerid)) != 0 )
                processrecv(model,nn,recv,timestamp,i), free(recv), flag++;
            if ( i < nn->transport.numpairs && (recv= nanorecv(&len,nn,nn->transport.insocks[i],peerid)) != 0 )
                processrecv(model,nn,recv,timestamp,i), free(recv), flag++;
        }
    }
    return(flag);
}

void *crypto777_loop(void *args)
{
    struct crypto777_node *nn = (struct crypto777_node *)args;
    uint32_t blocknum,timestamp,i,flag,lag = 0;
    struct crypto777_block *block;
    struct consensus_model *model = calloc(1,sizeof(*model));
    memcpy(model,MODEL,sizeof(*model));
    select_peers(nn);
    //printf("crypto777_loop.(%s) nodeid.%d\n",nn->email,nn->nodeid);
    nn->lastblocknum = 0;
    nn->first_timestamp = nn->lastblocktimestamp = (uint32_t)time(NULL);
    while ( 1 )
    {
        timestamp = (uint32_t)time(NULL);
        if ( process_recvs(model,nn,timestamp) == 0 )
            msleep((500./NETWORKSIZE) * (double)(rand() % NETWORKSIZE));
        //printf("node.%d timestamp.%u\n",nn->nodeid,timestamp);
        for (i=flag=lag=0; i<nn->numpeers; i++)
            if ( model->peerblocknum[i] > 1  && model->peerblocknum[i] < nn->lastblocknum-2 && (block= model->blocks[model->peerblocknum[i]+1]) != 0 )
            {
                crypto777_broadcast(nn,(uint8_t *)block,block->blocksize,model->packet_leverage,0);
                msleep((100./NETWORKSIZE) * (double)(rand() % NETWORKSIZE));
                if ( nn->lastblocknum-model->peerblocknum[i] > lag )
                    lag = nn->lastblocknum-model->peerblocknum[i];
                //printf("node%d: issue prev.%u block.%u lag.%d max.%d\n",nn->nodeid,model->peerblocknum[i],nn->lastblocknum,nn->lastblocknum-model->peerblocknum[i],lag);
                flag++;
            }
        blocknum = (timestamp - nn->first_timestamp) / model->blockduration;
        if ( blocknum > nn->lastblocknum && timestamp > nn->lastblocktimestamp )
        {
       //printf("node.%d blocknum.%d timestamp.%d 1st %u, duration.%d\n",nn->nodeid,blocknum,timestamp,nn->first_timestamp,model->blockduration);
            broadcast_if_generator(model,nn,nn->lastblocknum+1);
            msleep((100./NETWORKSIZE) * (double)(rand() % NETWORKSIZE));
        } //else printf("node.%d blocknum.%d timestamp.%d 1st %u, duration.%d\n",nn->nodeid,blocknum,timestamp,nn->first_timestamp,model->blockduration);
    }
    return(0);
}

struct consensus_model *create_genesis(uint64_t PoW_diff,int32_t blockduration,int32_t leverage,uint64_t *stakes,int32_t num)
{
    struct consensus_model *mp = calloc(1,sizeof(*mp));
    struct crypto777_generator gen;
    struct crypto777_ledger *ledger = 0;
    uint32_t i,blocknum = 0;
    bits384 prev;
    mp->genesis_timestamp = (uint32_t)time(NULL);
    if ( stakes != 0 )
    {
        ledger = calloc(1,sizeof(*ledger));
        ledger->num = num, ledger->timestamp = mp->genesis_timestamp;
        strncpy(ledger->name,"stakeledger",sizeof(ledger->name)-1);
        for (i=0; i<num; i++)
            ledger->vals[i] = stakes[i];
        mp->stakeledger = *ledger;
        free(ledger);
    } else mp->stakeid = MODEL_ROUNDROBIN;
    memset(prev.bytes,0,sizeof(prev));
    mp->POW_DIFF = PoW_diff, mp->blockduration = blockduration, mp->packet_leverage = leverage;
    memset(&prev,0,sizeof(prev));
    calc_generator(&gen,calc_stake(0,mp),"",0,0,prev,mp->POW_DIFF);
    mp->blocks[0] = sign_block(Network[0],&gen,0,0,blocknum,mp->genesis_timestamp);
    return(mp);
}
#endif


