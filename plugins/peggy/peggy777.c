
#ifdef DEFINES_ONLY
#ifndef peggy777_h
#define peggy777_h

// derived from vps from CfB
#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include "utils/bits777.c"
#include "txind777.c"
#include "serdes777.c"
#include "accts777.c"
#include "opreturn777.c"

// CfB "the rule is simple = others can know the redemption day only AFTER the price for that day is set in stone."
#define PEGGY_NUMCOEFFS 539

#define HASH_SIZE 32
#define PEGGY_DAYTICKS (24 * 60 * 60)
#define PEGGY_HOURTICKS (60 * 60)
#define MAX_TIMEFRAME (24 * 3600 * 365)
#define MAX_PEGGYDAYS (365 * 10)
#define PEGGY_MINEXTRADAYS 3

#define PEGGY_MAXPRICEDPEGS 256
#define PEGGY_MAXPAIREDPEGS 4096
#define PEGGY_MAXPEGS (PEGGY_MAXPRICEDPEGS + PEGGY_MAXPAIREDPEGS)

#define PEGGY_MAXVOTERS 4096
#define PEGGY_MARGINMAX 100
#define PEGGY_MARGINLOCKDAYS 30
#define PEGGY_MARGINGAPDAYS 7

#define PEGGY_RATE_777 2052

struct peggy_time { uint32_t blocknum,blocktimestamp; };
struct peggy_units { int64_t num,numoppo; };
struct peggy_margin { int64_t deposits,margindeposits,marginvalue; };
struct peggy_description { char name[32],base[16],rel[16]; uint64_t basebits,relbits,assetbits; int16_t id,baseid,relid; int8_t hasprice,enabled; };
struct peggy_pool { struct peggy_margin funds; struct peggy_units liability; uint64_t quorum,decisionthreshold,mainunitsize,mainbits; };
//struct peggy_limits { int64_t scales[MAX_TIMEFRAMES],maxsupply,maxnetbalance; uint32_t timeframes[MAX_TIMEFRAMES],numtimeframes; };

struct peggy
{
    struct peggy_description name; struct peggy_pool pool; struct peggy_lock lockparms; int64_t maxsupply,maxnetbalance;
    struct price_resolution spread,mindenomination,genesisprice; uint32_t firsttimestamp,maxdailyrate,unitincr,peggymils;
    struct price_resolution *baseprices,*relprices;
};

struct peggy_pricedpeg
{
    struct peggy PEG;
    struct price_resolution prices[MAX_PEGGYDAYS]; // In main currency units
};

union peggy_pair { struct peggy PEG; struct peggy_pricedpeg pricedPEG; };

struct peggy_bet { struct price_resolution prediction; uint64_t distbet,dirbet,payout,shares,dist; uint32_t timestamp,minutes; };
struct peggy_vote { int32_t pval,tolerance; };//struct price_resolution price,tolerance; uint64_t nxt64bits,weight; };
struct peggy_entry
{
    int64_t total,costbasis,satoshis,royalty,fee,estimated_interest,interest_unlocked,interestpaid,supplydiff,denomination;
    int16_t dailyrate,baseid,relid,polarity;
    struct peggy_units supply; struct price_resolution price,oppoprice;
};

struct peggy_balances {  struct peggy_margin funds; int64_t privatebetfees,crypto777_royalty,APRfund,APRfund_reserved; };

struct peggy_info
{
    char maincurrency[16]; uint64_t basebits[256],mainbits,mainunitsize,quorum,decisionthreshold; int64_t hwmbalance,worstbalance,maxdrawdown;
    struct price_resolution default_spread; struct peggy_lock default_lockparms;
    struct peggy_balances bank,basereserves[256];
    int32_t default_dailyrate,interesttenths,posboost,negpenalty,feediv,feemult;
    int32_t numpegs,numpairedpegs,numpricedpegs,numopreturns,numvoters;
    struct accts777_info *accts;
    double btcusd,btcdbtc; char path[512],*genesis; uint32_t genesistime,BTCD_price0;
    struct peggy_vote votes[PEGGY_MAXPRICEDPEGS][PEGGY_MAXVOTERS];
    struct peggy *contracts[PEGGY_MAXPEGS];
    struct peggy pairedpegs[PEGGY_MAXPRICEDPEGS + PEGGY_MAXPAIREDPEGS];
    struct peggy_pricedpeg pricedpegs[PEGGY_MAXPRICEDPEGS];
};

