//
//  html.h
//  Created by jl777, April 2014
//  MIT License
//

#ifndef gateway_html_h
#define gateway_html_h

char *formfield(char *name,char *disp,int cols,int rows)
{
    char str[512],buf[1024];
    if ( rows == 0 && cols == 0 )
        sprintf(str,"<input type=\"text\" name=\"%s\"/>",name);
    else sprintf(str,"<textarea cols=\"%d\" rows=\"%d\"  name=\"%s\"/></textarea>",cols,rows,name);
    sprintf(buf,"<td>%s</td> <td> %s </td> </tr>\n<tr>\n",disp,str);
    return(clonestr(buf));
}

char *make_form(char *NXTaddr,char **scriptp,char *name,char *disp,char *button,char *url,char *path,int (*fields_func)(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp))
{
    int i,n;
    char *fields[100],str[512],buf[65536];
    memset(fields,0,sizeof(fields));
    n = (*fields_func)(NXTaddr,path,name,fields,scriptp);
    if ( n > (int)(sizeof(fields)/sizeof(*fields)) )
    {
        printf("%s form is too big!! %d\n",name,n);
        return(0);
    }
    sprintf(buf,"<b>%s</b>  <br/> <form name=\"%s\" action=\"%s/%s\" method=\"POST\" onsubmit=\"return submitForm(this);\">\n<table>\n",disp,name,url,path);
    for (i=0; i<n; i++)
    {
        if ( fields[i] != 0 )
        {
            strcat(buf,fields[i]);
            free(fields[i]);
        }
    }
    sprintf(str,"<td colspan=\"2\"> <input type=\"button\" value=\"%s\" onclick=\"click_%s()\" /></td> </tr>\n</table></form>",button,name);
    if ( (strlen(buf)+strlen(str)) >= sizeof(buf) )
    {
        printf("yikes! make_form.%s stack smashing???\n",name);
        return(0);
    }
    strcat(buf,str);
    return(clonestr(buf));
}

char *gen_handler_forms(char *NXTaddr,char *handler,char *disp,int (*forms_func)(char *NXTaddr,char **forms,char **scriptp))
{
    int i,n;
    char str[512],body[65536],*forms[100],*scripts[sizeof(forms)/sizeof(*forms)];
    memset(forms,0,sizeof(forms));
    memset(scripts,0,sizeof(scripts));
    n = (*forms_func)(NXTaddr,forms,scripts);
    if ( n > (int)(sizeof(forms)/sizeof(*forms)) )
    {
        printf("%s has too many forms!! %d\n",handler,n);
        return(0);
    }
    strcpy(body,"<script>\n");
    for (i=0; i<n; i++)
        if ( scripts[i] != 0 )
        {
            strcat(body,scripts[i]);
            free(scripts[i]);
        }
    sprintf(str,"</script>\n\n<h3>%s</h3>\n",disp);
    strcat(body,str);
    for (i=0; i<n; i++)
        if ( forms[i] != 0 )
        {
            strcat(body,forms[i]);
            free(forms[i]);
        }
    strcat(body,"<hr>\n");
    if ( strlen(body) > sizeof(body) )
    {
        printf("yikes! make_form stack smashing???\n");
        return(0);
    }
    return(clonestr(body));
}

char *construct_varname(char **fields,int n,char *name,char *field,char *disp,int col,int row)
{
    char buf[512];
    fields[n++] = formfield(field,disp,col,row);
    sprintf(buf,"document.%s.%s.value",name,field);
    return(clonestr(buf));
}

