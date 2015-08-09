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
    sprintf(str,"<td colspan=\"2\"> <input type=\"button\" value=\"%s\" onclick=\"click_%s()\" /></td> </tr>\n</table></form><br/>",button,name);
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

#ifdef INSIDE_MGW
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

int gen_pNXT_select_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*ipaddr,*port,*secret;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    //server = construct_varname(fields,n++,name,"server","NXT address:",0,0);
    ipaddr = construct_varname(fields,n++,name,"ipaddr","or ipaddr address:",0,0);
    port = construct_varname(fields,n++,name,"port","port: (blank for default)",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"ipaddr\":\"' + %s + '\",\"port\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,ipaddr,port);
    *scriptp = clonestr(script);
    free(secret); free(ipaddr); free(port);
    return(n);
}

int gen_pNXT_pubkey_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*addr,*secret;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    addr = construct_varname(fields,n++,name,"addr","address (NXT, BTCD, pNXT or BTC):",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"addr\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,addr);
    *scriptp = clonestr(script);
    free(secret); free(addr);
    return(n);
}

int gen_pNXT_depwith_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*amount,*secret;
    amount = construct_varname(fields,n++,name,"amount","amount:",0,0);
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"amount\":\"' + %s + '\",\"secret\":\"' + %s + '\"}';\n}\n",name,handler,name,amount,secret);
    *scriptp = clonestr(script);
    free(amount); free(secret);
    return(n);
}

int gen_pNXT_send_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*dest,*amount,*level,*paymentid,*secret;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    dest = construct_varname(fields,n++,name,"dest","dest pNXT address:",0,0);
    amount = construct_varname(fields,n++,name,"amount","amount:",0,0);
    level = construct_varname(fields,n++,name,"level","privacy level (0 or more):",0,0);
    paymentid = construct_varname(fields,n++,name,"paymentid","paymentid:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"dest\":\"' + %s + '\",\"amount\":\"' + %s + '\",\"level\":\"' + %s + '\",\"paymentid\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,dest,amount,level,paymentid);
    *scriptp = clonestr(script);
    free(secret); free(dest); free(amount); free(level); free(paymentid);
    return(n);
}

int gen_pNXT_teleport_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*dest,*amount,*secret,*coin,*minage,*M,*N;
    secret = construct_varname(fields,n++,name,"secret","NXT secret:",0,0);
    dest = construct_varname(fields,n++,name,"dest","dest account (BTCD, NXT or BTC):",0,0);
    coin = construct_varname(fields,n++,name,"coin","coin:",0,0);
    amount = construct_varname(fields,n++,name,"amount","amount:",0,0);
    minage = construct_varname(fields,n++,name,"minage","minimum block age for telepods:",0,0);
    M = construct_varname(fields,n++,name,"M","M(inimum signers):",0,0);
    N = construct_varname(fields,n++,name,"N","from N:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"dest\":\"' + %s + '\",\"amount\":\"' + %s + '\",\"coin\":\"' + %s + '\",\"minage\":\"' + %s + '\",\"M\":\"' + %s + '\",\"N\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,dest,amount,coin,minage,M,N);
    *scriptp = clonestr(script);
    free(secret); free(dest); free(amount); free(coin); free(minage); free(M); free(N);
    return(n);
}

int gen_pNXT_buy_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*dest,*amount,*level,*maxrate,*secret;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    dest = construct_varname(fields,n++,name,"dest","dest BTC address (blank to get mgwBTC asset):",0,0);
    amount = construct_varname(fields,n++,name,"amount","amount:",0,0);
    level = construct_varname(fields,n++,name,"level","privacy level (0 or more):",0,0);
    maxrate = construct_varname(fields,n++,name,"maxrate","maximum NXT/BTC price:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"dest\":\"' + %s + '\",\"amount\":\"' + %s + '\",\"level\":\"' + %s + '\",\"maxrate\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,dest,amount,level,maxrate);
    *scriptp = clonestr(script);
    free(secret); free(dest); free(amount); free(level); free(maxrate);
    return(n);
}