struct price_resolution peggy_priceconsensus(struct peggy_info *PEGS,uint64_t seed,int16_t peg,struct peggy_time T,struct peggy_vote *votes,uint32_t numvotes,struct peggy_bet *bets,uint32_t numbets);
extern char CURRENCIES[][8];
uint64_t peggy_createunit(struct peggy_info *PEGS,struct peggy_unit *readU,uint64_t seed,struct peggy_time T,char *name,uint64_t nxt64bits,bits256 lockhash,struct peggy_lock *lock,uint64_t amount,uint64_t marginamount);
uint64_t peggy_redeem(struct peggy_info *PEGS,int32_t readonly,char *name,int32_t polarity,uint64_t nxt64bits,bits256 pubkey,uint16_t lockdays,uint8_t chainlen,struct peggy_time T);
void peggy_margincalls(struct peggy_info *PEGS,struct peggy *PEG,struct price_resolution newprice,struct price_resolution shortprice,struct peggy_time T);

struct price_resolution peggy_lastprice(struct peggy_info *PEGS,struct peggy *PEG,int32_t polarity,uint32_t firsttimestamp,struct peggy_time T);
struct price_resolution peggy_aveprice(struct peggy_info *PEGS,struct peggy *PEG,struct peggy_time T,int32_t extradays,int32_t polarity,int32_t numiters);

char *peggy_emitprices(int32_t *nonzp,struct peggy_info *PEGS,struct peggy_time T,int32_t maxlockdays);

struct peggy *peggy_find(struct peggy_entry *entry,struct peggy_info *PEGS,char *name,int32_t polarity);
char *peggy_aprstr(int64_t dailyrate);
uint64_t peggy_assetbits(char *name);
int64_t peggy_compound(int32_t dispflag,int64_t satoshis,int64_t dailyrate,int32_t n);
int32_t prices777_mindenomination(int32_t base);
struct price_resolution peggy_shortprice(struct peggy *PEG,struct price_resolution price);
uint32_t peggy_mils(int32_t i);
struct price_resolution peggy_scaleprice(struct price_resolution price,int64_t peggymils);
extern uint64_t peggy_smooth_coeffs[PEGGY_NUMCOEFFS];
extern int32_t Debuglevel;

#endif
#else
#ifndef peggy777_c
#define peggy777_c

#ifndef peggy777_h
#define DEFINES_ONLY
#include "peggy777.c"
#undef DEFINES_ONLY
#endif

int32_t prices777_mindenomination(int32_t);

#include "txind777.c"
#include "serdes777.c"
#include "accts777.c"

int32_t dailyrates[101] =
{
0, 27, 55, 82, 110, 137, 164, 192, 219, 246, 273, 300, 327, 355, 382, 409, 436, 463, 489, 516, 543, 570, 597, 624, 651, 677, 704, 731, 757, 784, 811, 837, 864, 890, 917, 943, 970, 996, 1023, 1049, 1076, 1102, 1128, 1155, 1181, 1207, 1233, 1259, 1286, 1312, 1338, 1364, 1390, 1416, 1442, 1468, 1494, 1520, 1546, 1572, 1598, 1624, 1649, 1675, 1701, 1727, 1752, 1778, 1804, 1830, 1855, 1881, 1906, 1932, 1957, 1983, 2008, 2034, 2059, 2085, 2110, 2136, 2161, 2186, 2212, 2237, 2262, 2287, 2313, 2338, 2363, 2388, 2413, 2438, 2463, 2488, 2513, 2538, 2563, 2588, 2613
};

int32_t peggy_aprpercs(int64_t dailyrate)
{
    int32_t i;
    if ( dailyrate == PEGGY_RATE_777 )
        return(777);
    else if ( dailyrate == -PEGGY_RATE_777 )
        return(-777);
    for (i=0; i<sizeof(dailyrates)/sizeof(*dailyrates)-1; i++)
        if ( dailyrate >= dailyrates[i] && dailyrate < dailyrates[i+1] )
            return(i*10);
    return(0);
}

char *peggy_aprstr(int64_t dailyrate)
{
    static char aprstr[16];
    int32_t apr,dir = 1;
    if ( dailyrate < 0 )
        dir = -1, dailyrate = -dailyrate;
    apr = peggy_aprpercs(dailyrate);
    sprintf(aprstr,"%c%d.%02d%% APR",dir<0?'-':'+',apr/100,(apr%100));
    return(aprstr);
}

int64_t peggy_compound(int32_t dispflag,int64_t satoshis,int64_t dailyrate,int32_t n)
{
    int32_t i; int64_t compounded = satoshis;
    if ( dailyrate == 0 )
        return(satoshis);
    if ( dispflag != 0 )
        printf("peggy_compound rate.%lld n.%d %.8f %lld %lld -> ",(long long)dailyrate,n,dstr(satoshis),(long long)compounded,(long long)SCALED_PRICE(compounded,dailyrate));
    for (i=0; i<n; i++)
    {
        //tmp = (compounded * (PRICE_RESOLUTION + dailyrate)) / PRICE_RESOLUTION; // WARNING: Do not use floating-point math!
        compounded += SCALED_PRICE(compounded,dailyrate);
        if ( dispflag != 0 )
            printf("%.8f ",dstr(compounded));
    }
    if ( dispflag != 0 )
        printf("%.8f %.8f\n",dstr(compounded),(double)compounded/satoshis);
    if ( compounded < 0 )
        compounded = 0;
    return(compounded);
}