int gen_list_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*title,*category1,*category2,*category3,*tag1,*tag2,*tag3,*descr,*price,*subatomic,*URI,*duration;
    title = construct_varname(fields,n++,name,"title","Item Title:",0,0);
    category1 = construct_varname(fields,n++,name,"category1","category1:",0,0);
    category2 = construct_varname(fields,n++,name,"category2","category2:",0,0);
    category3 = construct_varname(fields,n++,name,"category3","category3:",0,0);
    tag1 = construct_varname(fields,n++,name,"tag1","tag1:",0,0);
    tag2 = construct_varname(fields,n++,name,"tag2","tag2:",0,0);
    tag3 = construct_varname(fields,n++,name,"tag3","tag3:",0,0);
    descr = construct_varname(fields,n++,name,"description","Item Description:",60,5);
    URI = construct_varname(fields,n++,name,"URI","URI:",60,0);
    price = construct_varname(fields,n++,name,"price","Item Price (in NXT):",0,0);
    subatomic = construct_varname(fields,n++,name,"NXTsubatomic","{NXTsubatomic}:",60,0);
    duration = construct_varname(fields,n++,name,"duration","duration (hours):",0,0);
    sprintf(script,"function click_%s()\n{\nvar A = %s; B = %s; C = %s; D = %s; E = %s; F = %s; G = %s; H = %s; I = %s; J = %s; K = %s; L = %s;\n",name,title,category1,category2,category3,tag1,tag2,tag3,descr,URI,price,subatomic,duration);
    sprintf(vars,"\"title\":\"' + A + '\",\"category1\":\"' + B + '\",\"category2\":\"' + C + '\",\"category3\":\"' + D + '\",\"tag1\":\"' + E + '\",\"tag2\":\"' + F + '\",\"tag3\":\"' + G + '\",\"description\":\"' + H + '\",\"URI\":\"' + I + '\",\"price\":\"' + J + '\",\"subatomic\":\"' + K + '\",\"duration\":\"' + L + '\"");
    sprintf(script+strlen(script),"location.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",%s}';\n}\n",handler,name,NXTaddr,vars);
    *scriptp = clonestr(script);
    free(title); free(category1); free(category2); free(category3); free(tag1); free(tag2); free(tag3);
    free(descr); free(URI); free(price); free(subatomic); free(duration);
    return(n);
}

int gen_listings_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*title,*type,*minprice,*maxprice,*seller,*buyer;
    seller = construct_varname(fields,n++,name,"seller","Only Seller:",0,0);
    buyer = construct_varname(fields,n++,name,"buyer","Only Buyer:",0,0);
    title = construct_varname(fields,n++,name,"title","Only Title:",0,0);
    type = construct_varname(fields,n++,name,"type","Only Type:",0,0);
    minprice = construct_varname(fields,n++,name,"minprice","Min Price (in NXT):",0,0);
    maxprice = construct_varname(fields,n++,name,"maxprice","Max Price (in NXT):",0,0);
    sprintf(vars,"\"seller\":\"' + %s + '\",\"buyer\":\"' + %s + '\",\"title\":\"' + %s + '\",\"type\":\"' + %s + '\",\"minprice\":\"' + %s + '\",\"maxprice\":\"' + %s + '\"",seller,buyer,title,type,minprice,maxprice);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",\"filters\":{%s}}';\n}\n",name,handler,name,NXTaddr,vars);
    *scriptp = clonestr(script);
    free(seller); free(buyer); free(title); free(type); free(minprice); free(maxprice);
    return(n);
}

int gen_changeurl_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*url;
    url = construct_varname(fields,n++,name,"changeurl","Change location of NXTprotocol.html",0,0);
    sprintf(vars,"\"URL\":\"' + %s + '\"",url);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",%s}';\n}\n",name,handler,name,vars);
    *scriptp = clonestr(script);
    free(url);
    return(n);
}

int gen_listingid_field(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*listingid;
    listingid = construct_varname(fields,n++,name,"listingid","Listing ID:",0,0);
    sprintf(vars,"\"listingid\":\"' + %s + '\"",listingid);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",%s}';\n}\n",name,handler,name,NXTaddr,vars);
    *scriptp = clonestr(script);
    free(listingid);
    return(n);
}

int gen_acceptoffer_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*listingid,*buyer;
    listingid = construct_varname(fields,n++,name,"listingid","Listing ID:",0,0);
    buyer = construct_varname(fields,n++,name,"buyer","NXT address:",0,0);
    sprintf(vars,"\"listingid\":\"' + %s + '\",\"buyer\":\"' + %s + '\"",listingid,buyer);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",%s}';\n}\n",name,handler,name,NXTaddr,vars);
    *scriptp = clonestr(script);
    free(listingid); free(buyer);
    return(n);
}

