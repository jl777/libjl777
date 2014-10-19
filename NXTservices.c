 
// Created by jl777, Feb-Mar 2014
// MIT License
//

int32_t Numtransfers,Historical_done;
struct NXThandler_info *Global_mp;
uint64_t *Assetlist;

int32_t is_special_addr(char *NXTaddr)
{
    extern int32_t is_gateway_addr(char *);
    if ( strcmp(NXTaddr,NXTISSUERACCT) == 0 || is_gateway_addr(NXTaddr) != 0 )
        return(1);
    else return(0);
}

int32_t validate_nxtaddr(char *nxtaddr)
{
    // make sure it has publickey
    int32_t n = (int32_t)strlen(nxtaddr);
    while ( n > 10 && (nxtaddr[n-1] == '\n' || nxtaddr[n-1] == '\r') )
        n--;
    if ( n < 1 )
        return(-1);
    return(0);
}

void *register_NXT_handler(char *name,struct NXThandler_info *mp,int32_t type,int32_t subtype,NXT_handler handler,uint32_t AMsigfilter,int32_t priority,char **assetlist,char **whitelist)
{
    static struct NXT_protocol_parms PARMS;
    struct NXT_protocol *p;
    printf("register size.%ld\n",sizeof(*p));
    p = calloc(1,sizeof(*p));
    safecopy(p->name,name,sizeof(p->name));
    p->type = type; p->subtype = subtype;
    p->AMsigfilter = AMsigfilter;
    p->priority = priority;
    p->assetlist = assetlist; p->whitelist = whitelist;
    p->NXT_handler = handler;
    printf("register %p\n",p);
    memset(&PARMS,0,sizeof(PARMS));
    PARMS.mode = NXTPROTOCOL_INIT;
    if ( Num_NXThandlers < (int32_t)(sizeof(NXThandlers)/sizeof(*NXThandlers)) )
    {
        printf("calling handlerinit.%s %p size.%d\n",name,handler,Num_NXThandlers);
        p->handlerdata = (*handler)(mp,&PARMS,0,0);
        NXThandlers[Num_NXThandlers++] = p;
        printf("back handlerinit.%s size.%d\n",name,Num_NXThandlers);
    }
    printf("done register.%p\n",p);
    return(p->handlerdata);
}

void call_handlers(struct NXThandler_info *mp,int32_t mode,int32_t height)
{
    int32_t i;
    struct NXT_protocol_parms PARMS;
    struct NXT_protocol *p;
    memset(&PARMS,0,sizeof(PARMS));
    PARMS.mode = mode;
    for (i=0; i<Num_NXThandlers; i++)
    {
        if ( (p= NXThandlers[i]) != 0 )
            (*p->NXT_handler)(mp,&PARMS,p->handlerdata,height);
    }
}

struct NXT_protocol *get_NXTprotocol(char *name)
{
    static struct NXT_protocol P;
    int32_t i;
    struct NXT_protocol *p;
    if ( name[0] == '/' )
        name++;
    if ( strcmp("NXTservices",name) == 0 )
    {
        strcpy(P.name,name);
        return(&P);
    }
    for (i=0; i<Num_NXThandlers; i++)
    {
        if ( (p= NXThandlers[i]) != 0 && strcmp(p->name,name) == 0 )
            return(p);
    }
    return(0);
}

char *NXTprotocol_json(cJSON *argjson)
{
    void ensure_depositaddrs(char *NXTaddr);
    int32_t height= -1,i,flag,n=0;
    char *retstr;
    //struct NXThandler_info *mp = Global_mp;
    char numstr[1024],chatcmd[1024];//,oldsecret[1024],newsecret[104];
    struct NXT_protocol *p;
    cJSON *json,*array,*obj,*item;
    flag = 0;
    if ( argjson != 0 )
    {
        height = (int32_t)get_cJSON_int(argjson,"height");
        /*if ( extract_cJSON_str(newsecret,sizeof(newsecret),argjson,"newsecret") > 0 )
        {
            flag = 1;
            if ( extract_cJSON_str(oldsecret,sizeof(oldsecret),argjson,"oldsecret") > 0 && strcmp(mp->NXTACCTSECRET,oldsecret) == 0 )
            {
                printf("set new acct secret to (%s)\n",newsecret);
                strcpy(mp->NXTACCTSECRET,newsecret);
                mp->nxt64bits = issue_getAccountId(mp->curl_handle,mp->NXTACCTSECRET);
                expand_nxt64bits(mp->NXTADDR,mp->nxt64bits);
                printf("new acct# NXT.%s\n",mp->NXTADDR);
                memset(mp->dispname,0,sizeof(mp->dispname));
                memset(PC_USERNAME,0,sizeof(PC_USERNAME));
                memset(mp->groupname,0,sizeof(mp->groupname));
                if ( mp->accountjson != 0 )
                    free_json(mp->accountjson);
                mp->accountjson = issue_getAccountInfo(mp->curl_handle,&mp->acctbalance,mp->dispname,PC_USERNAME,mp->NXTADDR,mp->groupname);
                printf("(%s) PC_USERNAME.(%s) groupname.(%s) NXT.(%s) [%s] balance %.8f\n",mp->dispname,PC_USERNAME,mp->groupname,mp->NXTADDR,mp->NXTACCTSECRET,dstr(mp->acctbalance));
                //ensure_depositaddrs(Global_mp->NXTADDR);
            }
        }
        else*/ if ( extract_cJSON_str(chatcmd,sizeof(chatcmd),argjson,"chat") > 0 )
        {
            /*if ( Global_mp->Punch_connect_id[0] != 0 )
            {
                flag = 2;
                //terminal_io(chatcmd,&Global_mp->Punch_tcp,Global_mp->Punch_servername,Global_mp->Punch_connect_id);
            }*/
        }
    }
    json = cJSON_CreateObject();
    array = cJSON_CreateArray();
    for (i=0; i<Num_NXThandlers; i++)
    {
        if ( (p= NXThandlers[i]) != 0 )
        {
            n++;
            item = cJSON_CreateObject();
            obj = cJSON_CreateString(p->name); cJSON_AddItemToObject(item,"name",obj);
            obj = cJSON_CreateNumber(p->type); cJSON_AddItemToObject(item,"type",obj);
            obj = cJSON_CreateNumber(p->subtype); cJSON_AddItemToObject(item,"subtype",obj);
            obj = cJSON_CreateNumber(p->priority); cJSON_AddItemToObject(item,"priority",obj);
            sprintf(numstr,"0x%08x",p->AMsigfilter);
            obj = cJSON_CreateString(numstr); cJSON_AddItemToObject(item,"AMsig",obj);
            if ( p->assetlist != 0 )
            {
                obj = gen_list_json(p->assetlist);
                cJSON_AddItemToObject(item,"assetlist",obj);
            }
            if ( p->whitelist != 0 )
            {
                obj = gen_list_json(p->whitelist);
                cJSON_AddItemToObject(item,"whitelist",obj);
            }
            cJSON_AddItemToArray(array,item);
        }
    }
    obj = cJSON_CreateNumber(n); cJSON_AddItemToObject(json,"numhandlers",obj);
    cJSON_AddItemToObject(json,"handlers",array);
    if ( flag == 1 )
    {
        //cJSON_AddItemToObject(json,"NXTaddr",cJSON_CreateString(Global_mp->NXTADDR));
        //cJSON_AddItemToObject(json,"secret",cJSON_CreateString(Global_mp->NXTACCTSECRET));
    }
    /*else if ( flag == 2 )
    {
        cJSON_AddItemToObject(json,"group",cJSON_CreateString(Global_mp->groupname));
        cJSON_AddItemToObject(json,"handle",cJSON_CreateString(Global_mp->dispname));
    }*/
    //cJSON_AddItemToObject(json,"API",cJSON_CreateString(Global_mp->NXTURL));
    if ( height >= 0 && height < Global_mp->numblocks )
    {
        cJSON_AddItemToObject(json,"height",cJSON_CreateNumber(height));
        cJSON_AddItemToObject(json,"block",cJSON_CreateString(nxt64str(Global_mp->blocks[height])));
        cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(Global_mp->timestamps[height]));
    }
    retstr = cJSON_Print(json);
    free_json(json);
    return(retstr);
}

