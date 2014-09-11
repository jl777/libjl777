//
//  jsoncodec.h
//  Created by jl777, Mar 2014
//  MIT License
//


#ifndef gateway_jsoncodec_h
#define gateway_jsoncodec_h

//#include <zlib.h>
struct compressed_json { uint32_t complen,sublen,origlen; unsigned char encoded[]; };


struct jsonwords { const char *word; int32_t len,count; };
struct jsonwords *JSONlist = 0;
struct jsonwords _JSONlist[128] = { {"requestType", 11, 2}, {"alias", 5, 5}, {"uri", 3, 3}, {"transactionBytes", 16, 3}, {"order", 5, 4}, {"poll", 4, 2}, {"vote", 4, 1}, {"name", 4, 4}, {"description", 11, 4}, {"minNumberOfOptions", 18, 2}, {"maxNumberOfOptions", 18, 2}, {"optionsAreBinary", 16, 2}, {"option", 6, 1}, {"secretPhrase", 12, 8}, {"publicKey", 9, 4}, {"deadline", 8, 3}, {"referencedTransaction", 21, 1}, {"hallmark", 8, 3}, {"website", 7, 2}, {"token", 5, 2}, {"account", 7, 15}, {"timestamp", 9, 7}, {"asset", 5, 11}, {"subtype", 7, 1}, {"limit", 5, 2}, {"assetName", 9, 1}, {"block", 5, 2}, {"numberOfConfirmations", 21, 1}, {"peer", 4, 1}, {"firstIndex", 10, 1}, {"lastIndex", 9, 1}, {"transaction", 11, 4}, {"hash", 4, 4}, {"quantity", 8, 7}, {"host", 4, 3}, {"weight", 6, 3}, {"date", 4, 2}, {"price", 5, 4}, {"recipient", 9, 3}, {"message", 7, 1}, {"amount", 6, 1}, {"errorCode", 9, 10}, {"errorDescription", 16, 10}, {"error", 5, 3}, {"valid", 5, 2}, {"balance", 7, 3}, {"effectiveBalance", 16, 3}, {"unconfirmedBalance", 18, 3}, {"assetBalances", 13, 1}, {"unconfirmedAssetBalances", 24, 1}, {"blockIds", 8, 1}, {"askOrderIds", 11, 2}, {"bidOrderIds", 11, 2}, {"accountId", 9, 1}, {"transactionIds", 14, 1},  {"aliasIds", 8, 1}, {"trades", 6, 2}, {"height", 6, 3}, {"numberOfTrades", 14, 2}, {"assetIds", 8, 1}, {"assets", 6, 1}, {"generator", 9, 1}, {"numberOfTransactions", 20, 2}, {"totalAmount", 11, 1}, {"totalFee", 8, 1}, {"payloadLength", 13, 1}, {"version", 7, 3}, {"baseTarget", 10, 1}, {"previousBlock", 13, 1}, {"nextBlock", 9, 1}, {"payloadHash", 11, 1}, {"generationSignature", 19, 1}, {"previousBlockHash", 17, 1}, {"blockSignature", 14, 1}, {"transactions", 12, 1}, {"genesisBlockId", 14, 1}, {"genesisAccountId", 16, 1}, {"maxBlockPayloadLength", 21, 1}, {"transactionTypes", 16, 1}, {"peerStates", 10, 1}, {"guaranteedBalance", 17, 2}, {"address", 7, 1}, {"state", 5, 1}, {"announcedAddress", 16, 1}, {"shareAddress", 12, 1}, {"downloadedVolume", 16, 1}, {"uploadedVolume", 14, 1}, {"application", 11, 1}, {"platform", 8, 1}, {"blacklisted", 11, 1}, {"peers", 5, 1}, {"options", 7, 1}, {"voters", 6, 1}, {"pollIds", 7, 1}, {"time", 4, 2}, {"lastBlock", 9, 1}, {"cumulativeDifficulty", 20, 1}, {"totalEffectiveBalance", 21, 1}, {"numberOfBlocks", 14, 1}, {"numberOfAccounts", 16, 1}, {"numberOfAssets", 14, 1}, {"numberOfOrders", 14, 1}, {"numberOfAliases", 15, 1}, {"numberOfPolls", 13, 1}, {"numberOfVotes", 13, 1}, {"numberOfPeers", 13, 1}, {"numberOfUsers", 13, 1}, {"numberOfUnlockedAccounts", 24, 1}, {"lastBlockchainFeeder", 20, 1}, {"availableProcessors", 19, 1}, {"maxMemory", 9, 1}, {"totalMemory", 11, 1}, {"freeMemory", 10, 1}, {"confirmations", 13, 2}, {"blockTimestamp", 14, 1}, {"sender", 6, 1}, {"unconfirmedTransactionIds", 25, 1}, {"aliases", 7, 1}, {"foundAndStopped", 15, 1},