int gen_makeoffer_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*listingid,*bid,*comments;
    listingid = construct_varname(fields,n++,name,"listingid","Listing ID:",0,0);
    bid = construct_varname(fields,n++,name,"bid","amount or {NXTsubatomic}",80,0);
    comments = construct_varname(fields,n++,name,"comments","comments:",30,5);
    sprintf(vars,"\"listingid\":\"' + %s + '\",\"bid\":\"' + %s + '\",\"comments\":\"' + %s + '\"",listingid,bid,comments);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",%s}';\n}\n",name,handler,name,NXTaddr,vars);
    *scriptp = clonestr(script);
    free(listingid); free(bid); free(comments);
    return(n);
}

int gen_feedback_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*listingid,*rating,*aboutbuyer,*aboutseller;
    listingid = construct_varname(fields,n++,name,"listingid","Listing ID:",0,0);
    rating = construct_varname(fields,n++,name,"rating","Rating:",0,0);
    aboutbuyer = construct_varname(fields,n++,name,"aboutbuyer","Feedback about buyer:",30,5);
    aboutseller = construct_varname(fields,n++,name,"aboutseller","Feedback about seller:",30,5);
    sprintf(vars,"\"listingid\":\"' + %s + '\",\"rating\":\"' + %s +'\",\"aboutbuyer\":\"' + %s + '\",\"aboutseller\":\"' + %s + '\"",listingid,rating,aboutbuyer,aboutseller);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",%s}';\n}\n",name,handler,name,NXTaddr,vars);
    *scriptp = clonestr(script);
    free(listingid); free(rating); free(aboutbuyer); free(aboutseller);
    return(n);
}

int NXTorrent_forms(char *NXTaddr,char **forms,char **scripts)
{
    int n = 0;
    forms[n] = make_form(NXTaddr,&scripts[n],"changeurl","Change this websockets default page","change","127.0.0.1:7777","NXTorrent",gen_changeurl_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"list","Post an Item for Sale: ","List this item","127.0.0.1:7777","NXTorrent",gen_list_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"status","Display Listing ID:","display","127.0.0.1:7777","NXTorrent",gen_listingid_field);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"listings","Search Listings:","search","127.0.0.1:7777","NXTorrent",gen_listings_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"cancel","Cancel Listing ID:","cancel listing","127.0.0.1:7777","NXTorrent",gen_listingid_field);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"acceptoffer","Accept Offer for Listing ID:","accept offer","127.0.0.1:7777","NXTorrent",gen_acceptoffer_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"makeoffer","Make Offer for Listing ID:","make offer","127.0.0.1:7777","NXTorrent",gen_makeoffer_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"feedback","Feedback for Listing ID:","send feedback","127.0.0.1:7777","NXTorrent",gen_feedback_fields);
    n++;
    return(n);
}

#ifdef BTC_COINID
char *gen_coinacct_line(char *buf,int32_t coinid,uint64_t nxt64bits,char *NXTaddr)
{
    char *withdraw,*deposit,*str;
    deposit = get_deposit_addr(coinid,NXTaddr);
    if ( deposit == 0 || deposit[0] == 0 )
        deposit = "<no deposit address>";
    withdraw = find_user_withdrawaddr(coinid,nxt64bits);
    if ( withdraw == 0 || withdraw[0] == 0 )
    {
        str = "set";
        withdraw = "<no withdraw address>";
    } else str = "change";
    sprintf(buf,"deposit %4s -> (%s)<br/> %s %s withdraw address %s ->",coinid_str(coinid),deposit,str,coinid_str(coinid),withdraw);
    return(buf);
}