int gen_pNXT_sell_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*dest,*amount,*level,*minrate,*duration,*secret;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    dest = construct_varname(fields,n++,name,"dest","dest NXT address (blank to get pNXT cryptonotes):",0,0);
    amount = construct_varname(fields,n++,name,"amount","amount:",0,0);
    duration = construct_varname(fields,n++,name,"duration","duration in hours:",0,0);
    level = construct_varname(fields,n++,name,"level","privacy level (0 or more):",0,0);
    minrate = construct_varname(fields,n++,name,"minrate","minimum NXT/BTC price:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"dest\":\"' + %s + '\",\"amount\":\"' + %s + '\",\"duration\":\"' + %s + '\",\"level\":\"' + %s + '\",\"minrate\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,dest,amount,duration,level,minrate);
    *scriptp = clonestr(script);
    free(dest); free(amount); free(level); free(minrate); free(duration); free(secret);
    return(n);
}

int gen_pNXT_getorderbooks_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*secret;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\"}';\n}\n",name,handler,name,secret);
    *scriptp = clonestr(script);
    free(secret);
    return(n);
}

int gen_pNXT_getmsg_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*secret,*sender;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    sender = construct_varname(fields,n++,name,"sender","sender's NXT address (blank for any):",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"sender\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,sender);
    *scriptp = clonestr(script);
    free(secret); free(sender);
    return(n);
}

int gen_pNXT_sendmsg_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*secret,*dest,*msg,*L;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    dest = construct_varname(fields,n++,name,"dest","dest NXT address:",0,0);
    msg = construct_varname(fields,n++,name,"msg","encrypted message:",60,0);
    L = construct_varname(fields,n++,name,"L","max additional onion layers:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"dest\":\"' + %s + '\",\"msg\":\"' + %s + '\",\"L\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,dest,msg,L);
    *scriptp = clonestr(script);
    free(secret); free(dest); free(msg); free(L);
    return(n);
}

int gen_pNXT_orderbook_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*obookid,*polarity,*allfields,*secret;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    obookid = construct_varname(fields,n++,name,"obookid","orderbook:",0,0);
    polarity = construct_varname(fields,n++,name,"polarity","polarity:",0,0);
    allfields = construct_varname(fields,n++,name,"allfields","allfields:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"obookid\":\"' + %s + '\",\"polarity\":\"' + %s + '\",\"allfields\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,obookid,polarity,allfields);
    *scriptp = clonestr(script);
    free(obookid); free(polarity); free(allfields); free(secret);
    return(n);
}

int gen_pNXT_makeoffer_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*secret,*qtyA,*qtyB,*assetA,*assetB,*type,*other;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    other = construct_varname(fields,n++,name,"other","other party's NXT address:",0,0);
    assetA = construct_varname(fields,n++,name,"assetA","assetA:",0,0);
    qtyA = construct_varname(fields,n++,name,"qtyA","qtyA:",0,0);
    assetB = construct_varname(fields,n++,name,"assetB","assetB:",0,0);
    qtyB = construct_varname(fields,n++,name,"qtyB","qtyB:",0,0);
    type = construct_varname(fields,n++,name,"type","type:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"other\":\"' + %s + '\",\"assetA\":\"' + %s + '\",\"qtyA\":\"' + %s + '\",\"assetB\":\"' + %s + '\",\"qtyB\":\"' + %s + '\",\"type\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,other,assetA,qtyA,assetB,qtyB,type);
    *scriptp = clonestr(script);
    free(secret); free(other), free(assetA); free(qtyA); free(assetB); free(qtyB); free(type);
    return(n);
}

int gen_pNXT_placequote_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*secret,*obookid,*polarity,*volume,*price,*assetA,*assetB;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    obookid = construct_varname(fields,n++,name,"obookid","orderbook:",0,0);
    polarity = construct_varname(fields,n++,name,"polarity","polarity:",0,0);
    volume = construct_varname(fields,n++,name,"volume","volume:",0,0);
    price = construct_varname(fields,n++,name,"price","price:",0,0);
    assetA = construct_varname(fields,n++,name,"assetA","assetA:",0,0);
    assetB = construct_varname(fields,n++,name,"assetB","assetB:",0,0);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"obookid\":\"' + %s + '\",\"polarity\":\"' + %s + '\",\"volume\":\"' + %s + '\",\"price\":\"' + %s + '\",\"assetA\":\"' + %s + '\",\"assetB\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,obookid,polarity,volume,price,assetA,assetB);
    *scriptp = clonestr(script);
    free(secret); free(obookid); free(polarity); free(volume); free(price); free(assetA); free(assetB);
    return(n);
}