void peggy_updatemargin(struct peggy_margin *funds,int64_t satoshis,uint16_t margin,int64_t amount)
{
    if ( margin == 0 )
        funds->deposits += satoshis;
    else
    {
        funds->margindeposits += amount;
        funds->marginvalue += (amount * margin);
    }
}

void peggy_thanks_you(struct peggy_info *PEGS,int64_t tip) { PEGS->bank.crypto777_royalty += tip; }

void peggy_changereserve(struct peggy_info *PEGS,struct peggy *PEG,int32_t dir,struct peggy_entry *entry,int64_t satoshis,uint16_t margin,int64_t marginamount)
{
    int64_t *dest; uint64_t replenish = 0,fee = 0;
    if ( Debuglevel > 2 )
        printf("CHANGE %.8f: %c%s: %lld costbasis %.8f %s fee %.8f royalty %.8f estimated %.8f unlocked %.8f, interestpaid %.8f | units.%lld oppo.%lld\n",dstr(satoshis),entry->polarity<0?'-':'+',PEG->name.name,(long long)entry->denomination,dstr(entry->costbasis),peggy_aprstr(entry->dailyrate),dstr(entry->fee),dstr(entry->royalty),dstr(entry->estimated_interest),dstr(entry->interest_unlocked),dstr(entry->interestpaid),(long long)entry->supply.num,(long long)entry->supply.numoppo);
    PEGS->bank.APRfund_reserved += (entry->estimated_interest - entry->interest_unlocked);
    if ( PEG->pool.funds.deposits > 0 && PEGS->bank.funds.deposits > 0 )
    {
        if ( PEGS->bank.APRfund < 0 )
            PEGS->bank.APRfund += entry->fee;
        else
        {
            fee = entry->fee / PEGS->feediv, fee *= PEGS->feemult;
            PEGS->bank.APRfund += ((entry->fee - fee) - entry->interestpaid);
        }
    } else replenish = entry->fee;
    peggy_thanks_you(PEGS,entry->royalty + fee);
    peggy_updatemargin(&PEG->pool.funds,satoshis + replenish,margin,marginamount);
    peggy_updatemargin(&PEGS->bank.funds,satoshis + replenish,margin,marginamount);
    peggy_updatemargin(&PEGS->basereserves[entry->baseid].funds,dir * satoshis,margin,dir * marginamount);
    peggy_updatemargin(&PEGS->basereserves[entry->relid].funds,-dir * satoshis,margin,-dir * marginamount);
    //printf("dir.%d polarity.%d baseid.%d relid.%d sats %.8f [%.8f %.8f]\n",dir,entry->polarity,entry->baseid,entry->relid,dstr(satoshis),dstr(PEGS->basereserves[entry->baseid]),dstr(PEGS->basereserves[entry->relid]));
    dest = (entry->polarity < 0) ? &PEG->pool.liability.numoppo : &PEG->pool.liability.num;
    *dest += (dir * entry->denomination);
    if ( Debuglevel > 2 || (rand() % 1000) == 0 )
        printf(">>>>>>> %c%s %lld: cost %11.8f %s royalties %.8f APR.(R%.8f B%.8f) satoshis %11.8f %s.(+%lld -%lld) base.(%.8f %.8f) total %s (%.8f %.8f %.8f) (%.8f %.8f %.8f)\n",entry->polarity<0?'-':'+',PEG->name.name,(long long)entry->denomination,dstr(entry->costbasis),peggy_aprstr(entry->dailyrate),dstr(PEGS->bank.crypto777_royalty),dstr(PEGS->bank.APRfund_reserved),dstr(PEGS->bank.APRfund),dstr(satoshis),PEG->name.name,(long long)PEG->pool.liability.num,(long long)PEG->pool.liability.numoppo,dstr(PEGS->basereserves[entry->baseid].funds.deposits),dstr(PEGS->basereserves[entry->relid].funds.deposits),PEG->name.name,dstr(PEG->pool.funds.deposits),dstr(PEG->pool.funds.margindeposits),dstr(PEG->pool.funds.marginvalue),dstr(PEGS->bank.funds.deposits),dstr(PEGS->bank.funds.margindeposits),dstr(PEGS->bank.funds.marginvalue));
}