int gen_register_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    uint64_t nxt64bits;
    char script[16384],vars[1024],buf[512],*btc,*ltc,*doge,*cgb,*drk;
    nxt64bits = calc_nxt64bits(NXTaddr);
    btc = construct_varname(fields,n++,name,"btc",gen_coinacct_line(buf,BTC_COINID,nxt64bits,NXTaddr),60,1);
    ltc = construct_varname(fields,n++,name,"ltc",gen_coinacct_line(buf,LTC_COINID,nxt64bits,NXTaddr),60,1);
    doge = construct_varname(fields,n++,name,"doge",gen_coinacct_line(buf,DOGE_COINID,nxt64bits,NXTaddr),60,1);
    drk = construct_varname(fields,n++,name,"drk",gen_coinacct_line(buf,DRK_COINID,nxt64bits,NXTaddr),60,1);
    cgb = construct_varname(fields,n++,name,"cgb",gen_coinacct_line(buf,CGB_COINID,nxt64bits,NXTaddr),60,1);
    sprintf(vars,"\"BTC\":\"' + %s + '\",\"LTC\":\"' + %s + '\",\"DOGE\":\"' + %s + '\",\"DRK\":\"' + %s + '\",\"CGB\":\"' + %s + '\"",btc,ltc,doge,drk,cgb);
    //printf("vars.(%s)\n",vars);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",\"coins\":{%s}}';\n}\n",name,handler,name,NXTaddr,vars);
    *scriptp = clonestr(script);
    free(btc); free(ltc); free(doge); free(drk); free(cgb);
    return(n);
}

int gen_dispNXTacct_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*coin,*assetid;
    coin = construct_varname(fields,n++,name,"coin","Specify coin:",0,0);
    assetid = construct_varname(fields,n++,name,"assetid","Specify assetid:",0,0);
    sprintf(vars,"\"coin\":\"' + %s + '\",\"assetid\":\"' + %s + '\"",coin,assetid);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",%s}';\n}\n",name,handler,name,NXTaddr,vars);
    *scriptp = clonestr(script);
    free(coin); free(assetid);
    return(n);
}

int gen_dispcoininfo_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*coin,*nxtacct;
    coin = construct_varname(fields,n++,name,"coin","Specify coin:",0,0);
    nxtacct = construct_varname(fields,n++,name,"nxtacct","Specify NXTaddr (blank for all):",0,0);
    sprintf(vars,"\"coin\":\"' + %s + '\",\"assetid\":\"' + %s + '\"",coin,nxtacct);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",%s}';\n}\n",name,handler,name,NXTaddr,vars);
    *scriptp = clonestr(script);
    free(coin); free(nxtacct);
    return(n);
}

int gen_redeem_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*coin,*amount,*destaddr,*InstantDEX;
    coin = construct_varname(fields,n++,name,"coin","Specify coin:",0,0);
    amount = construct_varname(fields,n++,name,"amount","Specify withdrawal amount:",0,0);
    destaddr = construct_varname(fields,n++,name,"destaddr","Specify destination address (blank to use registered):",60,1);
    InstantDEX = construct_varname(fields,n++,name,"InstantDEX","Specify amount for InstantDEX:",0,0);
    sprintf(vars,"\"redeem\":\"' + %s + '\",\"withdrawaddr\":\"' + %s + '\",\"InstantDEX\":\"' + %s + '\"",coin,destaddr,InstantDEX);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",\"coin\":\"' + %s + '\",\"amount\":\"' + %s + '\",\"comment\":{%s}}';\n}\n",name,handler,name,NXTaddr,coin,amount,vars);
    *scriptp = clonestr(script);
    free(coin); free(amount); free(destaddr); free(InstantDEX);
    return(n);
}

int multigateway_forms(char *NXTaddr,char **forms,char **scripts)
{
    int n = 0;
   // forms[n] = make_form(NXTaddr,&scripts[n],"changeurl","Change this websockets default page","change","127.0.0.1:7777","multigateway",gen_changeurl_fields);
   // n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"genDepositaddrs","Update coin addresses","submit","127.0.0.1:7777","multigateway",gen_register_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"dispNXTacct","Display account details","display","127.0.0.1:7777","multigateway",gen_dispNXTacct_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"dispcoininfo","Display coin details","display","127.0.0.1:7777","multigateway",gen_dispcoininfo_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"redeem","Redeem NXT Asset -> coin","withdraw","127.0.0.1:7777","multigateway",gen_redeem_fields);
    n++;
    return(n);
}

//static char *subatomic_trade[] = { (char *)subatomic_trade_func, "trade", "", "NXT", "coin", "amount", "coinaddr", "otherNXT", "othercoin", "otheramount", "othercoinaddr", "pubkey", "otherpubkey", 0 };
//var listingTitle = document.listingValues.listingTitle.value;
//var listingCategory = document.listingValues.listingCategory.value;
//var listingCategory2 = document.listingValues.listingCategory2.value;
//var listingCategory3 = document.listingValues.listingCategory3.value;
//var listingDescription = document.listingValues.listingDescription.value;
//var listingPrice = document.listingValues.listingPrice.value;