    {"withdrawaddrs",13,1}, {"depositaddrs",12,1}, {"redeemScript",12,1}, {"assetxfers",9,1}, 
    {"pubkey",7,1}, {"ipaddr",6,1}, {"coinid",6,1}, {"flag",4,1},
};


// deleted {"maxArbitraryMessageLength", 25, 1}, {"id", 2, 1},{"type", 4, 1}, {"fee", 3, 1}, 
int32_t Num_JSONwords = 128;

int32_t _encode_json(unsigned char *dest,unsigned long *lenp,char *jsontext,unsigned long *sublenp)
{
    int32_t i,j,k,flag,retval,level = 9;
    unsigned long len,sublen;
    unsigned char *src,*substr;
    len = strlen(jsontext);
    src = (unsigned char *)clonestr(jsontext);
    substr = malloc(len+1);
    stripstr((char *)src,len);
    for (i=k=0; src[i]!=0; i++)
    {
        if ( src[i] == '"' && src[i+1] != '"' )
        {
            for (j=flag=0; j<Num_JSONwords&&j<128&&JSONlist[j].len>0; j++)
                if ( strncmp((char *)src+i+1,JSONlist[j].word,JSONlist[j].len) == 0 && src[i+1+JSONlist[j].len] == '"' && src[i+2+JSONlist[j].len] == ':' )
                {
                    i += JSONlist[j].len + 2;
                    substr[k++] = 0x80 | j;
                    flag = 1;
                    break;
                }
            if ( flag != 0 )
                continue;
        }
        else if ( (src[i] & 0x80) != 0 )
            printf("unexpected control char in srctext\n");
        substr[k++] = src[i];
    }
    substr[k] = 0;
    *sublenp = sublen = strlen((char *)substr);
    retval = compress2(dest,lenp,substr,sublen,level);
    free(src); free(substr);
    return(retval);
}

int32_t _decode_json(unsigned char *decoded,long sublen,unsigned char *encoded,unsigned long *lenp)
{
    int32_t retval,i,j,k,wordi;
    unsigned char *rawdecoded;
    rawdecoded = malloc(sublen+1);
    decoded[0] = 0;
    retval = uncompress(rawdecoded,lenp,encoded,sublen);
    if ( retval == 0 )
    {
        for (i=j=0; i<sublen; i++)
        {
            if ( (rawdecoded[i] & 0x80) != 0 )
            {
                wordi = (rawdecoded[i] & 0x7f);
                decoded[j++] = '"';
                for (k=0; k<JSONlist[wordi].len; k++)
                    decoded[j++] = JSONlist[wordi].word[k];
                decoded[j++] = '"';
                decoded[j++] = ':';
            }
            else decoded[j++] = rawdecoded[i];
        }
        decoded[j++] = 0;
    } else printf("zlib retval.%d\n",retval);
    free(rawdecoded);
    return(retval);
}

int32_t compare_jsontext(char *jsonA,char *jsonB)
{
    int32_t retval;
    long lenA,lenB;
    char *strA,*strB;
    strA = clonestr(jsonA); strB = clonestr(jsonB);
    lenA = stripstr(strA,strlen(strA));
    lenB = stripstr(strB,strlen(strB));
    if ( lenA == lenB )
        retval = strcmp(strA,strB);
    else retval = (int32_t)(lenA - lenB);
    free(strA); free(strB);
    return(retval);
}