uint64_t peggy_satoshis(int32_t polarity,int16_t denomination,int64_t price,int64_t oppoprice)
{
    uint64_t satoshi;
    if ( polarity < 0 )
    {
        //satoshi = (denomination * price) / PRICE_RESOLUTION;
        //satoshi = (satoshi * oppoprice) / PRICE_RESOLUTION;
        //return((SATOSHIDEN * satoshi) / PRICE_RESOLUTION);
        
        satoshi = (oppoprice * price) / PRICE_RESOLUTION_ROOT;
        satoshi = ((SATOSHIDEN / PRICE_RESOLUTION_ROOT) * (satoshi * denomination)) / PRICE_RESOLUTION;
        return(satoshi);
        //return(denomination * price * oppoprice * (10 / (PRICE_RESOLUTION * PRICE_RESOLUTION * PRICE_RESOLUTION)));
    }
    else
    {
        //satoshi = (denomination * price) / PRICE_RESOLUTION;
        //return((SATOSHIDEN * satoshi) / PRICE_RESOLUTION);
        return((SATOSHIDEN * denomination * price) / PRICE_RESOLUTION);
    }
}

uint64_t peggy_poolmainunits(struct peggy_entry *entry,int32_t dir,int32_t polarity,struct price_resolution price,struct price_resolution oppoprice,struct price_resolution spread,uint64_t poolincr,int16_t denomunits)
{
    uint64_t mainunits,satoshis; struct price_resolution fee;
    entry->denomination = denomunits;
    entry->costbasis = peggy_satoshis(polarity,denomunits,price.Pval,oppoprice.Pval);
    if ( (entry->baseid != 0 || entry->relid != 0) && spread.Pval != 0 && (fee.Pval= SCALED_PRICE(price.Pval,spread.Pval)) != 0 )
        price.Pval += dir * fee.Pval;
    else fee.Pval = 0;
    satoshis = peggy_satoshis(polarity,denomunits,price.Pval,oppoprice.Pval);
    if ( (satoshis % poolincr) == 0 )
        dir = 0;
    mainunits = dir + (satoshis / poolincr);
    entry->total = poolincr * mainunits;
    entry->fee = (entry->total - entry->costbasis);
    if ( Debuglevel > 2 )
        printf("mainunits %llu: origcost %.8f [%.8f] denomination %d poolincr %.8f price %.6f spread %.6f fee %.8f\n",(long long)mainunits,dstr(entry->costbasis),dstr(entry->fee),denomunits,dstr(poolincr),Pval(&price),Pval(&spread),Pval(&fee));
    return(entry->total);
}

int64_t peggy_pairabs(int64_t basebalance,int64_t relbalance)
{
    int64_t baserelbalance;
    if ( (baserelbalance= basebalance) < 0 )
        baserelbalance = -baserelbalance;
    if ( relbalance < 0 )
        baserelbalance -= relbalance;
    else baserelbalance += relbalance;
    return(baserelbalance);
}

int64_t peggy_lockrate(struct peggy_entry *entry,struct peggy_info *PEGS,struct peggy *PEG,uint64_t satoshis,uint16_t numdays)
{
    int64_t diff,compounded,prod,dailyrate=0,both,needed = 0;
    if ( (both= entry->supply.num + entry->supply.numoppo) != 0 )
    {
        if ( (diff= (entry->supply.numoppo - entry->supply.num)) < 0 )
            entry->supplydiff = -diff;
        else entry->supplydiff = diff;
        prod = (PEG->maxdailyrate * diff);
        if ( prod > 0 )
            prod *= PEGS->posboost;
        dailyrate = (prod /  both) + dailyrates[PEGS->interesttenths];
        if ( dailyrate > PEGGY_RATE_777 )
            dailyrate = PEGGY_RATE_777;
        else if ( dailyrate < -PEGGY_RATE_777 )
            dailyrate = -PEGGY_RATE_777;
        if ( prod < 0 && dailyrate > 0 )
            dailyrate /= PEGS->negpenalty;
        compounded = peggy_compound(0,satoshis,dailyrate,numdays);
        needed = (compounded - satoshis);
        needed *= 3, needed /= 2;
        if ( Debuglevel > 2 )
            printf("2x needed %.8f dailyrate.%lld compounded %lld <- %lld diff.%lld prod.%lld both.%lld -> %lld %s\n",dstr(needed),(long long)dailyrate,(long long)compounded,(long long)satoshis,(long long)diff,(long long)prod,(long long)both,(long long)dailyrate,peggy_aprstr(dailyrate));
        if ( (PEGS->bank.APRfund_reserved + needed) > PEGS->bank.APRfund )
        {
            compounded = peggy_compound(1,satoshis,dailyrate,numdays);
            printf("reserved %.8f + needed %.8f) > APRfund %.8f\n",dstr(PEGS->bank.APRfund_reserved),dstr(needed),dstr(PEGS->bank.APRfund));
            needed = dailyrate = 0;
        } else entry->estimated_interest = needed;
    }
    entry->dailyrate = dailyrate;
    return(dailyrate);
}