int gen_pNXT_tradebot_fields(char *NXTaddr,char *handler,char *name,char **fields,char **scriptp)
{
    int n = 0;
    char script[16384],*secret,*code;
    secret = construct_varname(fields,n++,name,"secret","secret:",0,0);
    code = construct_varname(fields,n++,name,"tradebot","tradebot code JSON or (filename):",100,10);
    sprintf(script,"function click_%s()\n{\n\tlocation.href = 'http://127.0.0.1:7777/%s?{\"requestType\":\"%s\",\"secret\":\"' + %s + '\",\"code\":\"' + %s + '\"}';\n}\n",name,handler,name,secret,code);
    *scriptp = clonestr(script);
    free(secret); free(code);
    return(n);
}

int pNXT_forms(char *NXTaddr,char **forms,char **scripts)
{
    char *get_pNXT_addr();
    //uint64_t get_pNXT_confbalance();
    //uint64_t get_pNXT_rawbalance();
    //uint64_t get_privateNXT_balance(char *NXTaddr);
    int n = 0;
    //char buf[512];
    forms[n] = make_form(NXTaddr,&scripts[n],"select","select my privacyServer","choose server","127.0.0.1:7777","pNXT",gen_pNXT_select_fields);
    n++;
    
    forms[n] = make_form(NXTaddr,&scripts[n],"makeoffer","make offer","offer","127.0.0.1:7777","pNXT",gen_pNXT_makeoffer_fields);
    n++;
    
    forms[n] = make_form(NXTaddr,&scripts[n],"tradebot","launch tradebot","startbot","127.0.0.1:7777","pNXT",gen_pNXT_tradebot_fields);
    n++;
    
    forms[n] = make_form(NXTaddr,&scripts[n],"sendmessage","send encrypted message to NXT address","sendmsg","127.0.0.1:7777","pNXT",gen_pNXT_sendmsg_fields);
    n++;
    
    forms[n] = make_form(NXTaddr,&scripts[n],"checkmessages","check for encrypted messages from NXT address","check","127.0.0.1:7777","pNXT",gen_pNXT_getmsg_fields);
    n++;
    
    forms[n] = make_form(NXTaddr,&scripts[n],"getorderbooks","get all orderbooks","get orderbooks","127.0.0.1:7777","pNXT",gen_pNXT_getorderbooks_fields);
    n++;
    
    forms[n] = make_form(NXTaddr,&scripts[n],"orderbook","get orderbook","orderbook","127.0.0.1:7777","pNXT",gen_pNXT_orderbook_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"placebid","place bid","bid","127.0.0.1:7777","pNXT",gen_pNXT_placequote_fields);
    n++;
    
    forms[n] = make_form(NXTaddr,&scripts[n],"placeask","place ask","ask","127.0.0.1:7777","pNXT",gen_pNXT_placequote_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"teleport","teleport funds","teleport","127.0.0.1:7777","pNXT",gen_pNXT_teleport_fields);
    n++;

    //sprintf(buf,"pNXT cryptnote address \"%s\" has raw %.8f confirmed %.8f",get_pNXT_addr(),dstr(get_pNXT_rawbalance()),dstr(get_pNXT_confbalance()));
    //forms[n] = make_form(NXTaddr,&scripts[n],"sellpNXT",buf,"sell pNXT","127.0.0.1:7777","pNXT",gen_pNXT_depwith_fields);
    //n++;
    //sprintf(buf,"NXT address \"%s\" has %.8f privateNXT assets",NXTaddr,dstr(get_privateNXT_balance(NXTaddr)));
    forms[n] = make_form(NXTaddr,&scripts[n],"buypNXT","convert NXT into pNXT cryptonotes","buy pNXT","127.0.0.1:7777","pNXT",gen_pNXT_depwith_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"send","send to another pNXT cryptonote address (low level pNXT to pNXT)","send pNXT","127.0.0.1:7777","pNXT",gen_pNXT_send_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"buy","buy mgwBTC below maximum price and send to BTC address","send to BTC addr","127.0.0.1:7777","pNXT",gen_pNXT_buy_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"sell","sell mgwBTC above minimum price and send privateNXT to NXT address","sell mgwBTC","127.0.0.1:7777","pNXT",gen_pNXT_sell_fields);
    n++;
    forms[n] = make_form(NXTaddr,&scripts[n],"getpubkey","get public key for address","get pubkey","127.0.0.1:7777","pNXT",gen_pNXT_pubkey_fields);
    n++;
  
    return(n);
}