int gen_subatomic_tradefields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],vars[1024],*destNXT,*coin,*amount,*coinaddr,*destcoin,*destamount,*destcoinaddr;//,*senderip;
    //senderip = construct_varname(fields,n++,name,"senderip","your IP address:",0,0);

    coin = construct_varname(fields,n++,name,"coin","source coin or NXT for atomic swap:",0,0);
    amount = construct_varname(fields,n++,name,"amount","source amount or mysignaturehash:",64,1);
    coinaddr = construct_varname(fields,n++,name,"coinaddr","source coinaddress or unsigned txbytes:",64,5);

    destcoin = construct_varname(fields,n++,name,"destcoin","dest coin or NXT for atomic swap:",0,0);
    destamount = construct_varname(fields,n++,name,"destamount","dest amount or othersighash:",64,1);
    destcoinaddr = construct_varname(fields,n++,name,"destcoinaddr","dest coinaddress or unsigned txbytes:",64,5);

    destNXT = construct_varname(fields,n++,name,"destNXT","NXT address you are trading with:",0,0);
    sprintf(script,"function click_%s()\n{\nvar A = %s; B = %s; C = %s; D = %s; E = %s; F = %s; G = %s;\n",name,coin,amount,coinaddr,destNXT,destcoin,destamount,destcoinaddr);
    sprintf(vars,"\"coinaddr\":\"' + C + '\",\"destNXT\":\"' + D + '\",\"destcoin\":\"' + E + '\",\"destamount\":\"' + F + '\",\"destcoinaddr\":\"' + G + '\"");
    sprintf(script+strlen(script),"location.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",\"coin\":\"' + A + '\",\"amount\":\"' + B + '\",%s}';\n}\n",handler,name,NXTaddr,vars);
    *scriptp = clonestr(script);
    free(coin); free(amount); free(coinaddr);
    free(destNXT); free(destcoin); free(destamount); free(destcoinaddr);
    return(n);
}

int gen_subatomic_statusfields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384];//,vars[1024],*listingid;
   // listingid = construct_varname(fields,n++,name,"listingid","Listing ID:",0,0);
    //sprintf(vars,"\"listingid\":\"' + %s +'\"",listingid);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\"}';\n}\n",name,handler,name,NXTaddr);
    *scriptp = clonestr(script);
   // free(listingid);
    return(n);
}

int gen_subatomic_cancelfields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384];//,vars[1024],*listingid;
    // listingid = construct_varname(fields,n++,name,"listingid","Listing ID:",0,0);
    //sprintf(vars,"\"listingid\":\"' + %s +'\"",listingid);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\"}';\n}\n",name,handler,name,NXTaddr);
    *scriptp = clonestr(script);
    // free(listingid);
    return(n);
}

int subatomic_forms(char *NXTaddr,char **forms,char **scripts)
{
    int n = 0;
    forms[n] = make_form(NXTaddr,&scripts[n],"subatomic_trade","Subatomic crypto trade or NXT Atomic swap","exchange","127.0.0.1:7777","subatomic",gen_subatomic_tradefields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"subatomic_status","Subatomic status","display","127.0.0.1:7777","subatomic",gen_subatomic_statusfields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"subatomic_cancel","Cancel Subatomic trade","cancel","127.0.0.1:7777","subatomic",gen_subatomic_cancelfields);
    n++;
    return(n);
}

int gen_nodecoin_cashout_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384];
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\"}';\n}\n",name,handler,name,NXTaddr);
    *scriptp = clonestr(script);
    return(n);
}

int gen_acctassets_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*assetid,*maxtimestamp,*txlog;
    assetid = construct_varname(fields,n++,name,"assetid","assetid:",0,0);
    maxtimestamp = construct_varname(fields,n++,name,"maxtimestamp","maxtimestamp 0 for latest:",0,0);
    txlog = construct_varname(fields,n++,name,"txlog","enable tx log:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",\"assetid\":\"' + %s + '\",\"maxtimestamp\":\"' + %s + '\",\"txlog\":\"' + %s + '\"}';\n}\n",name,handler,name,NXTaddr,assetid,maxtimestamp,txlog);
    *scriptp = clonestr(script);
    free(assetid); free(maxtimestamp); free(txlog);
    return(n);
}