/*
char *NXTprotocol_json_handler(struct NXT_protocol *p,char *argstr)
{
    long len;
    cJSON *json;
    char *retjsontxt = 0;
    struct NXT_protocol_parms PARMS;
    if ( argstr != 0 )
    {
        convert_percent22(argstr);
        json = cJSON_Parse(argstr);
        printf("NXTprotocol_json_handler.(%s)\n",argstr);
        memset(&PARMS,0,sizeof(PARMS));
        PARMS.mode = NXTPROTOCOL_WEBJSON;
        PARMS.argjson = json;
        PARMS.argstr = argstr;
        if ( strcmp(p->name,"NXTservices") == 0 )
            retjsontxt = NXTprotocol_json(json);
        else retjsontxt = (*p->NXT_handler)(Global_mp,&PARMS,p->handlerdata,0);
        //printf("retjsontxt.%p\n",retjsontxt);
        if ( PARMS.argjson != 0 )
            free_json(PARMS.argjson);
    }
    else if ( strcmp(p->name,"NXTservices") == 0 )
        retjsontxt = NXTprotocol_json(0);
    if ( retjsontxt != 0 )
    {
        //printf("got.(%s)\n",retjsontxt);
        len = strlen(retjsontxt);
        if ( len > p->retjsonsize )
            p->retjsontxt = realloc(p->retjsontxt,len+1);
        strcpy(p->retjsontxt,retjsontxt);
        free(retjsontxt);
        retjsontxt = p->retjsontxt;
    }
    else return("{\"result\":null}");
    return(retjsontxt);
}
*/

int32_t process_NXT_event(struct NXThandler_info *mp,int32_t height,char *txid,int64_t type,int64_t subtype,struct NXT_AMhdr *AMhdr,char *sender,char *receiver,char *assetid,int64_t assetoshis,char *comment,cJSON *json)
{
    int32_t i,count,highest_priority;
    NXT_handler NXT_handler = 0;
    void *handlerdata = 0;
    struct NXT_protocol *p;
    struct NXT_protocol_parms PARMS;
    count = 0;
    highest_priority = -1;
    memset(&PARMS,0,sizeof(PARMS));
    if ( Num_NXThandlers == 0 )
        printf("WARNING: process_NXT_event Num_NXThandlers.%d\n",Num_NXThandlers);
    PARMS.height = height;
    for (i=0; i<Num_NXThandlers; i++)
    {
        if ( (p= NXThandlers[i]) != 0 )
        {
            if ( AMhdr != 0 || ((p->type < 0 || p->type == type) && (p->subtype < 0 || p->subtype == subtype)) )
            {
                if ( AMhdr != 0 )
                {
                    if ( p->AMsigfilter != 0 && p->AMsigfilter != AMhdr->sig )
                        continue;
                }
                else if ( p->assetlist != 0 && assetid != 0 && listcmp(p->assetlist,assetid) != 0 )
                    continue;
                if ( strcmp(sender,GENESISACCT) == 0 || strcmp(receiver,GENESISACCT) == 0 ||
                     (AMhdr == 0 || p->whitelist == 0 || listcmp(p->whitelist,sender) == 0) ||
                     (AMhdr != 0 && (cmp_nxt64bits(sender,AMhdr->nxt64bits) == 0 || (is_special_addr(sender) != 0 && cmp_nxt64bits(receiver,AMhdr->nxt64bits) == 0))) )
                {
                    //if ( p->priority > highest_priority )
                    {
                        count = 1;
                        highest_priority = p->priority;
                        NXT_handler = p->NXT_handler;
                        handlerdata = p->handlerdata;
                        PARMS.argjson = json; PARMS.txid = txid; PARMS.sender = sender; PARMS.receiver = receiver;
                        PARMS.type = (int32_t)type; PARMS.subtype = (int32_t)subtype; PARMS.priority = highest_priority; //PARMS.histflag = histmode;
                        if ( AMhdr != 0 )
                        {
                            PARMS.mode = NXTPROTOCOL_AMTXID;
                            PARMS.AMptr = AMhdr;
/*#define GATEWAY_SIG 0xbadbeefa
                            if ( p->AMsigfilter == GATEWAY_SIG )
                            {
                                char *tmpstr = 0;
                                if ( json != 0 )
                                    tmpstr = cJSON_Print(json);
                                printf("MGW AMtxid.%s (%s)\n",txid,tmpstr!=0?tmpstr:"");
                                if ( tmpstr != 0 )
                                    free(tmpstr);
                            }*/
                        }
                        else
                        {
                            PARMS.mode = NXTPROTOCOL_TYPEMATCH;
                            PARMS.assetid = assetid;
                            PARMS.assetoshis = assetoshis;
                            PARMS.comment = comment;
                        }
                        (*NXT_handler)(mp,&PARMS,handlerdata,height);
                    }
                   // else if ( p->priority == highest_priority )
                   //     count++;
                }
                //printf("end iter\n");
            }
        }
    }
    if ( count > 1 )
        printf("WARNING: (%s) claimed %d times, priority.%d\n",cJSON_Print(json),count,highest_priority);  // this is bad, also leaks mem!
    /*if ( 0 && highest_priority >= 0 && NXT_handler != 0 )
    {
        //printf("count.%d handler.%p NXThandler_info_handler.%p AMhdr.%p\n",count,NXT_handler,multigateway_handler,AMhdr);
        PARMS.argjson = json; PARMS.txid = txid; PARMS.sender = sender; PARMS.receiver = receiver;
        PARMS.type = (int32_t)type; PARMS.subtype = (int32_t)subtype; PARMS.priority = highest_priority;// PARMS.histflag = histmode;
        if ( AMhdr != 0 )
        {
            PARMS.mode = NXTPROTOCOL_AMTXID;
            PARMS.AMptr = AMhdr;
        }
        else
        {
            PARMS.mode = NXTPROTOCOL_TYPEMATCH;
            PARMS.assetid = assetid;
            PARMS.assetoshis = assetoshis;
            PARMS.comment = comment;
        }
        (*NXT_handler)(mp,&PARMS,handlerdata);///histmode,txid,AMhdr,sender,receiver,assetid,assetoshis,comment,json);
    }*/
    return(count != 0);
}

void transfer_asset_balance(struct NXThandler_info *mp,struct NXT_assettxid *tp,uint64_t assetbits,char *sender,char *receiver,uint64_t quantity)
{
    int32_t createdflag,srcind,destind;
    struct NXT_acct *src,*dest;
    struct NXT_asset *ap;
    char assetid[1024];
    if ( sender == 0 || sender[0] == 0 || receiver == 0 || receiver[0] == 0 )
    {
        printf("illegal transfer asset (%s -> %s)\n",sender,receiver);
        return;
    }
    expand_nxt64bits(assetid,assetbits);
    //printf("transfer_asset_balance\n");
    ap = MTadd_hashtable(&createdflag,mp->NXTassets_tablep,assetid);
    if ( createdflag != 0 )
        printf("transfer_asset_balance: unexpected hashtable creation??\n");
    src = get_NXTacct(&createdflag,mp,sender);
    srcind = get_asset_in_acct(src,ap,1);
    dest = get_NXTacct(&createdflag,mp,receiver);
    destind = get_asset_in_acct(dest,ap,1);
    if ( is_special_addr(receiver) != 0 && tp->comment != 0 )
    {
        tp->completed = MGW_PENDING_WITHDRAW;
    }
    //else if ( is_special_addr(sender) != 0 )
    //    tp->completed = MGW_PENDING_DEPOSIT;
    if ( srcind >= 0 && destind >= 0 )
    {
       // if ( strcmp(GENESISACCT,sender) == 0 || src->quantities[srcind] >= quantity )
        {
            src->quantities[srcind] -= quantity;
            addto_account_txlist(src,srcind,tp);
        }
       // else if ( strcmp(EX_DIVIDEND_ASSETID,assetid) == 0 )
       //     printf("asset balance error! %s %lld < %lld\n",sender,(long long)src->quantities[srcind],(long long)quantity);
        dest->quantities[destind] += quantity;
        addto_account_txlist(dest,destind,tp);
    } else printf("error finding inds for assetid.%s %s.%d %s.%d\n",assetid,sender,srcind,receiver,destind);
}