struct compressed_json *encode_json(char *jsontext)
{
    int32_t retval;
    struct compressed_json *jsn = 0;
    unsigned long len,sublen,origlen;
    unsigned char *encoded;
    origlen = len = strlen(jsontext);
    encoded = malloc(len+1);
    retval = _encode_json(encoded,&len,jsontext,&sublen);
    if ( retval == 0 )
    {
        jsn = malloc(sizeof(*jsn) + len);
        jsn->origlen = (uint32_t)origlen;
        jsn->complen = (uint32_t)len;
        jsn->sublen = (uint32_t)sublen;
        memcpy(jsn->encoded,encoded,len);
    } else printf("encode_json: error calling zlib.(%s)\n",jsontext);
    free(encoded);
    return(jsn);
}

char *decode_json(struct compressed_json *jsn,int32_t dictionaryid)
{
    int32_t decoderet;
    unsigned char *decoded;
    unsigned long sublen;
    sublen = jsn->sublen;
    decoded = malloc(jsn->origlen+1);
    decoderet = _decode_json(decoded,sublen,jsn->encoded,&sublen);
    if ( decoderet == 0 )
        return((char *)decoded);
    free(decoded);
    return(0);
}

int32_t init_jsoncodec(char *jsontext)
{
    FILE *fp;
    int32_t i,n;
    struct compressed_json *jsn = 0;
    char line[512],*word,*decoded;
    unsigned long origlen;//,encodelen,sublen;
    int32_t cmpret = 0;
    if ( Num_JSONwords == 0 && (fp= fopen("/tmp/words","r")) != 0 ) // grep all NXT .java files for response.put and req.getParameter > /tmp/words
    {
        n = 0;
        while ( fgets(line,sizeof(line),fp) != 0 )
        {
            word = 0;
            for (i=0; line[i]!=0; i++)
                if ( line[i] == '"' )
                {
                    word = line + i + 1;
                    break;
                }
            if ( word != 0 )
            {
                for (i++; line[i]!='"'&&line[i]!=0; i++)
                    ;
                line[i] = 0;
                for (i=0; i<n; i++)
                    if ( strcmp(word,JSONlist[i].word) == 0 )
                    {
                        JSONlist[i].count++;
                        break;
                    }
                if ( i == n )
                {
                    n++;
                    JSONlist = realloc(JSONlist,n * sizeof(*JSONlist) * n);
                    memset(&JSONlist[i],0,sizeof(JSONlist[i]));
                    JSONlist[i].word = clonestr(word);
                    JSONlist[i].len = (int32_t)strlen(word);
                    JSONlist[i].count = 1;
                }
            }
        }
        printf("struct jsonwords JSONlist[] = { ");
        for (i=0; i<n; i++)
            printf("{\"%s\", %d, %d}, ",JSONlist[i].word,JSONlist[i].len,JSONlist[i].count);
        printf("};\nint32_t Num_JSONwords = %d;\n",n);
        Num_JSONwords = n;
        fclose(fp);
    }
    else
    {
        JSONlist = _JSONlist;
        for (i=0; i<128; i++)
        {
            if ( JSONlist[i].word == 0 )
                break;
            JSONlist[i].len = (int32_t)strlen(JSONlist[i].word);
        }
    }
    if ( jsontext != 0 )
    {
        origlen = strlen(jsontext);
        jsn = encode_json(jsontext);
        if ( jsn != 0 )
        {
            decoded = decode_json(jsn,0);
            if ( decoded != 0 )
            {
                cmpret = compare_jsontext(jsontext,decoded);
                printf("(%s).%ld ->\n(%s).%d retvals %d | compression ratio %.3f (orig.%d substition.%d compressed.%d)\n",jsontext,origlen,decoded,jsn->complen,cmpret,(double)origlen/jsn->complen,jsn->origlen,jsn->sublen,jsn->complen);
                free(decoded);
            }
            free(jsn);
        }
    }
    return(cmpret);
}
#endif