int gen_dividends_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*assetid,*maxtimestamp,*dividend,*dividendid,*payout;
    assetid = construct_varname(fields,n++,name,"assetid","assetid:",0,0);
    maxtimestamp = construct_varname(fields,n++,name,"maxtimestamp","maxtimestamp 0 for latest:",0,0);
    dividendid = construct_varname(fields,n++,name,"dividendid","assetid for dividend (0 for NXT):",0,0);
    dividend = construct_varname(fields,n++,name,"dividend","dividend amount:",0,0);
    payout = construct_varname(fields,n++,name,"payout","1 to display tx, WARNING 2 will payout dividends:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\",\"assetid\":\"' + %s + '\",\"maxtimestamp\":\"' + %s + '\",\"dividendid\":\"' + %s + '\",\"dividend\":\"' + %s + '\",\"payout\":\"' + %s + '\"}';\n}\n",name,handler,name,NXTaddr,assetid,maxtimestamp,dividendid,dividend,payout);
    *scriptp = clonestr(script);
    free(assetid); free(maxtimestamp); free(dividend); free(dividendid); free(payout);
    return(n);
}

int NXTcoinsco_forms(char *NXTaddr,char **forms,char **scripts)
{
    int n = 0;
    char buf[512];
    sprintf(buf,"Nodecoins available %.8f (cashout costs 2 NXT)",dstr(get_nodecoins_avail()));
    forms[n] = make_form(NXTaddr,&scripts[n],"nodecoin_cashout",buf,"cashout","127.0.0.1:7777","NXTcoinsco",gen_nodecoin_cashout_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"disp_acctassets","Get account history","get acct","127.0.0.1:7777","NXTcoinsco",gen_acctassets_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"calc_dividends","Generate dividend list","calc dividends","127.0.0.1:7777","NXTcoinsco",gen_dividends_fields);
    n++;

    return(n);
}
#endif

int gen_pNXT_cashout_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384];
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"NXT\":\"%s\"}';\n}\n",name,handler,name,NXTaddr);
    *scriptp = clonestr(script);
    return(n);
}

int pNXT_forms(char *NXTaddr,char **forms,char **scripts)
{
    char *get_pNXT_addr();
    uint64_t get_pNXT_confbalance();
    uint64_t get_pNXT_rawbalance();
    int n = 0;
    char buf[512];
    sprintf(buf,"pNXT %s available raw %.8f confirmed %.8f",get_pNXT_addr(),dstr(get_pNXT_rawbalance()),dstr(get_pNXT_confbalance()));
    forms[n] = make_form(NXTaddr,&scripts[n],"cashout",buf,"cashout","127.0.0.1:7777","pNXT",gen_pNXT_cashout_fields);
    n++;
    return(n);
}

char *teststr = "<!DOCTYPE html>\
<html>\
<head>\
<meta charset=\"UTF-8\"/>\
<title>NXTprotocol dev GUI</title>\
<article>\
<section class=\"browser\">NXTprotocol detected Browser: <div id=brow>...</div></section><BR><BR>\
<section id=\"increment\" class=\"group2\">\
<table>\
<BR><BR>\
<div id=number> </div>\
<td>Your websocket connection status:</td>\
<td id=wsdi_statustd align=center class=\"explain\"><div id=wsdi_status>Not initialized</div></td>\
<BR>\
</table>\
</td></tr></table>\
</section>\
</article>";

