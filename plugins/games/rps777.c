//
//  rps777.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "rps"
#define PLUGNAME(NAME) rps ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define DEFINES_ONLY
#include "includes/cJSON.h"
#include "plugin777.c"
#include "kv777.c"
#include "txnet777.c"
#undef DEFINES_ONLY

#define DEFAULT_RPSPORT 8193
#define GATEWAY_ADDRESS "NXT-ZGGJ-5G8H-K24Y-3BD8S"

struct rps_info
{
    char depositaddr[64],name[64];
    int32_t readyflag; struct txnet777 TXNET;
};
STRUCTNAME RPS;

int32_t rps_idle(struct plugin_info *plugin)
{
    return(0);
}

void *RPS_balanceiterator(struct kv777 *kv,void *_ptr,void *key,int32_t keysize,void *value,int32_t valuesize)
{
    int32_t i,flag = 0,polarity = 0; struct txnet777_tx *txnet,*args = _ptr; //char buf[1024];
    txnet = value;
    for (i=0; i<txnet->in.numoutputs; i++)
    {
        if ( args->in.senderbits == txnet->in.senderbits )
            polarity--, flag++;
        if ( args->in.senderbits == txnet->out[i].destbits )
            polarity++, flag++;
        if ( flag != 0 )
        {
            args->H.sig.txid += (polarity * txnet->out[i].destamount);
            //jaddistr((cJSON *)args->in.revealed.txid,txnet777_str(buf,txnet));
        }
    }
    return(0);
}

char *balance(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    char numstr[64],*arraystr; struct txnet777_tx args; cJSON *argjson;
    memset(&args,0,sizeof(args));
    argjson = cJSON_CreateArray();
    memcpy(&args.H.sig.txid,&argjson,sizeof(argjson));
    args.in.senderbits = conv_acctstr(sender);
    kv777_iterate(RPS.TXNET.transactions,&args,0,RPS_balanceiterator);
    arraystr = jprint(argjson,1);
    sprintf(retbuf,"{\"result\":\"success\",\"account\":\"%s\",\"balance\":\"%s\",\"transactions\":%s}",sender,numstr,arraystr);
    return(0);
}

void *balanceiterator(struct kv777 *kv,void *_ptr,void *key,int32_t keysize,void *value,int32_t valuesize)
{
    int32_t i; struct txnet777_tx *txnet,*args = _ptr;
    txnet = value;
    for (i=0; i<txnet->in.numoutputs; i++)
    {
        if ( args->in.senderbits == txnet->in.senderbits )
            args->in.totalcost -= txnet->in.totalcost;
        if ( args->in.senderbits == txnet->out[i].destbits )
            args->in.totalcost += txnet->in.totalcost;
    }
    return(0);
}

uint64_t calc_rpsbits(char *move)
{
    uint64_t rpsbits;
    if ( (rpsbits= stringbits(move)) == stringbits("rock") || rpsbits == stringbits("paper") || rpsbits == stringbits("scissors") )
        return(rpsbits);
    return(0);
}

char *bet(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    struct txnet777_tx tx; int64_t balance,senderbits,betamount,destbits;
    if ( (betamount= jdouble(json,"bet") * SATOSHIDEN) > 0 && (destbits= calc_rpsbits(jstr(json,"move"))) != 0 )
    {
        memset(&tx,0,sizeof(tx));
        tx.in.senderbits = senderbits = conv_acctstr(sender);
        kv777_iterate(RPS.TXNET.transactions,&tx,0,balanceiterator);
        if ( (balance= tx.out[0].destamount) >= betamount )
        {
            memset(&tx,0,sizeof(tx));
            tx.out[0].destamount = betamount, tx.out[0].destbits = destbits;
            txnet777_signtx(&RPS.TXNET,0,&tx,sizeof(tx),RPS.name,senderbits,(uint32_t)time(NULL));
            txnet777_broadcast(&RPS.TXNET,&tx);
        }
    } else sprintf(retbuf,"{\"error\":\"illegal bet\",\"betamount\":\"%llu\"}",(long long)betamount);
    return(0);
}

char *status(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    return(0);
}

int32_t deposit_func(uint64_t txid,uint32_t blocknum,uint32_t timestamp,uint64_t senderbits,uint64_t amount,uint64_t destbits)
{
    struct txnet777_tx txnet; int32_t status = 777;
    memset(&txnet,0,sizeof(txnet));
    //txnet.txidbits = txid, txnet.blocknum = blocknum, txnet.timestamp = timestamp;
    //txnet.senderbits = senderbits, txnet.amount = amount, txnet.destbits = destbits;
    //kv777_write(RPS.TXNET.transactions,&txnet,sizeof(txnet),&status,sizeof(status));
    return(status);
}

char *deposit(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    //cashier777_update(deposit_func,RPS.depositaddr,0,0);
    return(0);
}

char *sendcoin(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    //struct balanceargs item; int32_t status = 777;
    //memset(&item,0,sizeof(item));
    //item.txidbits = txid, item.nxt64bits = senderbits, item.amount = amount;
    //dKV777_write(RPS.dKV,RPS.coins,senderbits,&item,sizeof(item),&status,sizeof(status));
    return(0);
}

char *withdraw(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid)
{
    // same as send but xfer to NXT
    return(0);
}

typedef char *(*rpsfunc)(char *retbuf,long maxlen,struct txnet777 *TXNET,cJSON *json,char *jsonstr,char *tokenstr,char *forwarder,char *sender,int32_t valid);
#define RPS_COMMANDS "ping", "endpoint", "sendcoin", "balance", "deposit", "withdraw", "bet", "status"
rpsfunc PLUGNAME(_functions)[] = { txnet777_ping, txnet777_endpoint, sendcoin, balance, deposit, withdraw, bet, status };

char *PLUGNAME(_methods)[] = { RPS_COMMANDS };
char *PLUGNAME(_pubmethods)[] = { "status", "ping" };
char *PLUGNAME(_authmethods)[] = { "status", "ping" };

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    return(disableflags); // set bits corresponding to array position in _methods[]
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr,*depositaddr,*retstr = 0; int32_t i; double pingmillis = 60000;
    retbuf[0] = 0;
    printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        RPS.readyflag = 1;
        plugin->allowremote = 1;
        copy_cJSON(RPS.name,jobj(json,"name"));
        txnet777_init(&RPS.TXNET,json,"rps","RPS",RPS.name,pingmillis);
        strcpy(plugin->NXTADDR,RPS.TXNET.ACCT.NXTADDR);
        if ( (depositaddr= jstr(json,"depositaddr")) == 0 )
            depositaddr = GATEWAY_ADDRESS;
        printf("fixed.%ld malleable.%ld tx.%ld\n",sizeof(struct txnet777_input),sizeof(struct txnet777_output),sizeof(struct txnet777));
        sprintf(retbuf,"{\"result\":\"initialized RPS\",\"pluginNXT\":\"%s\",\"serviceNXT\":\"%s\",\"depositaddr\":\"%s\"}",plugin->NXTADDR,plugin->SERVICENXT,depositaddr);
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        methodstr = jstr(json,"method");
        resultstr = jstr(json,"result");
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        else if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        for (i=0; i<sizeof(PLUGNAME(_methods))/sizeof(*PLUGNAME(_methods)); i++)
            if ( strcmp(PLUGNAME(_methods)[i],methodstr) == 0 )
                retstr = (*PLUGNAME(_functions)[i])(retbuf,maxlen,&RPS.TXNET,json,jsonstr,tokenstr,forwarder,sender,valid);
    }
    return(plugin_copyretstr(retbuf,maxlen,retstr));
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}
#include "plugin777.c"