int32_t peggy_islegal_amount(struct peggy_entry *entry,struct peggy_info *PEGS,struct peggy *PEG,int32_t dir,int64_t satoshis,int32_t numdays,uint16_t margin,struct price_resolution price)
{
    uint64_t newsupply; int64_t baserelbalance,newbaserel;
    if ( margin == 0 )
        entry->dailyrate = peggy_lockrate(entry,PEGS,PEG,satoshis,numdays);
    if ( entry->baseid == entry->relid )
    {
        printf("illegal baseid.%d relid.%d (%s)\n",PEG->name.baseid,PEG->name.relid,PEG->name.name);
        getchar();
        return(1);
    }
    if ( PEG->name.id == 0 )
    {
        printf("BTCD is legal\n");
        return(1);
    }
    if ( (newsupply= ((PEG->pool.funds.deposits + PEG->pool.funds.marginvalue) + satoshis)) > PEG->limits.maxsupply )
    {
        printf("peggy_islegal_amount %.8f %.8f newsupply %.8f > limits %.8f\n",dstr(PEG->pool.funds.deposits),dstr(PEG->pool.funds.marginvalue),dstr(newsupply),dstr(PEG->limits.maxsupply));
        return(0);
    }
    //printf("baseid.%d relid.%d\n",entry->baseid,entry->relid);
    baserelbalance = peggy_pairabs(PEGS->basereserves[entry->baseid].funds.deposits,PEGS->basereserves[entry->relid].funds.deposits);
    newbaserel = peggy_pairabs(PEGS->basereserves[entry->baseid].funds.deposits + satoshis,PEGS->basereserves[entry->relid].funds.deposits - satoshis);
    if ( Debuglevel > 2 )
        printf("baseid.%d relid.%d satoshis %.8f baserelbalance %.8f newbaserel %.8f\n",entry->baseid,entry->relid,dstr(satoshis),dstr(baserelbalance),dstr(newbaserel));
    if ( newbaserel <= baserelbalance )
        return(1);
    //printf("entry->supplydiff.%lld diff %.8f vs maxnetbalance %.8f\n",(long long)entry->supplydiff,(double)(price.Pval*entry->supplydiff)/PRICE_RESOLUTION,dstr(PEG->limits.maxnetbalance));
    if ( PEG->limits.maxnetbalance != 0 && (price.Pval * entry->supplydiff) / PRICE_RESOLUTION > PEG->limits.maxnetbalance/SATOSHIDEN )
    {
        printf("entry->supplydiff.%lld diff %.8f vs maxnetbalance %.8f\n",(long long)entry->supplydiff,(double)(price.Pval*entry->supplydiff)/PRICE_RESOLUTION,dstr(PEG->limits.maxnetbalance));
        return(0);
    }
    return(1);
}

uint64_t peggy_assetbits(char *name) { return((is_decimalstr(name) != 0) ? calc_nxt64bits(name) : stringbits(name)); }

struct peggy *peggy_findpair(struct peggy_info *PEGS,char *name)
{
    int32_t i; uint64_t assetbits;
    if ( (assetbits= peggy_assetbits(name)) != 0 )
    {
        for (i=0; i<PEGS->numpegs; i++)
        {
            if ( PEGS->contracts[i]->name.assetbits == assetbits )
                return(PEGS->contracts[i]);
        }
    }
    return(0);
}

struct peggy *peggy_found(struct peggy_entry *entry,struct peggy *PEG,int32_t polarity)
{
    int64_t num,numoppo;
    memset(entry,0,sizeof(*entry));
    num = PEG->pool.liability.num, numoppo = PEG->pool.liability.numoppo;
    if ( polarity >= 0 )
        entry->polarity = 1, entry->baseid = PEG->name.baseid, entry->relid = PEG->name.relid, entry->supply.num = num, entry->supply.numoppo = numoppo;
    else entry->polarity = -1, entry->baseid = PEG->name.relid, entry->relid = PEG->name.baseid, entry->supply.num = numoppo, entry->supply.numoppo = num;
    //printf("(%s) -> baseid.%d relid.%d\n",PEG->name.name,entry->baseid,entry->relid);
    return(PEG);
}

struct peggy *peggy_find(struct peggy_entry *entry,struct peggy_info *PEGS,char *name,int32_t polarity)
{
    struct peggy *PEG;
    if ( (PEG= peggy_findpair(PEGS,name)) != 0 )
        return(peggy_found(entry,PEG,polarity));
    return(0);
}

