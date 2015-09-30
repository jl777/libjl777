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

int32_t crypto777_addnode(struct crypto777_node *nn)
{
    return(0);
}

struct crypto777_node *crypto777_findemail(char *email)
{
    int32_t i;
    for (i=0; i<NETWORKSIZE; i++)
        if ( Network[i] != 0 && strcmp(Network[i]->email,email) == 0 )
            return(Network[i]);
    return(0);
}

cJSON *crypto777_json(struct crypto777_node *nn)
{
    cJSON *array,*json = cJSON_CreateObject();
    char str384[(384>>3) * 2 + 1];
    int32_t i;
    cJSON_AddItemToObject(json,"email",cJSON_CreateString(nn->email));
    cJSON_AddItemToObject(json,"ip_port",cJSON_CreateString(nn->ip_port));
    init_hexbytes_noT(str384,nn->mypubkey.bytes,sizeof(nn->mypubkey)), cJSON_AddItemToObject(json,"pubkey",cJSON_CreateString(str384));
    init_hexbytes_noT(str384,nn->emailhash.bytes,sizeof(nn->emailhash)), cJSON_AddItemToObject(json,"acct777",cJSON_CreateString(str384));
    array = cJSON_CreateArray();
    for (i=0; i<nn->numpeers; i++)
        if ( nn->peers[i] != 0 )
            sprintf(str384,"%llu",(long long)nn->peers[i]->nxt64bits), cJSON_AddItemToArray(array,cJSON_CreateString(str384));
    cJSON_AddItemToObject(json,"peers",array);
    return(json);
}

int32_t _jsoncmp(cJSON *jsonA,cJSON *jsonB,char *field)
{
    cJSON *objA,*objB;
    struct destbuf strA,strB;
    objA = cJSON_GetObjectItem(jsonA,field), copy_cJSON(&strA,objA);
    objB = cJSON_GetObjectItem(jsonB,field), copy_cJSON(&strB,objB);
    return(strcmp(strA.buf,strB.buf));
}

int32_t crypto777_jsoncmp(cJSON *jsonA,cJSON *jsonB)
{
    cJSON *arrayA,*arrayB;
    int32_t i,n;
    uint64_t peersA[256],peersB[256];
    arrayA = cJSON_GetObjectItem(jsonA,"peers");
    arrayB = cJSON_GetObjectItem(jsonB,"peers");
    if ( jsonA != 0 && jsonB != 0 && arrayA != 0 && arrayB != 0 && (n= cJSON_GetArraySize(arrayA)) == cJSON_GetArraySize(arrayB) )
    {
        if ( _jsoncmp(jsonA,jsonB,"email") != 0 )
            return(-2);
        if ( _jsoncmp(jsonA,jsonB,"pubkey") != 0 )
            return(-3);
        if ( _jsoncmp(jsonA,jsonB,"acct777") != 0 )
            return(-4);
        if ( n > 0 )
        {
            for (i=0; i<n&&i<MAXPEERS; i++)
            {
                peersA[i] = get_API_nxt64bits(cJSON_GetArrayItem(arrayA,i));
                peersB[i] = get_API_nxt64bits(cJSON_GetArrayItem(arrayB,i));
            }
            sort64s(peersA,n,sizeof(peersA[0]));
            sort64s(peersB,n,sizeof(peersB[0]));
            for (i=0; i<n&&i<MAXPEERS; i++)
                if (peersA[i] != peersB[i] )
                    return(-10-i);
        }
        return(0);
    }
    return(-1);
}

int32_t crypto777_publish(struct crypto777_node *nn)
{
    struct destbuf name,info;
    char cmd[4096],str384[1024],*jsonstr,*infostr,*jsonstr2;
    cJSON *json,*json2,*obj,*infojson,*nnjson;
    int32_t retval = -1;
    sprintf(cmd,"requestType=getAccount&account=%llu",(long long)nn->nxt64bits);
    if ( (jsonstr= issue_NXTPOST(cmd)) != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            copy_cJSON(&name,cJSON_GetObjectItem(json,"name"));
            if ( (obj= cJSON_GetObjectItem(json,"description")) != 0 )
            {
                copy_cJSON(&info,obj);
                unstringify(info.buf);
                if ( (infojson= cJSON_Parse(info.buf)) != 0 )
                {
                    if ( (nnjson= crypto777_json(nn)) != 0 )
                    {
                        if ( crypto777_jsoncmp(infojson,nnjson) == 0 )
                            retval = 0;
                        else if ( (infostr= cJSON_Print(infojson)) != 0 )
                        {
                            init_hexbytes_noT(str384,nn->nxtpass.bytes,sizeof(nn->nxtpass));
                            sprintf(cmd,"requestType=setAccountInfo&account=%llu&name=%s&description=%s&secretPhrase=%s&deadline=%u&feeNQT=%llu&messageIsText=false",(long long)nn->nxt64bits,nn->email,infostr,str384,DEFAULT_NXT_DEADLINE,(long long)MIN_NQTFEE);
                            if ( (jsonstr2= issue_NXTPOST(cmd)) != 0 )
                            {
                                if ( (json2= cJSON_Parse(jsonstr2)) != 0 )
                                {
                                    retval = (int32_t)get_cJSON_int(json2,"errorCode");
                                    free_json(json2);
                                } free(jsonstr2);
                            } free(infostr);
                        } free_json(nnjson);
                    } free_json(infojson);
                } else printf("Cant parse AccountInfo.(%s)\n",info.buf);
            } free_json(json);
        } free(jsonstr);
    } else printf("Error issuing.(%s)\n",cmd);
    return(retval);
}

struct crypto777_node *crypto777_node(char *url,char *email,bits384 mypassword,char *type)
{
    struct crypto777_node *nn = 0;
    char nxtpassphrase[1024];
    uint64_t hit,other64bits = calc_nxt64bits(GENESISACCT);
    if ( (nn= calloc(1,sizeof(*nn))) != 0 )
    {
        crypto777_transport(nn,url,type);
        strncpy(nn->email,email,sizeof(nn->email)-1);
        nn->mypassword = mypassword;
        SaM(&nn->emailhash,(uint8_t *)email,(int32_t)strlen(email),0,0);
        hit = SaM(&nn->nxtpass,(uint8_t *)email,(int32_t)strlen(email),mypassword.bytes,(int32_t)sizeof(nn->mypassword));
        init_hexbytes_noT(nxtpassphrase,nn->nxtpass.bytes,sizeof(nn->nxtpass));
        nn->nxt64bits = conv_NXTpassword(nn->nxtsecret.bytes,nn->nxtpubkey.bytes,(uint8_t *)nxtpassphrase,(int32_t)strlen(nxtpassphrase));
        nn->pubkeychainid = 0, nn->revealed = 3;
       // nn->mypubkey = SaM_chain(mypassword,nn->pubkeychainid,0,nn->revealed);
        crypto777_publish(nn);
        crypto777_link(nn->nxt64bits,&nn->broadcast,nn->nxtpass,other64bits,0,0);
        nn->broadcast.recv.nxt64bits = nn->broadcast.send.nxt64bits = 0;
        printf("%s: hit.%llu nxt.%llu SaM.%llu | email.%llu\n",email,(long long)hit,(long long)nn->nxt64bits,(long long)nn->mypubkey.txid,(long long)nn->emailhash.txid);
    }
    return(nn);
}