char *endstr = "<script>\n\
var pos = 0;\n\
function get_appropriate_ws_url()\n\
{\n\
    var pcol;\n\
    var u = document.URL;\n\
    if (u.substring(0, 5) == \"https\") {\n\
        pcol = \"wss://\";\n\
        u = u.substr(8);\n\
    } else {\
        pcol = \"ws://\";\n\
        if (u.substring(0, 4) == \"http\")\n\
            u = u.substr(7);\n\
    }\
    u = u.split('/');\n\
    return pcol + u[0] + \"/xxx\";\n\
}\n\
\n\
document.getElementById(\"number\").textContent = get_appropriate_ws_url();\
var socket_di;\n\
if (typeof MozWebSocket != \"undefined\") {\n\
    socket_di = new MozWebSocket(get_appropriate_ws_url(),\n\
                                 \"dumb-increment-protocol\");\n\
} else {\n\
    socket_di = new WebSocket(get_appropriate_ws_url(),\n\
                              \"dumb-increment-protocol\");\n\
}\n\
try {\n\
    socket_di.onopen = function() {\n\
        document.getElementById(\"wsdi_statustd\").style.backgroundColor = \"#40ff40\";\n\
        document.getElementById(\"wsdi_status\").textContent = \"OPEN\";\n\
    }\n\
    socket_di.onmessage =function got_packet(msg) {\n\
        document.getElementById(\"number\").textContent = msg.data + \"\\n\";\n\
    }\n\
    socket_di.onclose = function(){\n\
        document.getElementById(\"wsdi_statustd\").style.backgroundColor = \"#ff4040\";\n\
        document.getElementById(\"wsdi_status\").textContent = \"CLOSED\";\n\
    }\n\
    } catch(exception) {\n\
        alert('<p>Error' + exception);\n\
    }\n\
    </script>\n\
    </html>";


void gen_testforms()
{
    //int32_t coinid;
    char *str;//,*depositaddr,buf[4096];
    //int64_t quantity,unconfirmed;
#ifdef MAINNET
    char *netstr = "MAINNET";
#else
    char *netstr = "TESTNET";
#endif
    sprintf(testforms,"%s %s Finished_loading.%d Historical_done.%d <br/><b>%s <br/> %s <br/>NXT.%s balance %.8f %s</b><br/>\n",teststr,netstr,Finished_loading,Historical_done,Global_mp->dispname,PC_USERNAME[0]!=0?PC_USERNAME:"setAccountInfo on http://127.0.0.1:6876/test description={\"username\":\"your pc username\"}",Global_mp->NXTADDR,dstr(Global_mp->acctbalance),Global_mp->acctbalance == 0?"<- need to send NXT":"");
    sprintf(testforms+strlen(testforms),"<br/><a href=\"https://coinomat.com/~jamesjl777\">Send NXT -> your Visa/Mastercard</a href>");
    str = gen_handler_forms(Global_mp->NXTADDR,"pNXT","privateNXT API test forms",pNXT_forms);
    strcat(testforms,str);
    free(str);
 
#ifdef BTC_COINID
    str = gen_handler_forms(Global_mp->NXTADDR,"NXTcoinsco","NXTcoins.co API test forms",NXTcoinsco_forms);
    strcat(testforms,str);
    free(str);
    strcpy(buf,"Deposit coinaddrs ");
    for (coinid=0; coinid<64; coinid++)
    {
        if ( strcmp(coinid_str(coinid),ILLEGAL_COIN) != 0 )
        {
            depositaddr = get_deposit_addr(coinid,Global_mp->NXTADDR);
            if ( depositaddr != 0 )
            {
                quantity = get_coin_quantity(Global_mp->curl_handle,&unconfirmed,coinid,Global_mp->NXTADDR);
                //if ( quantity != 0 || unconfirmed != 0 )
                {
                    sprintf(buf+strlen(buf),"(%s %s %.8f) ",coinid_str(coinid),depositaddr,dstr(quantity));
                }
            }
        }
    }
    strcat(buf,"<br/><br/>\n");
    strcat(testforms,buf);

    str = gen_handler_forms(Global_mp->NXTADDR,"multigateway","multigateway API test forms",multigateway_forms);
    if ( str != 0 )
    {
        strcat(testforms,str);
        free(str);
    }
    else printf("error generating multigateway forms\n");

    str = gen_handler_forms(Global_mp->NXTADDR,"NXTorrent","NXTorrent API test forms",NXTorrent_forms);
    strcat(testforms,str);
    free(str);
    
    str = gen_handler_forms(Global_mp->NXTADDR,"subatomic","subatomic API test forms",subatomic_forms);
    strcat(testforms,str);
    free(str);
#endif
    
    strcat(testforms,endstr);
    
}
#endif