int32_t process_NXTtransaction(struct NXThandler_info *mp,char *txid,int32_t height,int32_t timestamp)
{
    char cmd[4096],sender[1024],receiver[1024],assetid[MAX_NXTADDR_LEN],*jsonstr=0,*assetidstr,*commentstr = 0;
    cJSON *senderobj,*attachment,*message,*assetjson,*hashobj,*commentobj;
    unsigned char buf[4096];
    char AMstr[4096],comment[1024];//,descr[4096];
    struct NXT_AMhdr *hdr;
    struct NXT_guid *gp;
    struct NXT_asset *ap;
    struct NXT_assettxid *tp;
    int32_t createdflag,processed = 0;
    int64_t type,subtype,n,assetoshis = 0;
    union NXTtype retval;
    memset(assetid,0,sizeof(assetid));
    sprintf(cmd,"%s=getTransaction&transaction=%s",NXTSERVER,txid);
    //retval = extract_NXTfield(0,cmd,0,0);
    retval.json = 0;
    jsonstr = issue_curl(0,cmd);
    if ( jsonstr != 0 )
       retval.json = cJSON_Parse(jsonstr);
    if ( retval.json != 0 )
    {
        hdr = 0; assetidstr = 0;
        sender[0] = receiver[0] = 0;
        hashobj = cJSON_GetObjectItem(retval.json,"hash");
        if ( hashobj != 0 )
        {
            copy_cJSON((char *)buf,hashobj);
            //printf("guid add hash.%s %p %p\n",(char *)buf,mp->NXTguid_tablep,mp->NXTguid_tablep);
            gp = MTadd_hashtable(&createdflag,mp->NXTguid_tablep,(char *)buf);
            if ( createdflag != 0 )
                safecopy(gp->H.U.txid,txid,sizeof(gp->H.U.txid));
            else
            {
                printf("duplicate transaction hash??: %s already there, new tx.%s both hash.%s | Probably from history thread\n",gp->H.U.txid,txid,gp->guid);
                //mark_tx_hashconflict(mp,histmode,txid,gp);
                free_json(retval.json);
                return(0);
            }
        }
        type = get_cJSON_int(retval.json,"type");
        subtype = get_cJSON_int(retval.json,"subtype");

        senderobj = cJSON_GetObjectItem(retval.json,"sender");
        if ( senderobj == 0 )
            senderobj = cJSON_GetObjectItem(retval.json,"accountId");
        else if ( senderobj == 0 )
            senderobj = cJSON_GetObjectItem(retval.json,"account");
        add_NXT_acct(sender,mp,senderobj);
        add_NXT_acct(receiver,mp,cJSON_GetObjectItem(retval.json,"recipient"));
        attachment = cJSON_GetObjectItem(retval.json,"attachment");
        if ( type == 2 && subtype == 0 && strcmp(receiver,GENESISACCT) == 0 )//EX_DIVIDEND_DESCRIPTION != 0 )
            //if ( extract_cJSON_str(descr,sizeof(descr),attachment,"description") > 0 && strcmp(descr,EX_DIVIDEND_DESCRIPTION) == 0 )
        {
            //printf("GENESIS -> NXT.%s\n",sender);
            //expand_nxt64bits(assetid,calc_nxt64bits(EX_DIVIDEND_ASSETID));
            tp = add_NXT_assettxid(&ap,txid,mp,0,txid,timestamp);   // assetid is the txid of issue asset command
            if ( tp != 0 )
            {
                tp->timestamp = timestamp;
                tp->completed = 1;
                tp->senderbits = calc_nxt64bits(GENESISACCT);
                tp->receiverbits = calc_nxt64bits(sender);
                if ( attachment != 0 )
                    tp->quantity = get_cJSON_int(attachment,"quantityQNT");
                printf("%d) t%d >>>>>>>>>>>> AE transfer.%d.%d: GENESIS -> NXT.%s qty %.8f\n",Numtransfers,tp->timestamp,(int32_t)type,(int32_t)subtype,sender,dstr(tp->quantity));
                transfer_asset_balance(mp,tp,tp->assetbits,GENESISACCT,sender,tp->quantity);
            }
            else printf("%d) t%d >>>>>>>>>>>> AE transfer.%d.%d: GENESIS -> NXT.%s\n",Numtransfers,tp->timestamp,(int32_t)type,(int32_t)subtype,sender);
            Numtransfers++;
        }
        if ( attachment != 0 )
        {
            //printf("%s\n",cJSON_Print(retval.json));
            message = cJSON_GetObjectItem(attachment,"message");
            assetjson = cJSON_GetObjectItem(attachment,"asset");
            if ( message != 0 && type != 2 )
            {
                copy_cJSON(AMstr,message);
                //printf("AM message.(%s).%ld\n",AMstr,strlen(AMstr));
                n = strlen(AMstr);
                if ( (n&1) != 0 || n > 2000 )
                    printf("warning: odd message len?? %ld\n",(long)n);
                decode_hex((void *)buf,(int32_t)(n>>1),AMstr);
                hdr = (struct NXT_AMhdr *)buf;
                //printf("txid.%s NXT.%s -> NXT.%s (%s)\n",txid,sender,receiver,((struct json_AM *)buf)->jsonstr);
            }
            else if ( assetjson != 0 && type == 2 && subtype <= 1 )
            {
                tp = add_NXT_assettxid(&ap,assetid,mp,assetjson,txid,timestamp);
                if ( tp != 0 )
                {
                    //printf("type.%d subtype.%d tp.%p\n",(int32_t)type,(int32_t)subtype,tp);
                    if ( tp->comment != 0 )
                        free(tp->comment), tp->comment = 0;
                    tp->timestamp = timestamp;
                    commentobj = cJSON_GetObjectItem(attachment,"comment");
                    if ( commentobj == 0 )
                        commentobj = message;
                    copy_cJSON(comment,commentobj);
                    if ( comment[0] != 0 )
                        tp->comment = replace_backslashquotes(comment);
                    if ( tp != 0 && type == 2 )
                    {
                        if ( comment[0] != 0 )
                            commentstr = tp->comment = clonestr(comment);
                        tp->quantity = get_cJSON_int(attachment,"quantityQNT");
                        assetoshis = tp->quantity;
                        switch ( subtype )
                        {
                            case 0:
                                if ( strcmp(receiver,GENESISACCT) == 0 )
                                {
                                    tp->senderbits = calc_nxt64bits(GENESISACCT);
                                    tp->receiverbits = calc_nxt64bits(sender);
                                    tp->completed = 1;
                                    // if ( strcmp(assetid,EX_DIVIDEND_ASSETID) == 0 )
                                    printf("AE transfer: GENESIS -> NXT.%s volume %.8f\n",sender,dstr(tp->quantity));
                                    transfer_asset_balance(mp,tp,tp->assetbits,GENESISACCT,sender,tp->quantity);
                                }
                                else printf("non-GENESIS sender on Issuer Asset?\n");
                                break;
                            case 1:
                                tp->senderbits = calc_nxt64bits(sender);
                                tp->receiverbits = calc_nxt64bits(receiver);
                                if ( tp->comment != 0 && (commentobj= cJSON_Parse(tp->comment)) != 0 )
                                {
#ifdef INSIDE_MGW
                                    /*                               int32_t coinid;
                                     char cointxid[128];
                                     cJSON *cointxidobj;
                                     cointxidobj = cJSON_GetObjectItem(commentobj,"cointxid");
                                     if ( cointxidobj != 0 )
                                     {
                                     copy_cJSON(cointxid,cointxidobj);
                                     //printf("got comment.(%s) cointxidstr.(%s)\n",tp->comment,cointxid);
                                     if ( cointxid[0] != 0 )
                                     {
                                     tp->cointxid = clonestr(cointxid);
                                     //printf("set cointxid to (%s)\n",cointxid);
                                     }
                                     }
                                     else if ( strcmp(receiver,NXTISSUERACCT) == 0 )
                                     {
                                     int32_t conv_coinstr(char *);
                                     coinid = conv_coinstr(ap->name);
                                     cointxidobj = cJSON_GetObjectItem(commentobj,"redeem");
                                     copy_cJSON(cointxid,cointxidobj);
                                     printf("%s got comment.(%s) gotredeem.(%s) coinid.%d\n",ap->name,tp->comment,cointxid,coinid);
                                     if ( coinid >= 0 )
                                     {
                                     tp->redeemtxid = clonestr(txid);
                                     if ( ap != 0 )
                                     {
                                     void ensure_wp(int32_t coinid,uint64_t amount,char *NXTaddr,char *redeemtxid);
                                     ensure_wp(coinid,tp->quantity * ap->mult,sender,txid);
                                     }
                                     }
                                     }
                                     free_json(commentobj);*/
#endif
                                }
                                transfer_asset_balance(mp,tp,tp->assetbits,sender,receiver,tp->quantity);
                                break;
                            case 2:
                            case 3: // bids and asks, no indication they are filled at this point, so nothing to do
                                break;
                        }
                    }
                    //else add_NXT_assettxid(&ap,assetid,mp,assetjson,txid,timestamp);
                }
                assetidstr = assetid;
            }
        }
        //printf("call process event hdr.%p %x\n",hdr,hdr->sig);
        //printf("hist.%d before handler init_NXThashtables: %p %p %p %p\n",mp->histmode,mp->NXTguid_tablep,mp->NXTaccts_tablep,mp->NXTassets_tablep, mp->NXTasset_txids_tablep);
        processed += process_NXT_event(mp,height,txid,type,subtype,hdr,sender,receiver,assetidstr,assetoshis,commentstr,retval.json);
       // printf("hist.%d after handler init_NXThashtables: %p %p %p %p\n",mp->histmode,mp->NXTguid_tablep,mp->NXTaccts_tablep,mp->NXTassets_tablep, mp->NXTasset_txids_tablep);
        free_json(retval.json);
    }
    else printf("unexpected error iterating (%s) txid.(%s)\n",cmd,txid);
    if ( jsonstr != 0 )
        free(jsonstr);
    return(processed);
}