struct peggy *peggy_findpeg(struct peggy_entry *entry,struct peggy_info *PEGS,int32_t peg)
{
    if ( peg >= 0 )
        return(peggy_found(entry,PEGS->contracts[peg],peg));
    else return(peggy_found(entry,PEGS->contracts[-peg],peg));
}

int32_t peggy_pegstr(char *buf,struct peggy_info *PEGS,char *name)
{
    int32_t peg=0; struct peggy *PEG;
    buf[0] = 0;
    if ( name != 0 )
    {
        strcpy(buf,name);
        if ( (PEG= peggy_findpair(PEGS,name)) != 0 )
            return(PEG->name.id);
    }
    return(peg);
}

int32_t peggy_setname(char *buf,char *name)
{
    return(peggy_pegstr(buf,opreturns_context("peggy",0),name));
}

int64_t peggy_calcspread(int64_t spread,int32_t lockdays) { return((lockdays < 30) ? spread : ((30 * spread) / lockdays)); }

uint64_t peggy_setunit(struct peggy_unit *U,struct peggy_entry *entry,void *_PEGS,struct peggy *PEG,uint64_t seed,uint32_t blocktimestamp,uint64_t nxt64bits,struct peggy_lock *lock)
{
    int32_t numdays,polarity; uint64_t poolincr; int16_t denom; struct price_resolution spread; struct peggy_info *PEGS = _PEGS;
    memset(U,0,sizeof(*U));
    U->lock = *lock;
    if ( (denom= lock->denom) < 0 )
        polarity = -1, denom = -denom;
    else polarity = 1;
    if ( denom >= PRICE_RESOLUTION_MAXUNITS )
    {
        printf("denom.%d is too big max %lld\n",denom,(long long)PRICE_RESOLUTION_MAXUNITS);
        return(0);
    }
    if ( U->lock.margin != 0 )
    {
        if ( U->lock.maxlockdays > PEGGY_MARGINLOCKDAYS )
        {
            printf("error: maxlockdays cant be more than PEGGY_MARGINLOCKDAYS %d for margin trading\n",PEGGY_MARGINLOCKDAYS);
            return(0);
        }
        else if ( U->lock.margin > PEG->lockparms.margin )
        {
            printf("error: margin.%d cant be more than %d for margin trading %s\n",U->lock.margin,PEG->lockparms.margin,PEG->name.name);
            return(0);
        }
        else if ( denom < U->lock.margin )
        {
            printf("error: denomination %lld must be >= margin %dx trading %s\n",(long long)denom,U->lock.margin,PEG->name.name);
            return(0);
        }
        else if ( (denom + entry->supply.num) > entry->supply.numoppo )
        {
            //printf("error: denom.%d supply %d %d > oppo %d -> balance violation %s\n",denom,entry->supply.num,denom+entry->supply.num,entry->supply.numoppo,PEG->name.name);
            return(0);
        }
        U->lock.redemptiongapdays = PEGGY_MARGINGAPDAYS;
    } else U->lock.redemptiongapdays = PEG->lockparms.redemptiongapdays;
    if ( U->lock.maxlockdays <= PEG->lockparms.maxlockdays )
        U->lock.maxlockdays = PEG->lockparms.maxlockdays;
    if ( U->lock.minlockdays >= PEG->lockparms.minlockdays )
        U->lock.minlockdays = PEG->lockparms.minlockdays;
    if ( U->lock.minlockdays > U->lock.maxlockdays )
        U->lock.minlockdays = U->lock.maxlockdays;
    if ( U->lock.clonesmear > PEG->lockparms.clonesmear )
        U->lock.clonesmear = PEG->lockparms.clonesmear;
    if ( U->lock.mixrange > PEG->lockparms.mixrange )
        U->lock.mixrange = PEG->lockparms.mixrange;
    U->lock.extralockdays = (seed % PEG->lockparms.extralockdays);
    entry->price = peggy_lastprice(PEGS,PEG,1,PEG->firsttimestamp,blocktimestamp);
    entry->oppoprice = peggy_lastprice(PEGS,PEG,-1,PEG->firsttimestamp,blocktimestamp);
    poolincr = PEG->pool.mainunitsize;
    U->timestamp = blocktimestamp, U->baseid = entry->baseid, U->relid = entry->relid; //, U->nxt64bits = nxt64bits,
    U->lock.peg = PEG->name.id * polarity;
    spread.Pval = peggy_calcspread(PEG->spread.Pval,U->lock.minlockdays + U->lock.extralockdays);
    peggy_poolmainunits(entry,1,polarity,entry->price,entry->oppoprice,spread,poolincr,denom);
    U->costbasis = entry->costbasis;
    numdays = (polarity > 0) ? (U->lock.maxlockdays + PEG->lockparms.extralockdays) : (U->lock.minlockdays + PEG->lockparms.extralockdays);
    return(entry->total * peggy_islegal_amount(entry,PEGS,PEG,polarity,entry->total,numdays,U->lock.margin,entry->price));
}