char *teststr = "<!DOCTYPE html>\
<html>\
<head>\
<script language=\"JavaScript\">\
function refreshIt(element) {\
    setTimeout(function() {\
        element.src = element.src.split('?')[0] + '?' + new Date().getTime();\
    }, 1000);\
}\
</script>\
<meta charset=\"UTF-8\"/>\
<title>NXTservices API form</title>\
<article>\
<section class=\"browser\">NXTservices detected Browser: <div id=brow>...</div></section><BR><BR>\
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
<img src=\"BTCD_BTC.jpg\" name=\"BTCD_BTC\" onload=\"refreshIt(this)\">\
<img src=\"bittrex_BTCD_BTC.jpg\" name=\"bittrex_BTCD_BTC\" onload=\"refreshIt(this)\">\
<img src=\"cryptsy_BTCD_BTC.jpg\" name=\"cryptsy_BTCD_BTC\" onload=\"refreshIt(this)\">\
<img src=\"BTC_NXT.jpg\" name=\"BTC_NXT\" onload=\"refreshIt(this)\">\
<img src=\"bter_NXT_BTC.jpg\" name=\"bter_NXT_BTC\" onload=\"refreshIt(this)\">\
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
<body>\
</body>\
    </html>";

void gen_testforms(char *NXTACCTSECRET)
{
    struct NXThandler_info *mp = Global_mp;
    //int32_t coinid;
    //static unsigned int *bitmap;
    static int counter;
    uint64_t nxt64bits;
    char *str,NXTADDR[64];//,*depositaddr,buf[4096];
    //int64_t quantity,unconfirmed;
#ifdef MAINNET
    char *netstr = "MAINNET";
#else
    char *netstr = "TESTNET";
#endif
    counter++;
    //if ( bitmap == 0 )
    //    bitmap = calloc(Screenheight*Screenwidth,sizeof(*bitmap));
    //output_jpg("testimage",bitmap,.001);
    //load_testimage("testimage.jpg");
    NXTADDR[0] = 0;
    if ( NXTACCTSECRET != 0 )
    {
        nxt64bits = issue_getAccountId(0,NXTACCTSECRET);
        if ( nxt64bits != 0 )
        {
            expand_nxt64bits(NXTADDR,nxt64bits);
            if ( mp->accountjson != 0 )
                free_json(mp->accountjson);
            mp->accountjson = issue_getAccountInfo(mp->curl_handle,&Global_mp->acctbalance,mp->dispname,PC_USERNAME,NXTADDR,mp->groupname);
        }
    }
    sprintf(testforms,"%s %s counter.%d Finished_loading.%d Historical_done.%d <br/> <br/><br/>\n",teststr,netstr,counter,Finished_loading,Historical_done);
    sprintf(testforms+strlen(testforms),"<br/><a href=\"https://coinomat.com/~jamesjl777\">Send NXT -> your Visa/Mastercard</a href>");
    str = gen_handler_forms(NXTADDR,"pNXT","pNXT API test forms",pNXT_forms);
    strcat(testforms,str);
    free(str);
 
#ifdef INSIDE_MGW
    str = gen_handler_forms(Global_mp->NXTADDR,"NXTcoinsco","NXTcoins.co API test forms",NXTcoinsco_forms);
    strcat(testforms,str);
    free(str);
    strcpy(buf,"Deposit coinaddrs ");
    for (coinid=0; coinid<MAX_MGWCOINS; coinid++)
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