uint64_t find_quoting_NXTaddr(uint64_t txidbits)
{
    cJSON *json;
    uint64_t acctbits = 0;
    char txidstr[1024],numstr[1024],*jsonstr;
    expand_nxt64bits(txidstr,txidbits);
    jsonstr = issue_getTransaction(0,txidstr);
    if ( jsonstr != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( extract_cJSON_str(numstr,sizeof(numstr),json,"sender") > 0 )
            {
                acctbits = calc_nxt64bits(numstr);
                //printf("(%s -> %llu) ",numstr,(long long)acctbits);
            }
            else printf("find_quoting_NXTaddr: cant get acctbits.(%s)\n",jsonstr);
            free_json(json);
        }
        else printf("find_quoting_NXTaddr: cant parse.(%s)\n",jsonstr);
        free(jsonstr);
    } else printf("ERROR find_quoting_NXTaddr cant get txidstr.(%s)\n",txidstr);
    return(acctbits);
}

uint64_t update_assettxid_trade(struct NXT_trade *trade)
{
    uint64_t txidbits;
    struct NXT_asset *ap;
    struct NXT_assettxid *tp;
    char txid[1024],assetidstr[1024],sender[1024],receiver[1024];
    txidbits = trade->bidorder ^ trade->askorder;
    expand_nxt64bits(txid,txidbits);
    expand_nxt64bits(assetidstr,trade->assetid);
    tp = add_NXT_assettxid(&ap,assetidstr,Global_mp,0,txid,trade->timestamp);
    if ( tp != 0 )
    {
        tp->senderbits = find_quoting_NXTaddr(trade->askorder);
        tp->receiverbits = find_quoting_NXTaddr(trade->bidorder);
        tp->quantity = trade->quantity;
        tp->U.price = trade->price;
        tp->timestamp = trade->timestamp;
        tp->completed = 1;
       // if ( tp->senderbits != 0 && tp->receiverbits != 0 && tp->quantity != 0 && tp->price != 0 )
        {
            expand_nxt64bits(sender,tp->senderbits);
            expand_nxt64bits(receiver,tp->receiverbits);
            //if ( strcmp(assetidstr,EX_DIVIDEND_ASSETID) == 0 )
            //    printf("%d) t%d >>>>>>>>> %s AE trade: NXT.%s -> NXT.%s price %.8f volume %.8f %llx\n",ap->num,tp->timestamp,assetidstr,sender,receiver,dstr(tp->price),ap->mult*dstr(tp->quantity),(long long)txidbits);
            transfer_asset_balance(Global_mp,tp,tp->assetbits,sender,receiver,tp->quantity);
            return(txidbits);
        }
    } //else printf("error adding assettxid.(%s) txid %llx\n",assetidstr,txidbits);
    return(0);
}

uint64_t get_asset_balance(struct NXT_acct *np,char *refassetid)
{
    union NXTtype retval;
    uint64_t qty = 0;
    int32_t i,n;
    cJSON *obj,*item,*array;
    char buf[2048],assetid[64];
    sprintf(buf,"%s=getAccount&account=%s",_NXTSERVER,np->H.U.NXTaddr);
    retval = extract_NXTfield(0,0,buf,0,0);
    if ( retval.json != 0 )
    {
        array = cJSON_GetObjectItem(retval.json,"assetBalances");
        if ( is_cJSON_Array(array) != 0 )
        {
            n = cJSON_GetArraySize(array);
            for (i=0; i<n; i++)
            {
                qty = 0;
                item = cJSON_GetArrayItem(array,i);
                obj = cJSON_GetObjectItem(item,"asset");
                copy_cJSON(assetid,obj);
                //printf("i.%d of %d: %s(%s)\n",i,n,assetid,cJSON_Print(item));
                if ( strcmp(assetid,refassetid) == 0 )
                {
                    qty = get_cJSON_int(item,"balanceQNT");
                    break;
                }
            }
        }
        free_json(retval.json);
    }
    return(qty);
}

void **addto_listptr(int32_t *nump,void **list,void *ptr)
{
    int32_t i,n = *nump;
    for (i=0; i<n; i++)
    {
        if ( list[i] == ptr )
            return(list);
    }
    list = realloc(list,sizeof(*list) * (n+1));
    list[n++] = ptr;
    *nump = n;
    return(list);
}

struct NXT_acct **clear_workingvars(struct NXT_acct **accts,int32_t *nump,char *sender,char *receiver)
{
    int32_t createdflag;
    struct NXT_acct *seller,*buyer;
    if ( sender != 0 )
    {
        seller = get_NXTacct(&createdflag,Global_mp,sender);
        seller->quantity = seller->buysum = seller->buyqty = seller->sellsum = seller->sellqty = 0;
        if ( nump != 0 )
            accts = (struct NXT_acct **)addto_listptr(nump,(void **)accts,seller);
    }
    if ( receiver != 0 )
    {
        buyer = get_NXTacct(&createdflag,Global_mp,receiver);
        buyer->quantity = buyer->buysum = buyer->buyqty = buyer->sellsum = buyer->sellqty = 0;
        if ( nump != 0 )
            accts = (struct NXT_acct **)addto_listptr(nump,(void **)accts,buyer);

    }
    return(accts);
}

cJSON *update_workingvars(struct NXT_acct *seller,struct NXT_acct *buyer,struct NXT_assettxid *txid,uint64_t ap_mult)
{
    cJSON *json,*commentobj;
    double dir = 1.;
    char numstr[1024];
    json = cJSON_CreateObject();
    if ( seller != 0 )
        seller->quantity -= txid->quantity;
    if ( buyer != 0 )
        buyer->quantity += txid->quantity;
    if ( seller != 0 && buyer == 0 )
        dir = -1.;
    if ( txid->quantity != 0 && txid->U.price > 0 )
    {
        if ( buyer != 0 )
        {
            buyer->buyqty += txid->quantity;
            buyer->buysum += txid->quantity*txid->U.price;
        }
        if ( seller != 0 )
        {
            seller->sellqty += txid->quantity;
            seller->sellsum += txid->quantity*txid->U.price;
        }
        if ( buyer != 0 && seller != 0 )
        {
            cJSON_AddItemToObject(json,"seller",cJSON_CreateString(seller->H.U.NXTaddr));
            cJSON_AddItemToObject(json,"buyer",cJSON_CreateString(buyer->H.U.NXTaddr));
        }
        sprintf(numstr,"%.8f",dstr(txid->U.price));
        cJSON_AddItemToObject(json,"price",cJSON_CreateString(numstr));
    }
    else
    {
        //printf("buyer.%p seller.%p\n",buyer,seller);
        if ( buyer != 0 && seller != 0 )
        {
            if ( txid->comment == 0 )
                commentobj = 0;
            else commentobj = cJSON_Parse(txid->comment);
            if ( strcmp(seller->H.U.NXTaddr,NXTISSUERACCT) == 0 )
            {
                if ( txid->quantity != 0 )
                    cJSON_AddItemToObject(json,"MGW transfer",commentobj);
                else
                {
                    cJSON_AddItemToObject(json,"MGW deposit",commentobj);
                    cJSON_AddItemToObject(json,"assetoshis",cJSON_CreateNumber(txid->U.assetoshis));
                }
                if ( txid->cointxid != 0 )
                    cJSON_AddItemToObject(json,"MGW cointxid",cJSON_CreateString(txid->cointxid));
            }
            else if ( strcmp(buyer->H.U.NXTaddr,NXTISSUERACCT) == 0 || strcmp(buyer->H.U.NXTaddr,seller->H.U.NXTaddr) == 0 )
            {
                if ( txid->quantity != 0 )
                {
                    if ( txid->completed == 0 )
                        txid->completed = MGW_PENDING_WITHDRAW;
                    cJSON_AddItemToObject(json,"MGW withdraw",commentobj);
                }
                else
                {
                    cJSON_AddItemToObject(json,"seller",cJSON_CreateString(seller->H.U.NXTaddr));
                    cJSON_AddItemToObject(json,"buyer",cJSON_CreateString(buyer->H.U.NXTaddr));
                    if ( txid->cointxid != 0 )
                    {
                        cJSON_AddItemToObject(json,"MGW redeem",cJSON_CreateString(txid->redeemtxid));
                        cJSON_AddItemToObject(json,"cointxid",cJSON_CreateString(txid->cointxid));
                        sprintf(numstr,"%.8f",dstr(txid->U.price));
                        cJSON_AddItemToObject(json,"price",cJSON_CreateString(numstr));
                    }
                }
            }
        }
    }
    sprintf(numstr,"%.8f",dir * dstr(txid->quantity * ap_mult));
    cJSON_AddItemToObject(json,"qty",cJSON_CreateString(numstr));
    if ( txid->timestamp != 0 )
        cJSON_AddItemToObject(json,"timestamp",cJSON_CreateNumber(txid->timestamp));
    if ( txid->completed == MGW_PENDING_WITHDRAW )
    {
        cJSON_AddItemToObject(json,"redeemtxid",cJSON_CreateString(txid->H.U.txid));
        txid->redeemtxid = txid->H.U.txid;
    }
    else cJSON_AddItemToObject(json,"txid",cJSON_CreateString(txid->H.U.txid));
    cJSON_AddItemToObject(json,"completed",cJSON_CreateNumber(txid->completed));
    //printf("%d) %-12s t%-8d NXT.%-21s %16.8f -> NXT.%-21s %16.8f %16.8f @ %13.8f\n",i,ap->name,txid->timestamp,sender,dstr(seller->quantity)*ap->mult,receiver,dstr(buyer->quantity)*ap->mult,dstr(txid->quantity)*ap->mult,dstr(txid->price));
    return(json);
}