void peggy_margincalls(struct peggy_info *PEGS,struct peggy *PEG,struct price_resolution newprice,struct price_resolution shortprice,uint32_t blocktimestamp)
{
    int32_t i; struct peggy_unit *U; int64_t satoshis,gain,threshold; struct peggy_entry entry;
    for (i=0,U=&PEGS->accts->units[0]; i<PEGS->accts->numunits; i++,U++)
    {
        if ( U->lock.margin != 0 && (PEG->name.id == U->lock.peg || PEG->name.id == -U->lock.peg) )
        {
            if ( (PEG= peggy_findpeg(&entry,PEGS,U->lock.peg)) != 0 )
            {
                satoshis = peggy_poolmainunits(&entry,-1,entry.polarity,newprice,shortprice,PEG->spread,PEG->pool.mainunitsize,U->lock.denom);
                gain = entry.polarity * (satoshis - U->costbasis), threshold = -U->costbasis/U->lock.margin;
                if ( gain < threshold )
                {
                    printf("%s %8.6f %8.6f %2dx gain %11.8f polarity.%-2d (%11.8f - cost %11.8f) -> profit %11.8f margincall %11.8f ",PEG->name.name,Pval(&newprice),Pval(&shortprice),U->lock.margin,dstr(gain),entry.polarity,dstr(satoshis),dstr(U->costbasis),dstr(gain),dstr(threshold));
                    printf("MARGINCALLED\n");
                    peggy_changereserve(PEGS,PEG,-1,&entry,satoshis,U->lock.margin,U->marginamount);
                    peggy_delete(PEGS->accts,U,PEGGY_RSTATUS_MARGINCALL);
                }
            }
        }
    }
}

int64_t peggy_redeemhash(struct peggy_entry *entry,struct peggy_info *PEGS,struct peggy_unit *U,uint32_t blocktimestamp,int32_t lockdays)
{
    struct price_resolution price,oppoprice,spread; struct peggy *PEG;
    uint32_t timestamp; int32_t n,delta; uint64_t poolincr,interest,satoshis = 0;
    if ( (PEG= peggy_findpeg(entry,PEGS,U->lock.peg)) != 0 )
    {
        poolincr = PEG->pool.mainunitsize, oppoprice.Pval = spread.Pval = 0;
        delta = (blocktimestamp - U->timestamp) / PEGGY_DAYTICKS;
        lockdays += U->lock.extralockdays;
        if ( delta < lockdays )
            return(0);
        else if ( delta > (lockdays + U->lock.redemptiongapdays) )
            return(-1);
        timestamp = U->timestamp + (lockdays * PEGGY_DAYTICKS);
        price = peggy_lastprice(PEGS,PEG,1,PEG->firsttimestamp,timestamp);
        oppoprice = peggy_lastprice(PEGS,PEG,-1,PEG->firsttimestamp,timestamp);
        n = (int32_t)(satoshis / PEG->unitincr);
        price = peggy_aveprice(PEGS,PEG,timestamp,U->lock.extralockdays,1,n);
        if ( entry->polarity < 0 )
            oppoprice = peggy_aveprice(PEGS,PEG,timestamp,U->lock.extralockdays,-1,n);
        spread.Pval = peggy_calcspread(PEG->spread.Pval,lockdays);
        satoshis = peggy_poolmainunits(entry,-1,entry->polarity,price,oppoprice,spread,poolincr,U->lock.denom);
        entry->costbasis = U->costbasis;
        if ( U->estimated_interest != 0 )
        {
            interest = peggy_compound(0,satoshis,U->dailyrate,lockdays + U->lock.extralockdays) - satoshis;
            entry->interestpaid = (interest / poolincr) * poolincr;
            if ( entry->interestpaid > U->estimated_interest )
                entry->interestpaid = U->estimated_interest;
            entry->royalty += (interest - entry->interestpaid);
            entry->interest_unlocked = U->estimated_interest;
        }
        if ( U->lock.margin == 0 )
        {
            if ( PEG->pool.funds.deposits < satoshis )
                satoshis = PEG->pool.funds.deposits;
            else if (PEG->pool.funds.deposits >= (satoshis + entry->interestpaid) )
                satoshis += entry->interestpaid;
        }
        else
        {
            if ( PEG->pool.funds.margindeposits < satoshis )
                satoshis = PEG->pool.funds.margindeposits;
        }
    }
    return(satoshis);
}