struct NXT_acct **get_assetaccts(int32_t *nump,char *assetidstr,int32_t maxtimestamp)
{
    struct NXT_acct **accts = 0;
    int32_t n = 0;
    cJSON *tmp;
    int32_t i,iter,createdflag;
    char sender[1024],receiver[1024];
    struct NXT_acct *seller,*buyer;
    struct NXT_asset *ap;
    struct NXT_assettxid *txid;
    *nump = 0;
    if ( assetidstr == 0 || assetidstr[0] == 0 || strcmp(assetidstr,ILLEGAL_COINASSET) == 0 || strcmp(assetidstr,NXT_COINASSET) == 0 )
        return(0);
    //printf("maxtime.%d Numtransfers.%d emit ownership percentages of %s\n",maxtimestamp,Numtransfers,assetidstr);
    if ( maxtimestamp <= 0 )
        maxtimestamp = (1 << 30);
    ap = MTadd_hashtable(&createdflag,Global_mp->NXTassets_tablep,assetidstr);
    if ( createdflag == 0 )
    {
        //printf("maxtime.%d Numtransfers.%d num.%d emit ownership percentages of %s %s from %d txid\n",maxtimestamp,Numtransfers,ap->num,ap->name,assetidstr,ap->num);
        for (iter=0; iter<2; iter++)
            for (i=0; i<ap->num; i++)
            {
                txid = ap->txids[i];
                if ( txid->timestamp <= maxtimestamp )
                {
                    expand_nxt64bits(sender,txid->senderbits);
                    expand_nxt64bits(receiver,txid->receiverbits);
                    seller = get_NXTacct(&createdflag,Global_mp,sender);
                    buyer = get_NXTacct(&createdflag,Global_mp,receiver);
                    if ( iter == 0 )
                        accts = clear_workingvars(accts,&n,sender,receiver);
                    else
                    {
                        tmp = update_workingvars(seller,buyer,txid,ap->mult);
                        if ( tmp != 0 )
                            free_json(tmp);
                    }
                }
            }
    }
    *nump = n;
    return(accts);
}

char *emit_asset_json(int64_t *totalp,char *assetidstr,int32_t maxtimestamp,char **blacklist)
{
    struct NXT_acct **accts = 0;
    //static FILE *fp;
    int32_t n = 0;
    double avebuy,avesell,maxprofits;
    int64_t sum,checksum,checkval;
    int32_t assetind,changed,createdflag,err2,i;
    char numstr[1024],issuerhas[1024],*CSV=0,*jsonstr = 0;
    struct NXT_acct *np,*maxnp;
    struct NXT_asset *ap;
    cJSON *json,*array,*item;
    //printf("emit\n");
    if ( (accts= get_assetaccts(&n,assetidstr,maxtimestamp)) == 0 )
        return(0);
   // printf("done emit\n");
    
    ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
    //if ( fp == 0 )
   //     fp = fopen("assetstats.txt","w");
   // printf("ap %p\n",ap);
    
    changed = err2 = 0;
    checksum = sum = 0;
    maxprofits = 0.;
    maxnp = 0;
    issuerhas[0] = 0;
    //printf("n.%d accts.%p\n",n,accts);
    if ( n > 0 )
    {
        json = cJSON_CreateObject();
        array = cJSON_CreateArray();
        for (i=0; i<n; i++)
        {
            np = accts[i];
            if ( strcmp(np->H.U.NXTaddr,GENESISACCT) == 0 || np->quantity == 0 || (blacklist != 0 && listcmp(blacklist,np->H.U.NXTaddr) == 0) )
                continue;
            assetind = get_asset_in_acct(np,ap,0);
            if ( assetind < 0 )
            {
                printf("error getting assetind for %s\n",ap->name);
                continue;
            }
            sum += np->quantity;
            checkval = get_asset_balance(np,assetidstr);
            checksum += checkval;
            if ( checkval != (int64_t)np->quantity )
                changed++;
            if ( checkval != (int64_t)np->quantities[assetind] )
                err2++;
            //printf("%llu vs issuer %llu\n",(long long)np->H.nxt64bits,(long long)ap->issuer);
            if ( np->H.nxt64bits == ap->issuer )
            {
                sprintf(issuerhas,"%.8f",dstr(np->quantity) * ap->mult);
                printf("%s is issuer.%s\n",np->H.U.NXTaddr,nxt64str(ap->issuer));
                continue;
            }
            item = cJSON_CreateObject();
            cJSON_AddItemToObject(item,"NXT",cJSON_CreateString(np->H.U.NXTaddr));
            sprintf(numstr,"%.8f",dstr(np->quantity) * ap->mult);
            cJSON_AddItemToObject(item,"qty",cJSON_CreateString(numstr));
            sprintf(numstr,"%.8f",dstr(checkval) * ap->mult);
            cJSON_AddItemToObject(item,"checkval",cJSON_CreateString(numstr));
            cJSON_AddItemToArray(array,item);
            /*sprintf(line,"%s,%s;",np->H.NXTaddr,numstr);
            printf("line.(%s)\n",line);
            if ( CSV == 0 )
                CSV = malloc(n * 128);
            strcat(CSV,line);*/
            if ( np->quantity == 0 )
                continue;
         
            avebuy = avesell = 0.;
            if ( np->buyqty > 0 )
                avebuy = np->buysum/np->buyqty;
            if ( np->sellqty > 0 )
                avesell = np->sellsum/np->sellqty;
            if ( np->buyqty > 0 && np->sellqty > 0 )
            {
                if ( np->buyqty > np->sellqty )
                    np->profits += np->sellqty * (avesell - avebuy) / SATOSHIDEN;
                else np->profits += np->buyqty * (avesell - avebuy) / SATOSHIDEN;
                if ( np->profits > maxprofits )
                {
                    maxprofits = np->profits;
                    maxnp = np;
                }
            }
            //printf("%s $%.8f NXT.%-22s %16.8f checkval %16.8f buysum %.8f %.8f %.8f sellsum %.8f %.8f %.8f\n",ap->name,np->profits,np->H.NXTaddr,dstr(np->quantity)*ap->mult,ap->mult*dstr(checkval),ap->mult*dstr(np->buyqty),dstr(np->buysum),dstr(avebuy),ap->mult*dstr(np->sellqty),dstr(np->sellsum),dstr(avesell));
        }
        printf("%s num accts.%d sum %.8f RTbalances %.8f | changed %d %d\n",ap->name,n,dstr(sum)*ap->mult,dstr(checksum)*ap->mult,changed,err2);
        cJSON_AddItemToObject(json,"service",cJSON_CreateString("NXTservices"));
        cJSON_AddItemToObject(json,"name",cJSON_CreateString(ap->name));
        cJSON_AddItemToObject(json,"asset",cJSON_CreateString(ap->H.U.assetid));
        expand_nxt64bits(numstr,ap->issuer);
        cJSON_AddItemToObject(json,"issuer",cJSON_CreateString(numstr));
        cJSON_AddItemToObject(json,"issuerhas",cJSON_CreateString(issuerhas));

        sprintf(numstr,"%.8f",dstr(ap->issued) * ap->mult);
        cJSON_AddItemToObject(json,"issued",cJSON_CreateString(numstr));
        sprintf(numstr,"%.8f",dstr(sum) * ap->mult);
        *totalp = (sum * ap->mult);
        cJSON_AddItemToObject(json,"sum",cJSON_CreateString(numstr));
        sprintf(numstr,"%.8f",dstr(checksum) * ap->mult);
        cJSON_AddItemToObject(json,"checksum",cJSON_CreateString(numstr));
        cJSON_AddItemToObject(json,"changed",cJSON_CreateNumber(changed));
        cJSON_AddItemToObject(json,"balances",array);
        if ( CSV != 0 )
        {
            cJSON_AddItemToObject(json,"CSV",cJSON_CreateString(CSV));
            free(CSV);
        }
        jsonstr = cJSON_Print(json);
        /*if ( jsonstr != 0 )
        {
           // printf("%s",jsonstr);
            if ( fp != 0 )
            {
                fprintf(fp,"%s",jsonstr);
                fflush(fp);
            }
        }*/
        free_json(json);
    } else *totalp = 0;
    if ( accts != 0 )
        free(accts);
    if ( maxnp != 0 )
        printf("maxprofits NXT %.8f by acct %s\n",maxnp->profits,maxnp->H.U.NXTaddr);
    return(jsonstr);
}