uint64_t peggy_redeem(struct peggy_info *PEGS,int32_t readonly,char *name,int32_t polarity,uint64_t nxt64bits,bits256 pubkey,uint16_t lockdays,uint8_t chainlen,uint32_t blocktimestamp)
{
    struct peggy_entry entry; struct peggy_unit *U; struct peggy *PEG; int32_t peg; int64_t satoshis = 0; bits256 lockhash;
    if ( (PEG= peggy_findpair(PEGS,name)) != 0 )
    {
        peg = PEG->name.id * polarity;
        lockhash = acct777_lockhash(pubkey,lockdays,chainlen);
        if ( (U= peggy_match(PEGS->accts,peg,nxt64bits,lockhash,lockdays)) != 0 )
        {
            if ( (satoshis= peggy_redeemhash(&entry,PEGS,U,blocktimestamp,lockdays)) < 0 )
            {
                printf("Autopurge unit.%p\n",U);
                satoshis = 0;
            }
            if ( readonly == 0 )
            {
                peggy_changereserve(PEGS,PEG,-1,&entry,satoshis,U->lock.margin,U->marginamount);
                peggy_delete(PEGS->accts,U,satoshis == 0 ? PEGGY_RSTATUS_AUTOPURGED : PEGGY_RSTATUS_REDEEMED);
            }
        }
    }
	return(satoshis);
}

uint64_t peggy_createunit(struct peggy_info *PEGS,struct peggy_unit *readU,uint64_t seed,uint32_t blocktimestamp,char *name,uint64_t nxt64bits,bits256 lockhash,struct peggy_lock *lock,uint64_t amount,uint64_t marginamount)
{
    struct peggy_entry entry; struct peggy_unit U; int32_t peg,polarity; int16_t denomination; int64_t satoshis = 0; struct peggy *PEG = 0;
    denomination = lock->denom;
    if ( denomination < 0 )
        polarity = -1, denomination = -denomination;
    else polarity = 1;
    if ( denomination >= PRICE_RESOLUTION_MAXUNITS )
    {
        printf("denomination.%d is too big max %lld\n",denomination,(long long)PRICE_RESOLUTION_MAXUNITS);
        return(0);
    }
    if ( name == 0 )
    {
        if ( (peg= lock->peg) == 0 )
            PEG = PEGS->contracts[0];
        else if ( lock->peg < 0 )
            peg = -peg, polarity = -polarity;
        if ( peg >= PEGS->numpegs )
        {
            printf("illegal peg id.%d\n",lock->peg);
            return(0);
        }
        PEG = PEGS->contracts[peg];
        PEG = peggy_find(&entry,PEGS,PEG->name.name,polarity);
    } else PEG = peggy_find(&entry,PEGS,name,polarity);
    if ( PEG != 0 )
    {
        if ( (satoshis= peggy_setunit(&U,&entry,PEGS,PEG,seed,blocktimestamp,nxt64bits,lock)) != 0 )
        {
            U.estimated_interest = entry.estimated_interest, U.amount = amount, U.marginamount = marginamount;
            if ( readU == 0 )
            {
                if ( Debuglevel > 2 )
                    printf("%s.%d amount %.8f satoshis %.8f incr.%.8f price.%.8f\n",PEG->name.name,PEG->name.id,dstr(amount),dstr(satoshis),dstr(PEG->pool.mainunitsize),Pval(&entry.price));
                if ( (lock->margin == 0 && amount >= satoshis) || (marginamount * lock->margin) >= satoshis )
                {
                    peggy_changereserve(PEGS,PEG,1,&entry,entry.costbasis,lock->margin,marginamount);
                    peggy_addunit(PEGS->accts,&U,lockhash);
                    if ( Debuglevel > 2 )
                        printf("id.%d needed for interest reserved %.8f BTCD, royalty %.8f | dailyrate %lld %s\n",PEG->name.id,dstr(U.estimated_interest),dstr(amount) - dstr(satoshis),(long long)U.dailyrate,peggy_aprstr(U.dailyrate));
                } else printf("%s amount %.8f not enough for unit %.8f or margin.%d %llu %.8f\n",PEG->name.name,dstr(amount),dstr(satoshis),lock->margin,(long long)marginamount,dstr(marginamount * lock->margin)), satoshis = 0;
            }
        }
        if ( readU != 0 ) *readU = U, printf("%c%s cost %.8f %s\n",polarity < 0 ? '-' : '+',PEG->name.name,dstr(satoshis),peggy_aprstr(U.dailyrate));
    } else printf("peggy_createunit: cant find.(%s)\n",name);
    return(satoshis);
}

#include "peggytx.c"

#endif
#endif