int64_t calc_MGW_assets(char *coinstr)
{
  //static char *blacklist[] = { NXTISSUERACCT, NXTACCTA, NXTACCTB, NXTACCTC, NXTACCTD, NXTACCTE, BTC_COINASSET, "" };
    int64_t total = 0;
    char *assetidstr,*jsonstr;
    assetidstr = get_assetid_str(coinstr);
    if ( assetidstr != 0 && strcmp(assetidstr,ILLEGAL_COINASSET) != 0 && strcmp(assetidstr,"0") != 0 )
    {
        jsonstr = emit_asset_json(&total,assetidstr,0,MGW_blacklist);
        if ( jsonstr != 0 )
            free(jsonstr);
    }
    return(total);
}

char *emit_acct_json(char *NXTaddr,char *assetidstr,int32_t maxtimestamp,int32_t txlog)
{
    struct coin_info *conv_assetid(char *assetidstr);
    char *get_deposit_addr(int32_t coinid,char *NXTaddr);
    char *find_user_withdrawaddr(int32_t coinid,uint64_t nxt64bits);
    int32_t i,j,createdflag;
    struct coin_info *cp;
    char numstr[1024],sender[1024],receiver[1024],*jsonstr = 0;
    struct NXT_acct *np,*seller,*buyer,*issuernp;
    struct NXT_asset *ap;
    struct NXT_assettxid_list *txlist;
    struct NXT_assettxid *txid;
    cJSON *json=0,*array=0,*item,*txobj,*txarray = 0;
    cp = conv_assetid(assetidstr);
    if ( assetidstr == 0 || assetidstr[0] == 0 || strcmp(assetidstr,ILLEGAL_COINASSET) == 0 || strcmp(assetidstr,NXT_COINASSET) == 0 )
        return(0);
    if ( maxtimestamp <= 0 )
        maxtimestamp = (1 << 30);
    //if ( fp == 0 )
    //    fp = fopen("acctstats.txt","w");
    issuernp = get_NXTacct(&createdflag,Global_mp,NXTISSUERACCT);
    np = get_NXTacct(&createdflag,Global_mp,NXTaddr);
    //printf("numassets.%d %s to %d\n",np->numassets,NXTaddr,maxtimestamp);
    if ( np->numassets > 0 )
    {
        array = cJSON_CreateArray();
        if ( txlog != 0 )
            txarray = cJSON_CreateArray();
        for (i=0; i<np->numassets; i++)
        {
            ap = np->assets[i];
            if ( assetidstr != 0 && assetidstr[0] != 0 && strcmp(ap->H.U.assetid,assetidstr) != 0 )
                continue;
            txlist = np->txlists[i];
            //printf("compare.%s txs.%d\n",ap->name,txlist->num);
            np->quantity = np->buysum = np->buyqty = np->sellsum = np->sellqty = 0;
            //clear_workingvars(0,0,np->H.NXTaddr,0);
            for (j=0; j<txlist->num; j++)
            {
                txid = txlist->txids[j];
                if ( txid->timestamp <= maxtimestamp )
                {
                    //printf("t%d ",txid->timestamp);
                    buyer = seller = 0;
                    expand_nxt64bits(sender,txid->senderbits);
                    expand_nxt64bits(receiver,txid->receiverbits);
                    if ( strcmp(NXTaddr,sender) == 0 )
                        seller = np;
                    else if ( is_special_addr(sender) != 0 )
                        seller = issuernp;
                    
                    if ( strcmp(NXTaddr,receiver) == 0 )//txid->receiverbits == np->H.nxt64bits )
                        buyer = np;
                    else if ( is_special_addr(receiver) != 0 )
                        buyer = issuernp;
                    //printf("sender.(%s) %p receiver.(%s) %p | np.%p issuernp.%p\n",sender,seller,receiver,buyer,np,issuernp);
                    if ( buyer != 0 || seller != 0 )
                    {
                        txobj = update_workingvars(seller,buyer,txid,ap->mult);
                        if ( txobj != 0 )
                        {
                            if ( txlog == 0 || (txlog == 2 && txid->completed >= 0) )
                                free_json(txobj);
                            else
                                cJSON_AddItemToArray(txarray,txobj);
                        }
                    }
                    //printf("np qty %.8f %.8f\n",dstr(np->quantity),dstr(-np->quantity));
                }
            }
            item = cJSON_CreateObject();
            cJSON_AddItemToObject(item,"asset",cJSON_CreateString(ap->H.U.assetid));
            sprintf(numstr,"%.8f",dstr(np->quantity) * ap->mult);
            cJSON_AddItemToObject(item,"qty",cJSON_CreateString(numstr));
            if ( txarray != 0 )
                cJSON_AddItemToObject(item,"transactions",txarray);
            cJSON_AddItemToArray(array,item);
            if ( json == 0 )
            {
                json = cJSON_CreateObject();
                cJSON_AddItemToObject(json,"NXT",cJSON_CreateString(np->H.U.NXTaddr));
                cJSON_AddItemToObject(json,"name",cJSON_CreateString(ap->name));
                cJSON_AddItemToObject(json,"maxtimestamp",cJSON_CreateNumber(maxtimestamp));
            }
        }
    }
#ifdef INSIDE_MGW
    char *depositaddr,*withdrawaddr;
    withdrawaddr = find_user_withdrawaddr(coinid,np->H.nxt64bits);
    depositaddr = get_deposit_addr(coinid,NXTaddr);
    if ( withdrawaddr != 0 || depositaddr != 0 )
    {
        if ( json == 0 )
            json = cJSON_CreateObject();
        if ( depositaddr != 0 )
            cJSON_AddItemToObject(json,"MGW deposit addr",cJSON_CreateString(depositaddr));
        if ( withdrawaddr != 0 )
            cJSON_AddItemToObject(json,"MGW withdraw addr",cJSON_CreateString(withdrawaddr));
    }
#endif
    if ( json != 0 )
    {
        if ( array != 0 )
            cJSON_AddItemToObject(json,"assets",array);
        jsonstr = cJSON_Print(json);
        /*if ( jsonstr != 0 )
         {
         printf("%s",jsonstr);
         if ( fp != 0 )
         {
         fprintf(fp,"%s",jsonstr);
         fflush(fp);
         }
         }*/
        free_json(json);
    }
    return(jsonstr);
}

struct NXT_asset *init_asset(struct NXThandler_info *mp,char *assetidstr)
{
    cJSON *json;
    uint64_t mult = 1;
    char *jsonstr,buf[1024];
    int32_t i,createdflag;
    struct NXT_asset *ap = 0;
    jsonstr = issue_getAsset(0,assetidstr);
    if ( jsonstr != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
            ap->decimals = (int32_t)get_cJSON_int(json,"decimals");
            for (i=7-ap->decimals; i>=0; i--)
                mult *= 10;
            ap->mult = mult;
            if ( extract_cJSON_str(buf,sizeof(buf),json,"quantityQNT") > 0 )
                ap->issued = calc_nxt64bits(buf);
            if ( extract_cJSON_str(buf,sizeof(buf),json,"account") > 0 )
                ap->issuer = calc_nxt64bits(buf);
            if ( extract_cJSON_str(buf,sizeof(buf),json,"description") > 0 )
                ap->description = clonestr(buf);
            if ( extract_cJSON_str(buf,sizeof(buf),json,"name") > 0 )
            {
                //int32_t conv_coinstr(char *);
                if ( tolower(buf[0]) == 'm' && tolower(buf[1]) == 'g' && tolower(buf[2]) == 'w' )//&& conv_coinstr(buf+3) >= 0 )
                    ap->name = clonestr(buf+3);
                else ap->name = clonestr(buf);
            }
            if ( strcmp("11060861818140490423",assetidstr) == 0 )
                printf("INIT_ASSET: NXT.%llu issued %s %s decimals.%d %.8f (%s)\n",(long long)ap->issuer,assetidstr,ap->name,ap->decimals,dstr(mult * ap->issued),ap->description);
            //if ( strcmp(assetidstr,EX_DIVIDEND_ASSETID) == 0 )
            //    ap->exdiv_height = EX_DIVIDEND_HEIGHT;
            free_json(json);
        }
        free(jsonstr);
    }
    return(ap);
}

int process_asset_trades(char *assetidstr,int32_t firstindex,int32_t lastindex)
{
    uint64_t assetidbits;
    cJSON *json,*array,*item;
    int32_t i,n,numtrades = 0;
    struct NXT_trade T;
    char numstr[4096],cmd[4096],*jsonstr;
    assetidbits = calc_nxt64bits(assetidstr);
    //if ( endi == 0 )
    //    endi = (1 << 30);
    sprintf(cmd,"%s=getTrades&asset=%s&firstIndex=%d&lastIndex=%d",NXTSERVER,assetidstr,firstindex,lastindex);
    jsonstr = issue_curl(0,cmd);
    if ( jsonstr != 0 )
    {
        json = cJSON_Parse(jsonstr);
        if ( json != 0 )
        {
            if ( (array= cJSON_GetObjectItem(json,"trades")) != 0 && is_cJSON_Array(array) != 0 )
            {
                n = cJSON_GetArraySize(array);
                for (i=0; i<n; i++)
                {
                    item = cJSON_GetArrayItem(array,i);
                    memset(&T,0,sizeof(T));
                    T.timestamp = (int32_t)get_cJSON_int(item,"timestamp");
                    T.assetid = assetidbits;
                    if ( extract_cJSON_str(numstr,sizeof(numstr),item,"priceNQT") > 0 )
                        T.price = calc_nxt64bits(numstr);
                    if ( extract_cJSON_str(numstr,sizeof(numstr),item,"quantityQNT") > 0 )
                        T.quantity = calc_nxt64bits(numstr);
                    if ( extract_cJSON_str(numstr,sizeof(numstr),item,"block") > 0 )
                        T.blockbits = calc_nxt64bits(numstr);
                    if ( extract_cJSON_str(numstr,sizeof(numstr),item,"askOrder") > 0 )
                        T.askorder = calc_nxt64bits(numstr);
                    if ( extract_cJSON_str(numstr,sizeof(numstr),item,"bidOrder") > 0 )
                        T.bidorder = calc_nxt64bits(numstr);
                    if ( update_assettxid_trade(&T) != 0 )
                        numtrades++;
                    //else printf("process_asset_trades: error adding trade? (%s)\n",jsonstr);
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(numtrades);
}

int32_t update_asset_trades(struct NXThandler_info *mp,struct NXT_asset *ap)
{
    int32_t ind = 0;
    while ( process_asset_trades(ap->H.U.assetid,ind,ind) != 0 )
        ind++;
    return(ind);
}

void update_assets_trades(struct NXThandler_info *mp)
{
    int32_t createdflag,i = 0;
    uint64_t assetid;
    struct NXT_asset *ap;
    char assetidstr[1024];
    if ( Assetlist != 0 )
    {
        while ( (assetid= Assetlist[i++]) != 0 )
        {
            expand_nxt64bits(assetidstr,assetid);
            ap = get_NXTasset(&createdflag,Global_mp,assetidstr);
            update_asset_trades(mp,ap);
        }
    }
}

void init_assets(struct NXThandler_info *mp)
{
    char assetidstr[1024],sender[1024],receiver[1024];
    int32_t i,j,n;
    struct NXT_asset *ap;
    struct NXT_assettxid *txid;
    if ( (Assetlist= issue_getAssetIds(&n)) != 0 )
    {
        for (i=0; i<n; i++)
        {
            expand_nxt64bits(assetidstr,Assetlist[i]);
            ap = init_asset(mp,assetidstr);
            process_asset_trades(assetidstr,0,1<<30);
            for (j=0; j<ap->num; j++)
            {
                txid = ap->txids[j];
                expand_nxt64bits(sender,txid->senderbits);
                expand_nxt64bits(receiver,txid->receiverbits);
                printf("%d) %-12s t%-8d NXT.%-21s -> NXT.%-21s %13.8f @ %11.8f\n",j,ap->name,txid->timestamp,sender,receiver,dstr(txid->quantity)*ap->mult,dstr(txid->U.price));
            }
        }
        printf("total %d assetids\n",n);
    }
    //getchar();
}

int32_t ensure_NXT_blocks(struct NXThandler_info *mp,int32_t height,uint64_t blockid,int32_t timestamp)
{
    int32_t changed = 0;
    if ( height >= (int32_t)(sizeof(mp->blocks)/sizeof(*mp->blocks)) )
        return(-1);
    if ( (height+1) > mp->numblocks )
    {
        //mp->blocks = realloc(mp->blocks,sizeof(*mp->blocks) * (height + 1));
//memset(&mp->blocks[mp->numblocks],0,sizeof(*mp->blocks) * (height + 1 - mp->numblocks));
        //mp->timestamps = realloc(mp->timestamps,sizeof(*mp->timestamps) * (height + 1));
        //memset(&mp->timestamps[mp->numblocks],0,sizeof(*mp->timestamps) * (height + 1 - mp->numblocks));
        mp->numblocks = (int32_t)(height + 1);
        //printf("alloc numblocks.%d %p %p\n",mp->numblocks,mp->blocks[mp->numblocks-1],mp->blocks[lp->numblocks-2]);
    }
    if ( timestamp != 0 )
        mp->timestamps[height] = timestamp;
    if ( height > mp->height )
        mp->height = height;
    if ( blockid != 0 )
    {
        if ( mp->blocks[height] != 0 && mp->blocks[height] != blockid )
        {
            changed = 1;
            if ( mp->NXTheight != 0 && height <= mp->NXTheight )
                mp->GLEFU++;
            printf("NXTblocks changed height.%d when NXTheight.%d, GLEFU.%d condition %llu -> %llu\n",height,mp->NXTheight,mp->GLEFU,(long long)mp->blocks[height],(long long)blockid);
        }
        mp->blocks[height] = blockid;
    }
    return(changed);
}

/*void update_timestamps_fifo(struct NXThandler_info *mp,int32_t height)
{
    int32_t i,h,n;
    n = (int32_t)(sizeof(mp->timestampfifo)/sizeof(*mp->timestampfifo));
    for (i=0; i<n; i++)
    {
        h = height - i;
        if ( h < 0 )
            break;
        mp->timestampfifo[i] = mp->timestamps[h];
    }
}*/

int32_t getNXTblock(char *nextblock,struct NXThandler_info *mp,char *blockidstr,int32_t *height,int32_t *timestamp,int32_t *numblocks)
{
    int32_t errcode,retflag = 0;
    cJSON *nextjson,*json = 0;
    char cmd[4096],*jsonstr;
    if ( blockidstr[0] == 0 )
    {
        printf("getNXTblock h.%d getNXTblock.(%s)\n",*height,blockidstr);
        return(-1);
    }
    sprintf(cmd,"%s=getBlock&block=%s",NXTSERVER,blockidstr);
    jsonstr = issue_curl(0,cmd);
    if ( jsonstr != 0 )
    {
        json = cJSON_Parse(jsonstr);
        if ( json != 0 )
        {
            errcode = (int32_t)get_cJSON_int(json,"errorCode");
            if ( errcode == 0 )
            {
                *height = (int32_t)get_cJSON_int(json,"height");
                *timestamp = (int32_t)get_cJSON_int(json,"timestamp");
                nextjson = cJSON_GetObjectItem(json,"nextBlock");
                if ( nextjson != 0 )
                    copy_cJSON(nextblock,nextjson);
                else nextblock[0] = 0;
                if ( *height > 0 && *timestamp > 0 && nextblock[0] != 0 )
                {
                    if ( Finished_loading != 0 || Historical_done != 0 || (*height % 1000) == 0 )
                        printf("F.%d H.%d RT.%d height.%d/%d of %d t.%d %s next.%s\n",Finished_loading,Historical_done,Global_mp->RTflag,*height,mp->height,*numblocks,*timestamp,blockidstr,nextblock);
                    //if ( *height != *prevheight+1 )
                    //    printf("WARNING: height.%d != prevheight.%d + 1\n",*height,*prevheight);
                    ensure_NXT_blocks(mp,*height,calc_nxt64bits(blockidstr),*timestamp);
                    if ( *height+1 > *numblocks )
                        *numblocks = *height+1;
                } else retflag = -1;
                //if ( *height <= *prevheight || *timestamp <= *prevtimestamp || nextblock[0] == 0 )
                //    printf("UNEXPECTED height.%d prev.%d of %d t.%d prev.%d %s next.%s\n",*height,*prevheight,*numblocks,*timestamp,*prevtimestamp,blockidstr,nextblock);
                //*prevheight = *height;
                //*prevtimestamp = *timestamp;
            } else retflag = -1;
            free_json(json);
        }
        free(jsonstr);
    } else retflag = -1;
    return(retflag);
}

void *getNXTblocks(void *ptr)
{
    struct NXThandler_info *mp = ptr;
    uint64_t blockidbits;
    int32_t h,i,isrescan,changed,numblocks,height,timestamp,nextheight;
    char blockidstr[4096],nextblock[1024],tmpblock[1024],prevblock[1024];
    mp->origblockidstr = ORIGBLOCK;
    strcpy(blockidstr,mp->origblockidstr);
    strcpy(mp->blockidstr,mp->origblockidstr);
    mp->firsttimestamp = issue_getTime(0);
    numblocks = set_current_NXTblock(&isrescan,0,tmpblock);
    mp->height = height = numblocks - 1;
    ensure_NXT_blocks(mp,numblocks,0,0);
    Finished_loading = 0;
    nextheight = 0;
    while ( blockidstr[0] != 0 && getNXTblock(tmpblock,mp,blockidstr,&height,&timestamp,&numblocks) == 0 && (nextheight == 0 || height == nextheight) )
    {
        if ( tmpblock[0] != 0 )
        {
            strcpy(blockidstr,tmpblock);
            strcpy(mp->blockidstr,nextblock);
            tmpblock[0] = 0;
            nextheight = height+1;
        }
    }
    Finished_loading = 1;
    printf("reached end of consecutive blocks at height.%d\n",nextheight);
    while ( 1 )
    {
        isrescan = 1;
        while ( isrescan != 0 || blockidstr[0] == 0 )
        {
            sleep(3);
            numblocks = set_current_NXTblock(&isrescan,0,blockidstr);
            //printf("numblocks.%d %s\n",numblocks,blockidstr);
        }
        mp->height = numblocks - 1;
        for (i=1; i<3*(MIN_NXTCONFIRMS+mp->extraconfirms); i++)
        {
            h = numblocks - i;
            blockidbits = calc_nxt64bits(blockidstr);
            set_prev_NXTblock(0,&height,&timestamp,prevblock,blockidstr);
            if ( height != h )
            {
                printf("WARNING: got height.%d vs expected h.%d for %s\n",height,h,blockidstr);
                h = height;
            }
            if ( height > 0 && height < numblocks && timestamp > 0 && prevblock[0] != 0 )
            {
                if ( mp->blocks[h] != 0 && (mp->blocks[h] != blockidbits || mp->timestamps[h] != timestamp) )
                    printf("WARNING: when NXTheight.%d block changed at height %d: %llu != %llu || timestamp %d != %d |; maxpop depth.%d height.%d last.%d extra.%d\n",mp->NXTheight,h,(long long)mp->blocks[h],(long long)blockidbits,mp->timestamps[h],timestamp,mp->maxpopdepth,mp->maxpopheight,mp->lastchanged,mp->extraconfirms);
                changed = ensure_NXT_blocks(mp,h,blockidbits,timestamp);
                if ( changed != 0 )
                {
                    printf("height.%d block popped and got new one.(%llu) depth.%d\n",h,(long long)mp->blocks[h],i);
                    if ( i > mp->maxpopdepth )
                    {
                        mp->maxpopdepth = i;
                        mp->maxpopheight = h;
                        if ( i > mp->extraconfirms )
                            mp->extraconfirms = i;
                    }
                    mp->lastchanged = h;
                    mp->height = h;
                    if ( mp->NXTheight >= mp->lastchanged )
                    {
                        fprintf(stderr,"NXTloop: mp->NXTheight.%d >= mp->lastchanged %d | NXT network fragmentation?? | maxpop depth.%d height.%d extra.%d\n",mp->NXTheight,mp->lastchanged,mp->maxpopdepth,mp->maxpopheight,mp->extraconfirms);
                        exit(-1);
                    }
                }
                strcpy(blockidstr,prevblock);
            }
            else
            {
                fprintf(stderr,"BAD ERROR no data for height.%d\n",h);
                exit(-1);
            }
        }
    }
    return(0);
}

int32_t process_NXTblock(int32_t *heightp,char *nextblock,struct NXThandler_info *mp,int32_t height,char *blockidstr)
{
    char cmd[4096],txid[1024],*jsonstr;
    cJSON *transactions,*nextjson;
    int64_t n;
    union NXTtype retval;
    int32_t i,timestamp=0,errcode,processed = 0;
    *heightp = 0;
    //sprintf(cmd,"%s=getBlock&block=%s",NXTSERVER,blockidstr);
    //retval = extract_NXTfield(0,cmd,0,0);
    //printf("process_NXTblock.(%s)\n",blockidstr);
    if ( strlen(blockidstr) == 0 )
    {
        printf("process_NXTblock null blockidstr???\n");
        return(0);
    }
    sprintf(cmd,"%s=getBlock&block=%s",NXTSERVER,blockidstr);
    //printf("issue curl.(%s)\n",cmd);
    jsonstr = issue_curl(0,cmd);
    if ( jsonstr != 0 )
        retval.json = cJSON_Parse(jsonstr);
    else retval.json = 0;
    if ( retval.json != 0 )
    {
//printf("%s\n",cJSON_Print(retval.json));
        errcode = (int32_t)get_cJSON_int(retval.json,"errorCode");
        if ( errcode == 0 )
        {
            *heightp = (int32_t)get_cJSON_int(retval.json,"height");
            if ( *heightp != height )
                printf("process_NXTblock warning height.%d != %d\n",height,*heightp);
            timestamp = (int32_t)get_cJSON_int(retval.json,"timestamp");
            nextjson = cJSON_GetObjectItem(retval.json,"nextBlock");
            if ( nextjson != 0 )
                copy_cJSON(nextblock,nextjson);
            else nextblock[0] = 0;
            transactions = cJSON_GetObjectItem(retval.json,"transactions");
            n = get_cJSON_int(retval.json,"numberOfTransactions");
            if ( n != cJSON_GetArraySize(transactions) )
                printf("JSON parse error!! %ld != %d\n",(long)n,cJSON_GetArraySize(transactions));
            if ( n != 0 )
            {
                for (i=0; i<n; i++)
                {
                    copy_cJSON(txid,cJSON_GetArrayItem(transactions,i));
                    if ( txid[0] != 0 )
                        processed += process_NXTtransaction(mp,txid,height,timestamp);
                }
            }
        }
        free_json(retval.json);
    }
    if ( jsonstr != 0 )
        free(jsonstr);
    return(timestamp);
}

void NXTloop(struct NXThandler_info *mp)
{
    uint64_t block;
    char nextblock[1024],blockidstr[1024];
    int32_t tmp,height=0,timestamp,numconfirms = MIN_NXTCONFIRMS;
    printf("start of NXTloop\n");
    if ( 1 )
    {
        process_NXTblock(&tmp,nextblock,mp,height,GENESISBLOCK);
        fprintf(stderr,"tmp.%d height.%d genesis.%s\n",tmp,height,GENESISBLOCK);
        if ( tmp != height )
            exit(666);
        // printf("start\n");
    }
    process_NXTblock(&tmp,nextblock,mp,height,ORIGBLOCK);
    height = tmp;
    while ( Finished_loading == 0 )
        sleep(1);
    if ( mp->initassets != 0 )
        init_assets(mp);
    while ( 1 )
    {
        while ( height >= mp->height-numconfirms )
        {
            sleep(3);
            numconfirms = MIN_NXTCONFIRMS + mp->extraconfirms;
        //printf("height.%d mp->height.%d\n",height,mp->height);
        }
        if ( (block= mp->blocks[height]) != 0 )
        {
            expand_nxt64bits(blockidstr,block);
            timestamp = process_NXTblock(&tmp,nextblock,mp,height,blockidstr);
            if ( tmp == height ) //timestamp == mp->timestamps[height] && 
            {
                if ( Historical_done == 0 && height >= mp->height-numconfirms-1 )
                {
                    Historical_done = 1;
                    printf("Historical processing done, now RT mode height.%d\n",height);
                }
                if ( (height % 100) == 0 ) //Historical_done != 0 || 
                    printf("F.%d H.%d t.%d height.%d %d lastblock.(%s) -> nextblock.(%s)\n",Finished_loading,Historical_done,timestamp,height,mp->height,blockidstr,nextblock);
                if ( Historical_done != 0 )
                {
                    mp->RTflag++;   // wait for first block before doing any side effects
                    //printf("update assets trades\n");
                    update_assets_trades(mp);
                    //printf("call_handlers\n");
                    call_handlers(mp,NXTPROTOCOL_NEWBLOCK,height);
                    //printf("calling gen_testforms\n");
                    //gen_testforms(0);
                    //printf("done calling gen_testforms\n");
                }
                mp->NXTheight = height++;
            }
            else
            {
                printf("ERROR: height.%d t.%d returns %d and timestamp.%d for block.(%s) next.(%s)\n",height,mp->timestamps[height],tmp,timestamp,blockidstr,nextblock);
                sleep(30);
            }
        }
        else
        {
            printf("ERROR!!!!!!!! no block at height.%d nextblock.(%s)?\n",height,nextblock); //getchar();
            sleep(3);
            height--;
        }
    }
}
