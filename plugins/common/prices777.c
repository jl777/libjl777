//
//  prices777.c
//  crypto777
//
//  Copyright (c) 2015 jl777. All rights reserved.
//

#define BUNDLED
#define PLUGINSTR "prices"
#define PLUGNAME(NAME) prices ## NAME
#define STRUCTNAME struct PLUGNAME(_info) 
#define STRINGIFY(NAME) #NAME
#define PLUGIN_EXTRASIZE sizeof(STRUCTNAME)

#define USD 0
#define EUR 1
#define JPY 2
#define GBP 3
#define AUD 4
#define CAD 5
#define CHF 6
#define NZD 7
#define CNY 8
#define RUB 9

#define NZDUSD 0
#define NZDCHF 1
#define NZDCAD 2
#define NZDJPY 3
#define GBPNZD 4
#define EURNZD 5
#define AUDNZD 6
#define CADJPY 7
#define CADCHF 8
#define USDCAD 9
#define EURCAD 10
#define GBPCAD 11
#define AUDCAD 12
#define USDCHF 13
#define CHFJPY 14
#define EURCHF 15
#define GBPCHF 16
#define AUDCHF 17
#define EURUSD 18
#define EURAUD 19
#define EURJPY 20
#define EURGBP 21
#define GBPUSD 22
#define GBPJPY 23
#define GBPAUD 24
#define USDJPY 25
#define AUDJPY 26
#define AUDUSD 27

#define USDNUM 28
#define EURNUM 29
#define JPYNUM 30
#define GBPNUM 31
#define AUDNUM 32
#define CADNUM 33
#define CHFNUM 34
#define NZDNUM 35

#define MAX_DEPTH 25
#define NUM_CONTRACTS 28
#define NUM_CURRENCIES 8
#define NUM_COMBINED (NUM_CONTRACTS + NUM_CURRENCIES)
#define MAX_SPLINES 64
#define MAX_LOOKAHEAD 48

#define DEFAULT_POLLGAP 1000
#define MINUTES_FIFO (1024)
#define HOURS_FIFO (64)
#define DAYS_FIFO (512)
#define MAX_EXCHANGES 64
#define MAX_CURRENCIES 32
#define PRICE_DECAY 0.9

#define INSTANTDEX_EXCHANGEID 0
#define INSTANTDEX_UNCONFID 1
#define INSTANTDEX_NXTAEID 2

#define DEFINES_ONLY
#include <math.h>
#include "../agents/plugin777.c"
#undef DEFINES_ONLY

#define calc_predisplinex(startweekind,clumpsize,weekind) (((weekind) - (startweekind))/(clumpsize))
#define _extrapolate_Spline(Splines,gap) ((double)(Splines)[0] + ((gap) * ((double)(Splines)[1] + ((gap) * ((double)(Splines)[2] + ((gap) * (double)(Splines)[3]))))))
#define _extrapolate_Slope(Splines,gap) ((double)(Splines)[1] + ((gap) * ((double)(Splines)[2] + ((gap) * (double)(Splines)[3]))))

#define PRICE_BLEND(oldval,newval,decay,oppodecay) ((oldval == 0.) ? newval : ((oldval * decay) + (oppodecay * newval)))
#define PRICE_BLEND64(oldval,newval,decay,oppodecay) ((oldval == 0) ? newval : ((oldval * decay) + (oppodecay * newval) + 0.499))

struct prices777_nxtquote { uint64_t assetid,nxt64bits,quoteid,qty,priceNQT; uint32_t timestamp; };
struct prices777_nxtbooks { struct prices777_nxtquote orderbook[MAX_DEPTH][2],prevorderbook[MAX_DEPTH][2],prev2orderbook[MAX_DEPTH][2]; };
struct prices777
{
    char url[512],exchange[64],base[64],rel[64],lbase[64],lrel[64],key[64],contract[64],origbase[64],origrel[64];
    uint64_t contractnum,ap_mult; int32_t keysize; double lastupdate,decay,oppodecay;
    uint32_t exchangeid,numquotes,updated,lasttimestamp,RTflag,fifoinds[6];
    double orderbook[MAX_DEPTH][2][2],prevorderbook[MAX_DEPTH][2][2],prev2orderbook[MAX_DEPTH][2][2],lastprice;
    struct prices777_nxtbooks *nxtbooks;
    float days[DAYS_FIFO],hours[HOURS_FIFO],minutes[MINUTES_FIFO];
};

struct exchange_info
{
    void (*updatefunc)(struct prices777 *prices,int32_t maxdepth);
    int32_t (*supports)(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid);
    uint64_t (*trade)(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume);
    char name[16],apikey[MAX_JSON_FIELD],apisecret[MAX_JSON_FIELD],userid[MAX_JSON_FIELD];
    uint32_t num,exchangeid,pollgap,refcount; uint64_t nxt64bits; double lastupdate;
} Exchanges[MAX_EXCHANGES];

struct prices777_data
{
    uint64_t tmillistamps[128]; double tbids[128],tasks[128],topens[128],thighs[128],tlows[128];
    double flhlogmatrix[8][8],flogmatrix[8][8],fbids[128],fasks[128],fhighs[128],flows[128];
    uint32_t itimestamps[128]; double ilogmatrix[8][8],ibids[128],iasks[128];
    char edate[128]; double ecbmatrix[32][32],dailyprices[MAX_CURRENCIES * MAX_CURRENCIES],metals[4];
    int32_t ecbdatenum,ecbyear,ecbmonth,ecbday; double RTmatrix[32][32],RTprices[128],RTmetals[4];
    double btcusd,btcdbtc,cryptos[8];
};

struct prices777_spline { char name[64]; int32_t splineid,lasti,basenum,num,firstx,dispincr,spline32[MAX_SPLINES][4]; uint32_t utc32[MAX_SPLINES]; int64_t spline64[MAX_SPLINES][4]; double dSplines[MAX_SPLINES][4],pricevals[MAX_SPLINES+MAX_LOOKAHEAD],lastutc,lastval,aveslopeabs; };
struct prices777_info
{
    struct prices777 *ptrs[1024],*truefx[128],*fxcm[128],*instaforex[128],*ecb[128];
    struct prices777_spline splines[128]; double cryptovols[2][8][2],btcusd,btcdbtc,cnyusd;
    int32_t num,numt,numf,numi,nume; char *jsonstr;
    char truefxuser[64],truefxpass[64]; uint64_t truefxidnum;
    struct prices777_data data,tmp; struct kv777 *kv;
    float ecbdaily[DAYS_FIFO][MAX_CURRENCIES][MAX_CURRENCIES];
} BUNDLE;

STRUCTNAME PRICES;

char *PLUGNAME(_methods)[] = { "quote", "list" }; // list of supported methods approved for local access
char *PLUGNAME(_pubmethods)[] = { "quote", "list" }; // list of supported methods approved for public (Internet) access
char *PLUGNAME(_authmethods)[] = { "quote", "list" }; // list of supported methods that require authentication

uint64_t Currencymasks[NUM_CURRENCIES+1];

char CONTRACTS[][16] = {  "NZDUSD", "NZDCHF", "NZDCAD", "NZDJPY", "GBPNZD", "EURNZD", "AUDNZD", "CADJPY", "CADCHF", "USDCAD", "EURCAD", "GBPCAD", "AUDCAD", "USDCHF", "CHFJPY", "EURCHF", "GBPCHF", "AUDCHF", "EURUSD", "EURAUD", "EURJPY", "EURGBP", "GBPUSD", "GBPJPY", "GBPAUD", "USDJPY", "AUDJPY", "AUDUSD", "USDCNY", "USDHKD", "USDMXN", "USDZAR", "USDTRY", "EURTRY", "TRYJPY", "USDSGD", "EURNOK", "USDNOK","USDSEK","USDDKK","EURSEK","EURDKK","NOKJPY","SEKJPY","USDPLN","EURPLN","USDILS", // no more currencies
    "XAUUSD", "XAGUSD", "XPTUSD", "XPDUSD", "Copper", "NGAS", "UKOil", "USOil", // commodities
    // cryptos
    "NAS100", "SPX500", "US30", "Bund", "EUSTX50", "UK100", "JPN225", "GER30", "SUI30", "AUS200", "HKG33", "FRA40", "ESP35", "ITA40", "USDOLLAR", // indices
    "SuperNET" // assets
};

char CURRENCIES[][8] = { "USD", "EUR", "JPY", "GBP", "AUD", "CAD", "CHF", "NZD", // major currencies
    "CNY", "RUB", "MXN", "BRL", "INR", "HKD", "TRY", "ZAR", "PLN", "NOK", "SEK", "DKK", "CZK", "HUF", "ILS", "KRW", "MYR", "PHP", "RON", "SGD", "THB", "BGN", "IDR", "HRK", // end of currencies
    "XAU", "XAG", "XPT", "XPD", // metals, gold must be first
    "BTCD", "BTC", "NXT", "LTC", "ETH", "DOGE", "BTS", "MAID", "XCP",  "XMR" // cryptos
};

int32_t MINDENOMS[] = { 1000, 1000, 100000, 1000, 1000, 1000, 1000, 1000, // major currencies
   10000, 100000, 10000, 1000, 100000, 10000, 1000, 10000, 1000, 10000, 10000, 10000, 10000, 100000, 1000, 1000000, 1000, 10000, 1000, 1000, 10000, 1000, 10000000, 10000, // end of currencies
    1, 100, 1, 1, // metals, gold must be first
    1, 10, 100000, 100, 100, 10000000, 10000, 1000, 1000,  1000, 100000, 100000, 1000000 // cryptos
};

int32_t prices777_mindenomination(int32_t base)
{
    return(MINDENOMS[base]);
}

short Contract_base[NUM_COMBINED+1] = { 7, 7, 7, 7, 3, 1, 4, 5, 5, 0, 1, 3, 4, 0, 6, 1, 3, 4, 1, 1, 1, 1, 3, 3, 3, 0, 4, 4, 0,1,2,3,4,5,6,7, 8 };// Contract_base };
short  Contract_rel[NUM_COMBINED+1] = { 0, 6, 5, 2, 7, 7, 7, 2, 6, 5, 5, 5, 5, 6, 2, 6, 6, 6, 0, 4, 2, 3, 0, 2, 4, 2, 2, 0, 0,1,2,3,4,5,6,7,8 };// Contract_rel

short Baserel_contractnum[NUM_CURRENCIES+1][NUM_CURRENCIES+1] =
{
	{ 28, 18, 25, 22, 27,  9, 13,  0, 36 },
	{ 18, 29, 20, 21, 19, 10, 15,  5, 37 },
	{ 25, 20, 30, 23, 26,  7, 14,  3, -1 },
	{ 22, 21, 23, 31, 24, 11, 16,  4, 38 },
	{ 27, 19, 26, 24, 32, 12, 17,  6, 39 },
	{  9, 10,  7, 11, 12, 33,  8,  2, -1 },
	{ 13, 15, 14, 16, 17,  8, 34,  1, 40 },
	{  0,  5,  3,  4,  6,  2,  1, 35, -1 },
	{ 36, 37, -1, 38, 39, -1, 40, -1, 74 },
};

short Baserel_contractdir[NUM_CURRENCIES+1][NUM_CURRENCIES+1] =
{
	{  1, -1,  1, -1, -1,  1,  1, -1, -1 },
	{  1,  1,  1,  1,  1,  1,  1,  1, -1 },
	{ -1, -1,  1, -1, -1, -1, -1, -1,  0 },
	{  1, -1,  1,  1,  1,  1,  1,  1, -1 },
	{  1, -1,  1, -1,  1,  1,  1,  1, -1 },
	{ -1, -1,  1, -1, -1,  1,  1, -1,  0 },
	{ -1, -1,  1, -1, -1, -1,  1, -1, -1 },
	{  1, -1,  1, -1, -1,  1,  1,  1,  0 },
	{ -1, -1,  0, -1, -1,  0, -1,  0,  1 },
};

short Currency_contracts[NUM_CURRENCIES+1][NUM_CURRENCIES] =
{
	{  0,  9, 13, 18, 22, 25, 27, 28, },
	{  5, 10, 15, 18, 19, 20, 21, 29, },
	{  3,  7, 14, 20, 23, 25, 26, 30, },
	{  4, 11, 16, 21, 22, 23, 24, 31, },
	{  6, 12, 17, 19, 24, 26, 27, 32, },
	{  2,  7,  8,  9, 10, 11, 12, 33, },
	{  1,  8, 13, 14, 15, 16, 17, 34, },
	{  0,  1,  2,  3,  4,  5,  6, 35, },
	{ 36, 37, -1, 38, 39, -1, 40, 41, },
};

short Currency_contractothers[NUM_CURRENCIES+1][NUM_CURRENCIES] =	// buggy!
{
	{ 7, 5, 6, 1, 3, 2, 4, 0, },
	{ 7, 5, 6, 0, 4, 2, 3, 1, },
	{ 7, 5, 6, 1, 3, 0, 4, 2, },
	{ 7, 5, 6, 1, 0, 2, 4, 3, },
	{ 7, 5, 6, 1, 3, 2, 0, 4, },
	{ 7, 2, 6, 0, 1, 3, 4, 5, },
	{ 7, 5, 0, 2, 1, 3, 4, 6, },
	{ 0, 6, 5, 2, 1, 3, 4, 7, },
	{ 0, 1,-1, 3, 4,-1, 5,-1, },
};

short Currency_contractdirs[NUM_CURRENCIES+1][NUM_CURRENCIES] =
{
	{ -1,  1,  1, -1, -1,  1, -1,  1 },
	{  1,  1,  1,  1,  1,  1,  1,  1 },
	{ -1, -1, -1, -1, -1, -1, -1,  1 },
	{  1,  1,  1, -1,  1,  1,  1,  1 },
	{  1,  1,  1, -1, -1,  1,  1,  1 },
	{ -1,  1,  1, -1, -1, -1, -1,  1 },
	{ -1, -1, -1,  1, -1, -1, -1,  1 },
	{  1,  1,  1,  1, -1, -1, -1,  1 },
	{  1,  1,  1,  1,  1,  1,  1,  1 },
};

struct prices777 *prices777_initpair(int32_t needfunc,void (*updatefunc)(struct prices777 *prices,int32_t maxdepth),char *exchange,char *base,char *rel,double decay);
uint64_t submit_triggered_nxtae(char **retjsonstrp,int32_t is_MS,char *bidask,uint64_t nxt64bits,char *NXTACCTSECRET,uint64_t assetid,uint64_t qty,uint64_t NXTprice,char *triggerhash,char *comment,uint64_t otherNXT,uint32_t triggerheight);
int32_t prices777_basenum(char *base);

#include "InstantDEX/exchange_trades.h"

#define dto64(x) ((int64_t)((x) * (double)SATOSHIDEN * SATOSHIDEN))
#define dto32(x) ((int32_t)((x) * (double)SATOSHIDEN))
#define i64tod(x) ((double)(x) / ((double)SATOSHIDEN * SATOSHIDEN))
#define i32tod(x) ((double)(x) / (double)SATOSHIDEN)
#define _extrapolate_spline64(spline64,gap) ((double)i64tod((spline64)[0]) + ((gap) * ((double)i64tod(.001*.001*(spline64)[1]) + ((gap) * ((double)i64tod(.001*.001*.001*.001*(spline64)[2]) + ((gap) * (double)i64tod(.001*.001*.001*.001*.001*.001*(spline64)[3])))))))
#define _extrapolate_spline32(spline32,gap) ((double)i32tod((spline32)[0]) + ((gap) * ((double)i32tod(.001*.001*(spline32)[1]) + ((gap) * ((double)i32tod(.001*.001*.001*.001*(spline32)[2]) + ((gap) * (double)i32tod(.001*.001*.001*.001*.001*.001*(spline32)[3])))))))

double prices777_splineval(struct prices777_spline *spline,uint32_t timestamp,int32_t lookahead)
{
    int32_t i,gap,ind = (spline->num - 1);
    if ( timestamp >= spline->utc32[ind] )
    {
        gap = (timestamp - spline->utc32[ind]);
        if ( gap < lookahead )
            return(_extrapolate_spline64(spline->spline64[ind],gap));
        else return(0.);
    }
    else if ( timestamp <= spline->utc32[0] )
    {
        gap = (spline->utc32[0] - timestamp);
        if ( gap < lookahead )
            return(_extrapolate_spline64(spline->spline64[0],gap));
        else return(0.);
    }
    for (i=0; i<spline->num-1; i++)
    {
        ind = (i + spline->lasti) % (spline->num - 1);
        if ( timestamp >= spline->utc32[ind] && timestamp < spline->utc32[ind+1] )
        {
            spline->lasti = ind;
            return(_extrapolate_spline64(spline->spline64[ind],timestamp - spline->utc32[ind]));
        }
    }
    return(0.);
}

double prices777_calcspline(struct prices777_spline *spline,double *outputs,double *slopes,int32_t dispwidth,uint32_t *utc32,double *splinevals,int32_t num)
{
    static double errsums[3]; static int errcount;
	double c[MAX_SPLINES],f[MAX_SPLINES],dd[MAX_SPLINES],dl[MAX_SPLINES],du[MAX_SPLINES],gaps[MAX_SPLINES];
	int32_t n,i,lasti,x,numsplines,nonz; double vx,vy,vw,vz,gap,sum,xval,yval,abssum,lastval,lastxval,yval64,yval32,yval3; uint32_t gap32;
	sum = lastxval = n = lasti = nonz = 0;
	for (i=0; i<MAX_SPLINES&&i<num; i++)
	{
		if ( (f[n]= splinevals[i]) != 0. && utc32[i] != 0 )
		{
			//printf("i%d.(%f %f) ",i,utc[i],splinevals[i]);
			if ( n > 0 )
			{
				if ( (gaps[n-1]= utc32[i] - lastxval) < 0 )
				{
					printf("illegal gap %f to t%d\n",lastxval,utc32[i]);
					return(0);
				}
			}
			spline->utc32[n] = lastxval = utc32[i];
            n++;
		}
	}
	if ( (numsplines= n) < 4 )
		return(0);
	for (i=0; i<n-3; i++)
		dl[i] = du[i] = gaps[i+1];
	for (i=0; i<n-2; i++)
	{
		dd[i] = 2.0 * (gaps[i] + gaps[i+1]);
		c[i]  = (3.0 / (double)gaps[i+1]) * (f[i+2] - f[i+1]) - (3.0 / (double)gaps[i]) * (f[i+1] - f[i]);
	}
	//for (i=0; i<n; i++) printf("%f ",f[i]);
	//printf("F2[%d]\n",n);
	dd[0] += (gaps[0] + (double)gaps[0]*gaps[0] / gaps[1]);
	du[0] -= ((double)gaps[0]*gaps[0] / gaps[1]);
	dd[n-3] += (gaps[n-2] + (double)gaps[n-2]*gaps[n-2] / gaps[n-3]);
	dl[n-4] -= ((double)gaps[n-2]*gaps[n-2] / gaps[n-3]);
	
	//tridiagonal(n-2, dl, dd, du, c);
	for (i=0; i<n-1-2; i++)
	{
		du[i] /= dd[i];
		dd[i+1] -= dl[i]*du[i];
	}
	c[0] /= dd[0];
	for (i=1; i<n-2; i++)
		c[i] = (c[i] - dl[i-1] * c[i-1]) / dd[i];
	for (i=n-2-4; i>=0; i--)
		c[i] -= c[i+1] * du[i];
	//tridiagonal(n-2, dl, dd, du, c);
	
	for (i=n-3; i>=0; i--)
		c[i+1] = c[i];
	c[0] = (1.0 + (double)gaps[0] / gaps[1]) * c[1] - ((double)gaps[0] / gaps[1] * c[2]);
	c[n-1] = (1.0 + (double)gaps[n-2] / gaps[n-3] ) * c[n-2] - ((double)gaps[n-2] / gaps[n-3] * c[n-3]);
    //printf("c[n-1] %f, n-2 %f, n-3 %f\n",c[n-1],c[n-2],c[n-3]);
	abssum = nonz = lastval = 0;
    outputs[spline->firstx] = f[0];
    spline->num = numsplines;
    for (i=0; i<n; i++)
	{
        vx = f[i];
        vz = c[i];
        if ( i < n-1 )
        {
     		gap = gaps[i];
            vy = ((f[i+1] - f[i]) / gap) - (gap * (c[i+1] + 2.*c[i]) / 3.);
            vw = (c[i+1] - c[i]) / (3. * gap);
        }
        else
        {
            vy = 0;
            vw = 0;
        }
		//printf("%3d: t%u [%14.11f %14.11f %14.11f %14.11f] gap %f | %d\n",i,spline->utc32[i],(vx),vy*1000*1000,vz*1000*1000*1000*1000,vw*1000*1000*1000*1000*1000*1000,gap,conv_unixtime(&tmp,spline->utc32[i]));
		spline->dSplines[i][0] = vx, spline->dSplines[i][1] = vy, spline->dSplines[i][2] = vz, spline->dSplines[i][3] = vw;
		spline->spline64[i][0] = dto64(vx), spline->spline64[i][1] = dto64(vy*1000*1000), spline->spline64[i][2] = dto64(vz*1000*1000*1000*1000), spline->spline64[i][3] = dto64(vw*1000*1000*1000*1000*1000*1000);
		spline->spline32[i][0] = dto32(vx), spline->spline32[i][1] = dto32(vy*1000*1000), spline->spline32[i][2] = dto32(vz*1000*1000*1000*1000), spline->spline32[i][3] = dto32(vw*1000*1000*1000*1000*1000*1000);
		gap32 = gap = spline->dispincr;
		xval = spline->utc32[i] + gap;
		lastval = vx;
		while ( i < n-1 )
		{
			x = spline->firstx + ((xval - spline->utc32[0]) / spline->dispincr);
			if ( x > dispwidth-1 ) x = dispwidth-1;
			if ( x < 0 ) x = 0;
			if ( (i < n-2 && gap > gaps[i] + spline->dispincr) )
				break;
            if ( i == n-2 && xval > spline->utc32[n-1] + MAX_LOOKAHEAD*spline->dispincr )
            {
                //printf("x.%d dispwidth.%d xval %f > utc[n-1] %f + %f\n",x,dispwidth,xval,utc[n-1],MAX_LOOKAHEAD*incr);
                break;
            }
            if ( x >= 0 )
			{
				yval = _extrapolate_Spline(spline->dSplines[i],gap);
				yval64 = _extrapolate_spline64(spline->spline64[i],gap32);
                if ( (yval3 = prices777_splineval(spline,gap32 + spline->utc32[i],MAX_LOOKAHEAD*spline->dispincr)) != 0 )
                {
                    yval32 = _extrapolate_spline32(spline->spline32[i],gap32);
                    errsums[0] += fabs(yval - yval64), errsums[1] += fabs(yval - yval32), errsums[2] += fabs(yval - yval3), errcount++;
                    if ( fabs(yval - yval3) > SMALLVAL )
                        printf("(%.10f vs %.10f %.10f %.10f [%.16f %.16f %.16f]) ",yval,yval64,yval32,yval3, errsums[0]/errcount,errsums[1]/errcount,errsums[2]/errcount);
                }
				if ( yval > 5000. ) yval = 5000.;
				else if ( yval < -5000. ) yval = -5000.;
				if ( isnan(yval) == 0 )
				{
					outputs[x] = yval;
                    spline->lastval = outputs[x], spline->lastutc = xval;
                    if ( 1 && fabs(lastval) > SMALLVAL )
					{
						if ( lastval != 0 && outputs[x] != 0 )
						{
                            if ( slopes != 0 )
                                slopes[x] = (outputs[x] - lastval), abssum += fabs(slopes[x]);
							nonz++;
						}
					}
				}
				//else outputs[x] = 0.;
				//printf("x.%-4d %d %f %f %f i%-4d: gap %9.6f %9.6f last %9.6f slope %9.6f | %9.1f [%9.1f %9.6f %9.6f %9.6f %9.6f]\n",x,firstx,xval,utc[0],incr,i,gap,yval,lastval,slopes[x],xval,utc[i+1],dSplines[i][0],dSplines[i][1]*1000*1000,dSplines[i][2]*1000*1000*1000*1000,dSplines[i][3]*1000*1000*1000*1000*1000*1000);
			}
			gap32 += spline->dispincr, gap += spline->dispincr, xval += spline->dispincr;
		}
		//double pred = (i>0) ? _extrapolate_Spline(dSplines[i-1],gaps[i-1]) : 0.;
		//printf("%2d: w%8.1f [gap %f -> %9.6f | %9.6f %9.6f %9.6f %9.6f %9.6f]\n",i,weekinds[i],gap,pred,f[i],dSplines[i].x,1000000*dSplines[i].y,1000000*1000000*dSplines[i].z,1000000*1000000*1000*dSplines[i].w);
	}
	if ( nonz != 0 )
		abssum /= nonz;
	spline->aveslopeabs = abssum;
	return(lastval);
}

int32_t prices777_genspline(struct prices777_spline *spline,int32_t splineid,char *name,uint32_t *utc32,double *splinevals,int32_t maxsplines,double *refvals)
{
    int32_t i; double output[2048],slopes[2048],origvals[MAX_SPLINES];
    memset(spline,0,sizeof(*spline)), memset(output,0,sizeof(output)), memset(slopes,0,sizeof(slopes));
    spline->dispincr = 3600, spline->basenum = splineid, strcpy(spline->name,name);
    memcpy(origvals,splinevals,sizeof(*splinevals) * MAX_SPLINES);
    spline->lastval = prices777_calcspline(spline,output,slopes,sizeof(output)/sizeof(*output),utc32,splinevals,maxsplines);
    for (i=0; i<spline->num+3; i++)
    {
        if ( i < spline->num )
        {
            if ( refvals[i] != 0 && output[i * 24] != refvals[i] )
                printf("{%.8f != %.8f}.%d ",output[i * 24],refvals[i],i);
        }
        else printf("{%.8f %.3f} ",output[i * 24],slopes[i * 24]/spline->aveslopeabs);
        spline->pricevals[i] = output[i * 24];
    }
    printf("spline.%s num.%d\n",name,spline->num);
    return(spline->num);
}

int32_t prices777_ispair(char *base,char *rel,char *contract)
{
    int32_t i,j;
    base[0] = rel[0] = 0;
    for (i=0; i<sizeof(CURRENCIES)/sizeof(*CURRENCIES); i++)
    {
        if ( strncmp(CURRENCIES[i],contract,strlen(CURRENCIES[i])) == 0 )
        {
            for (j=0; j<sizeof(CURRENCIES)/sizeof(*CURRENCIES); j++)
                if ( strcmp(CURRENCIES[j],contract+strlen(CURRENCIES[i])) == 0 )
                {
                    strcpy(base,CURRENCIES[i]);
                    strcpy(rel,CURRENCIES[j]);
                    /*USDCNY 6.209700 -> 0.655564
                    USDCNY 6.204146 -> 0.652686
                    USDHKD 7.753400 -> 0.749321
                    USDHKD 7.746396 -> 0.746445
                    USDZAR 12.694000 -> 1.101688
                    USDZAR 12.682408 -> 1.098811
                    USDTRY 2.779700 -> 0.341327
                    EURTRY 3.048500 -> 0.386351
                    TRYJPY 44.724000 -> 0.690171
                    TRYJPY 44.679966 -> 0.687290
                    USDSGD 1.375200 -> 0.239415*/
                    //if ( strcmp(contract,"USDCNY") == 0 || strcmp(contract,"TRYJPY") == 0 || strcmp(contract,"USDZAR") == 0 )
                    //    printf("i.%d j.%d base.%s rel.%s\n",i,j,base,rel);
                    return((i<<8) | j);
                }
            break;
        }
    }
    return(-1);
}

int32_t prices777_basenum(char *base)
{
    int32_t i,j;
    if ( 1 )
    {
        for (i=0; i<sizeof(CURRENCIES)/sizeof(*CURRENCIES); i++)
            for (j=0; j<sizeof(CURRENCIES)/sizeof(*CURRENCIES); j++)
                if ( i != j && strcmp(CURRENCIES[i],CURRENCIES[j]) == 0 )
                    printf("duplicate.(%s)\n",CURRENCIES[i]), getchar();
    }
    for (i=0; i<sizeof(CURRENCIES)/sizeof(*CURRENCIES); i++)
        if ( strcmp(CURRENCIES[i],base) == 0 )
            return(i);
    return(-1);
}

int32_t prices777_contractnum(char *base,char *rel)
{
    int32_t i,j,c; char contractstr[16],*contract = 0;
    if ( 0 )
    {
        for (i=0; i<sizeof(CONTRACTS)/sizeof(*CONTRACTS); i++)
            for (j=0; j<sizeof(CONTRACTS)/sizeof(*CONTRACTS); j++)
                if ( i != j && strcmp(CONTRACTS[i],CONTRACTS[j]) == 0 )
                    printf("duplicate.(%s)\n",CONTRACTS[i]), getchar();
    }
    if ( base != 0 && base[0] != 0 && rel != 0 && rel[0] != 0 )
    {
        for (i=0; i<NUM_CURRENCIES; i++)
            if ( strcmp(base,CURRENCIES[i]) == 0 )
            {
                for (j=0; j<NUM_CURRENCIES; j++)
                    if ( strcmp(rel,CURRENCIES[j]) == 0 )
                        return(Baserel_contractnum[i][j]);
                break;
            }
        sprintf(contractstr,"%s%s",base,rel);
        contract = contractstr;
    } else contract = base;
    if ( contract != 0 && contract[0] != 0 )
    {
        for (c=0; c<sizeof(CONTRACTS)/sizeof(*CONTRACTS); c++)
            if ( strcmp(CONTRACTS[c],contract) == 0 )
                return(c);
    }
    return(-1);
}

int32_t PLUGNAME(_shutdown)(struct plugin_info *plugin,int32_t retcode)
{
    if ( retcode == 0 )  // this means parent process died, otherwise _process_json returned negative value
    {
    }
    return(retcode);
}

uint64_t PLUGNAME(_register)(struct plugin_info *plugin,STRUCTNAME *data,cJSON *argjson)
{
    uint64_t disableflags = 0;
    printf("init %s size.%ld\n",plugin->name,sizeof(struct prices_info));
    plugin->allowremote = 1;
    // runtime specific state can be created and put into *data
    return(disableflags); // set bits corresponding to array position in _methods[]
}

double _prices777_price_volume(double *volumep,uint64_t baseamount,uint64_t relamount)
{
    *volumep = (((double)baseamount + 0.000000009999999) / SATOSHIDEN);
    if ( baseamount > 0. )
        return((double)relamount / (double)baseamount);
    else return(0.);
}

void prices777_best_amounts(uint64_t *baseamountp,uint64_t *relamountp,double price,double volume)
{
    double checkprice,checkvol,distA,distB,metric,bestmetric = (1. / SMALLVAL);
    uint64_t baseamount,relamount,bestbaseamount = 0,bestrelamount = 0;
    int32_t i,j;
    baseamount = volume * SATOSHIDEN;
    relamount = ((price * volume) * SATOSHIDEN);
    //*baseamountp = baseamount, *relamountp = relamount;
    //return;
    for (i=-1; i<=1; i++)
        for (j=-1; j<=1; j++)
        {
            checkprice = _prices777_price_volume(&checkvol,baseamount+i,relamount+j);
            distA = (checkprice - price);
            distA *= distA;
            distB = (checkvol - volume);
            distB *= distB;
            metric = sqrt(distA + distB);
            if ( metric < bestmetric )
            {
                bestmetric = metric;
                bestbaseamount = baseamount + i;
                bestrelamount = relamount + j;
                //printf("i.%d j.%d metric. %f\n",i,j,metric);
            }
        }
    *baseamountp = bestbaseamount;
    *relamountp = bestrelamount;
}

int32_t prices777_addquote(struct prices777 *prices,uint32_t timestamp,int32_t bidask,int32_t ind,double price,double volume,struct prices777_nxtquote *nxtQ)
{
    uint32_t fifoind,i,k;
    if ( price == 0. )
        return(-1);
    fifoind = timestamp / 60;
    if ( prices->RTflag != 0 )
    {
        if ( (i= prices->lasttimestamp/60) != fifoind )
        {
            memcpy(prices->prev2orderbook,prices->prevorderbook,sizeof(prices->prev2orderbook));
            memcpy(prices->prevorderbook,prices->orderbook,sizeof(prices->orderbook));
            memset(prices->orderbook,0,sizeof(prices->orderbook));
            if ( prices->nxtbooks != 0 )
            {
                memcpy(prices->nxtbooks->prev2orderbook,prices->nxtbooks->prevorderbook,sizeof(prices->nxtbooks->prev2orderbook));
                memcpy(prices->nxtbooks->prevorderbook,prices->nxtbooks->orderbook,sizeof(prices->nxtbooks->orderbook));
                memset(prices->nxtbooks->orderbook,0,sizeof(prices->nxtbooks->orderbook));
            }
            while ( ++i <= fifoind )
            {
                k = (i % MINUTES_FIFO);
                prices->minutes[k] = 0;
            }
        }
        prices->lasttimestamp = timestamp;
        prices->fifoinds[0] = fifoind;
    }
    prices->orderbook[ind][bidask][0] = price, prices->orderbook[ind][bidask][1] = volume;
    if ( prices->nxtbooks != 0 )
        prices->nxtbooks->orderbook[ind][bidask] = *nxtQ;
    fifoind %= MINUTES_FIFO;
    if ( ind == 0 )
        prices->minutes[fifoind] = PRICE_BLEND(prices->minutes[fifoind],price,prices->decay,prices->oppodecay);
    return(fifoind);
}

int32_t prices777_json_quotes(int32_t dir,struct prices777 *prices,cJSON *array,int32_t maxdepth,char *pricefield,char *volfield,uint32_t reftimestamp)
{
    cJSON *item; int32_t i,j,n,numitems; uint32_t timestamp; double price,volume; struct prices777_nxtquote nxtQ;
    if ( reftimestamp == 0 )
        reftimestamp = (uint32_t)time(NULL);
    n = cJSON_GetArraySize(array);
    if ( maxdepth != 0 && n > maxdepth )
        n = maxdepth;
    for (i=0; i<n; i++)
    {
        if ( strcmp(prices->exchange,"bter") == 0 && dir < 0 )
            j = (n - 1) - i;
        else j = i;
        timestamp = 0;
        item = jitem(array,j);
        if ( pricefield != 0 && volfield != 0 )
        {
            price = get_API_float(jobj(item,pricefield));
            volume = get_API_float(jobj(item,volfield));
        }
        else if ( is_cJSON_Array(item) != 0 && (numitems= cJSON_GetArraySize(item)) != 0 ) // big assumptions about order within nested array!
        {
            price = get_API_float(jitem(item,0));
            volume = get_API_float(jitem(item,1));
        }
        else
        {
            printf("unexpected case in parseram_json_quotes\n");
            continue;
        }
        memset(&nxtQ,0,sizeof(nxtQ));
        if ( strcmp(prices->exchange,"nxtae") == 0 )
        {
            nxtQ.timestamp = timestamp = juinti(item,2);
            nxtQ.assetid = prices->contractnum;
            nxtQ.quoteid = j64bitsi(item,3);
            nxtQ.nxt64bits = j64bitsi(item,4);
            nxtQ.qty = j64bitsi(item,5);
            nxtQ.priceNQT = j64bitsi(item,6);
        }
        if ( timestamp == 0 )
            timestamp = reftimestamp;
        if ( Debuglevel > 2 || i < 1 )//|| strcmp("bter",prices->exchange) == 0 )
            printf("%d,%d: %-8s %s %5s/%-5s %13.8f vol %13.8f | invert %13.8f vol %13.8f | timestamp.%u\n",i,j,prices->exchange,dir>0?"bid":"ask",prices->base,prices->rel,price,volume,1./price,volume*price,timestamp);
        prices777_addquote(prices,timestamp,dir<0?1:0,i,price,volume,&nxtQ);
    }
    return(n);
}

cJSON *inner_json(double price,double vol,uint32_t timestamp,uint64_t quoteid,uint64_t nxt64bits,uint64_t qty,uint64_t pqt)
{
    cJSON *inner = cJSON_CreateArray();
    char numstr[64];
    sprintf(numstr,"%.8f",price), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%.8f",vol), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    cJSON_AddItemToArray(inner,cJSON_CreateNumber(timestamp));
    sprintf(numstr,"%llu",(long long)quoteid), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)nxt64bits), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)qty), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    sprintf(numstr,"%llu",(long long)pqt), cJSON_AddItemToArray(inner,cJSON_CreateString(numstr));
    return(inner);
}

void prices777_json_orderbook(char *exchangestr,struct prices777 *prices,int32_t maxdepth,cJSON *json,char *resultfield,char *bidfield,char *askfield,char *pricefield,char *volfield)
{
    cJSON *obj = 0,*bidobj,*askobj;
    if ( resultfield == 0 )
        obj = json;
    if ( maxdepth == 0 )
        maxdepth = MAX_DEPTH;
    if ( resultfield == 0 || (obj= cJSON_GetObjectItem(json,resultfield)) != 0 )
    {
        if ( (bidobj= cJSON_GetObjectItem(obj,bidfield)) != 0 && is_cJSON_Array(bidobj) != 0 )
            prices777_json_quotes(1,prices,bidobj,maxdepth,pricefield,volfield,0);
        if ( (askobj= cJSON_GetObjectItem(obj,askfield)) != 0 && is_cJSON_Array(askobj) != 0 )
            prices777_json_quotes(-1,prices,askobj,maxdepth,pricefield,volfield,0);
    }
}

void prices777_NXT(struct prices777 *prices,int32_t maxdepth)
{
    uint32_t timestamp; int32_t flip,i,n; uint64_t baseamount,relamount,qty,pqt; char url[1024],*str,*cmd,*field;
    cJSON *json,*bids,*asks,*srcobj,*item,*array; double price,vol;
    if ( NXT_ASSETID != stringbits("NXT") || strcmp(prices->rel,"NXT") != 0 )
        printf("NXT_ASSETID.%llu != %llu stringbits rel.%s\n",(long long)NXT_ASSETID,(long long)stringbits("NXT"),prices->rel);
    bids = cJSON_CreateArray(), asks = cJSON_CreateArray();
    for (flip=0; flip<2; flip++)
    {
        if ( flip == 0 )
            cmd = "getBidOrders", field = "bidOrders", array = bids;
        else cmd = "getAskOrders", field = "askOrders", array = asks;
        sprintf(url,"requestType=%s&asset=%llu&limit=%d",cmd,(long long)prices->contractnum,maxdepth);
        if ( (str= issue_NXTPOST(url)) != 0 )
        {
            if ( (json= cJSON_Parse(str)) != 0 )
            {
                if ( (srcobj= jarray(&n,json,field)) != 0 )
                {
                    for (i=0; i<n && i<maxdepth; i++)
                    {
                        /*
                            "quantityQNT": "79",
                            "priceNQT": "13499000000",
                            "transactionHeight": 480173,
                            "accountRS": "NXT-FJQN-8QL2-BMY3-64VLK",
                            "transactionIndex": 1,
                            "asset": "15344649963748848799",
                            "type": "ask",
                            "account": "5245394173527769812",
                            "order": "17926122097022414596",
                            "height": 480173
                        */
                        item = cJSON_GetArrayItem(srcobj,i);
                        qty = j64bits(item,"quantityQNT"), pqt = j64bits(item,"priceNQT");
                        baseamount = (qty * prices->ap_mult), relamount = (qty * pqt);
                        price = _prices777_price_volume(&vol,baseamount,relamount);
                        timestamp = get_blockutime(juint(item,"height"));
                        item = inner_json(price,vol,timestamp,j64bits(item,"order"),j64bits(item,"account"),qty,pqt);
                        cJSON_AddItemToArray(array,item);
                    }
                }
            }
            free(str);
        } else printf("cant get.(%s)\n",url);
    }
    json = cJSON_CreateObject();
    cJSON_AddItemToObject(json,"bids",bids);
    cJSON_AddItemToObject(json,"asks",asks);
    prices777_json_orderbook("nxtae",prices,maxdepth,json,0,"bids","asks",0,0);
}

void prices777_bittrex(struct prices777 *prices,int32_t maxdepth) // "BTC-BTCD"
{
    cJSON *json,*obj;
    char *jsonstr,market[128];
    if ( prices->url[0] == 0 )
    {
        sprintf(market,"%s-%s",prices->rel,prices->base);
        sprintf(prices->url,"https://bittrex.com/api/v1.1/public/getorderbook?market=%s&type=both&depth=%d",market,maxdepth);
    }
    jsonstr = issue_curl(prices->url);
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= cJSON_GetObjectItem(json,"success")) != 0 && is_cJSON_True(obj) != 0 )
                prices777_json_orderbook("bittrex",prices,maxdepth,json,"result","buy","sell","Rate","Quantity");
            free_json(json);
        }
        free(jsonstr);
    }
}

void prices777_bter(struct prices777 *prices,int32_t maxdepth)
{
    cJSON *json,*obj;
    char resultstr[MAX_JSON_FIELD],*jsonstr;
    if ( prices->url[0] == 0 )
        sprintf(prices->url,"http://data.bter.com/api/1/depth/%s_%s",prices->base,prices->rel);
    jsonstr = issue_curl(prices->url);
    //printf("(%s) -> (%s)\n",ep->url,jsonstr);
    //{"result":"true","asks":[["0.00008035",100],["0.00008030",2030],["0.00008024",100],["0.00008018",643.41783554],["0.00008012",100]
    if ( jsonstr != 0 )
    {
        //printf("BTER.(%s)\n",jsonstr);
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= cJSON_GetObjectItem(json,"result")) != 0 )
            {
                copy_cJSON(resultstr,obj);
                if ( strcmp(resultstr,"true") == 0 )
                {
                    maxdepth = 1000; // since bter ask is wrong order, need to scan entire list
                    prices777_json_orderbook("bter",prices,maxdepth,json,0,"bids","asks",0,0);
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
}

void prices777_standard(char *exchangestr,char *url,struct prices777 *prices,char *price,char *volume,int32_t maxdepth)
{
    char *jsonstr; cJSON *json;
    if ( (jsonstr= issue_curl(url)) != 0 )
    {
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            prices777_json_orderbook(exchangestr,prices,maxdepth,json,0,"bids","asks",price,volume);
            free_json(json);
        }
        free(jsonstr);
    }
}

void prices777_poloniex(struct prices777 *prices,int32_t maxdepth)
{
    char market[128];
    if ( prices->url[0] == 0 )
    {
        sprintf(market,"%s_%s",prices->rel,prices->base);
        sprintf(prices->url,"https://poloniex.com/public?command=returnOrderBook&currencyPair=%s&depth=%d",market,maxdepth);
    }
    prices777_standard("poloniex",prices->url,prices,0,0,maxdepth);
}

void prices777_bitfinex(struct prices777 *prices,int32_t maxdepth)
{
    if ( prices->url[0] == 0 )
        sprintf(prices->url,"https://api.bitfinex.com/v1/book/%s%s",prices->base,prices->rel);
    prices777_standard("bitfinex",prices->url,prices,"price","amount",maxdepth);
}

void prices777_btce(struct prices777 *prices,int32_t maxdepth)
{
    if ( prices->url[0] == 0 )
        sprintf(prices->url,"https://btc-e.com/api/2/%s%s/depth",prices->lbase,prices->lrel);
    prices777_standard("btce",prices->url,prices,0,0,maxdepth);
}

void prices777_bitstamp(struct prices777 *prices,int32_t maxdepth)
{
    if ( prices->url[0] == 0 )
        sprintf(prices->url,"https://www.bitstamp.net/api/order_book/");
    prices777_standard("bitstamp",prices->url,prices,0,0,maxdepth);
}

void prices777_okcoin(struct prices777 *prices,int32_t maxdepth)
{
    if ( prices->url[0] == 0 )
        sprintf(prices->url,"https://www.okcoin.com/api/depth.do?symbol=%s%s",prices->lbase,prices->lrel);
    prices777_standard("okcoin",prices->url,prices,0,0,maxdepth);
}

void prices777_huobi(struct prices777 *prices,int32_t maxdepth)
{
    if ( prices->url[0] == 0 )
        sprintf(prices->url,"http://api.huobi.com/staticmarket/depth_%s_json.js ",prices->lbase);
    prices777_standard("huobi",prices->url,prices,0,0,maxdepth);
}

void prices777_bityes(struct prices777 *prices,int32_t maxdepth)
{
    if ( prices->url[0] == 0 )
        sprintf(prices->url,"https://market.bityes.com/%s_%s/trade_history.js?time=%ld",prices->lrel,prices->lbase,time(NULL));
    prices777_standard("bityes",prices->url,prices,0,0,maxdepth);
}

void prices777_coinbase(struct prices777 *prices,int32_t maxdepth)
{
    if ( prices->url[0] == 0 )
        sprintf(prices->url,"https://api.exchange.coinbase.com/products/%s-%s/book?level=2",prices->base,prices->rel);
    prices777_standard("coinbase",prices->url,prices,0,0,maxdepth);
}

void prices777_lakebtc(struct prices777 *prices,int32_t maxdepth)
{
    if ( prices->url[0] == 0 )
    {
        if ( strcmp(prices->rel,"USD") == 0 )
            sprintf(prices->url,"https://www.LakeBTC.com/api_v1/bcorderbook");
        else if ( strcmp(prices->rel,"CNY") == 0 )
            sprintf(prices->url,"https://www.LakeBTC.com/api_v1/bcorderbook_cny");
        else printf("illegal lakebtc pair.(%s/%s)\n",prices->base,prices->rel);
    }
    prices777_standard("lakebtc",prices->url,prices,0,0,maxdepth);
}

void prices777_exmo(struct prices777 *prices,int32_t maxdepth)
{
    if ( prices->url[0] == 0 )
        sprintf(prices->url,"https://api.exmo.com/api_v2/orders_book?pair=%s_%s",prices->base,prices->rel);
    prices777_standard("exmo",prices->url,prices,0,0,maxdepth);
}

void prices777_btc38(struct prices777 *prices,int32_t maxdepth)
{
    if ( prices->url[0] == 0 )
        sprintf(prices->url,"http://api.btc38.com/v1/depth.php?c=%s&mk_type=%s",prices->lbase,prices->lrel);
    printf("btc38.(%s)\n",prices->url);
    prices777_standard("btc38",prices->url,prices,0,0,maxdepth);
}

/*void prices777_kraken(struct prices777 *prices,int32_t maxdepth)
 {
 if ( prices->url[0] == 0 )
 sprintf(prices->url,"https://api.kraken.com/0/public/Depth"); // need POST
 prices777_standard("kraken",prices->url,prices,0,0,maxdepth);
 }*/

/*void prices777_itbit(struct prices777 *prices,int32_t maxdepth)
 {
 if ( prices->url[0] == 0 )
 sprintf(prices->url,"https://www.itbit.com/%s%s",prices->base,prices->rel);
 prices777_standard("itbit",prices->url,prices,0,0,maxdepth);
 }*/

void init_Currencymasks()
{
	int32_t base,j,c; uint64_t basemask;
	for (base=0; base<NUM_CURRENCIES; base++)
	{
		basemask = 0L;
		for (j=0; j<7; j++)
		{
			if ( (c= Currency_contracts[base][j]) >= 0 )
			{
				basemask |= (1L << c);
				//printf("(%s %lx) ",CONTRACTS[c],1L<<c);
			}
		}
		Currencymasks[base] = basemask;
		printf("0x%llx, ",(long long)basemask);
	}
}

double calc_primary_currencies(double logmatrix[8][8],double *bids,double *asks)
{
	uint64_t nonzmask; int32_t c,base,rel; double bid,ask;
	memset(logmatrix,0,sizeof(double)*8*8);
	nonzmask = 0;
	for (c=0; c<28; c++)
	{
		bid = bids[c];
		ask = asks[c];
		if ( bid != 0 && ask != 0 )
		{
			base = Contract_base[c];
			rel = Contract_rel[c];
			nonzmask |= (1L << c);
			logmatrix[base][rel] = log(bid);
			logmatrix[rel][base] = -log(ask);
			//printf("[%f %f] ",bid,ask);
		}
	}
	//printf("%07lx\n",nonzmask);
	if ( nonzmask != 0 )
	{
		bids[USDNUM] = (logmatrix[USD][EUR] + logmatrix[USD][JPY] + logmatrix[USD][GBP] + logmatrix[USD][AUD] + logmatrix[USD][CAD] + logmatrix[USD][CHF] + logmatrix[USD][NZD]) / 8.;
		asks[USDNUM] = -(logmatrix[EUR][USD] + logmatrix[JPY][USD] + logmatrix[GBP][USD] + logmatrix[AUD][USD] + logmatrix[CAD][USD] + logmatrix[CHF][USD] + logmatrix[NZD][USD]) / 8.;
        
		bids[EURNUM] = (logmatrix[EUR][USD] + logmatrix[EUR][JPY] + logmatrix[EUR][GBP] + logmatrix[EUR][AUD] + logmatrix[EUR][CAD] + logmatrix[EUR][CHF] + logmatrix[EUR][NZD]) / 8.;
		asks[EURNUM] = -(logmatrix[USD][EUR] + logmatrix[JPY][EUR] + logmatrix[GBP][EUR] + logmatrix[AUD][EUR] + logmatrix[CAD][EUR] + logmatrix[CHF][EUR] + logmatrix[NZD][EUR]) / 8.;
        
		bids[JPYNUM] = (logmatrix[JPY][USD] + logmatrix[JPY][EUR] + logmatrix[JPY][GBP] + logmatrix[JPY][AUD] + logmatrix[JPY][CAD] + logmatrix[JPY][CHF] + logmatrix[JPY][NZD]) / 8.;
		asks[JPYNUM] = -(logmatrix[USD][JPY] + logmatrix[EUR][JPY] + logmatrix[GBP][JPY] + logmatrix[AUD][JPY] + logmatrix[CAD][JPY] + logmatrix[CHF][JPY] + logmatrix[NZD][JPY]) / 8.;
        
		bids[GBPNUM] = (logmatrix[GBP][USD] + logmatrix[GBP][EUR] + logmatrix[GBP][JPY] + logmatrix[GBP][AUD] + logmatrix[GBP][CAD] + logmatrix[GBP][CHF] + logmatrix[GBP][NZD]) / 8.;
		asks[GBPNUM] = -(logmatrix[USD][GBP] + logmatrix[EUR][GBP] + logmatrix[JPY][GBP] + logmatrix[AUD][GBP] + logmatrix[CAD][GBP] + logmatrix[CHF][GBP] + logmatrix[NZD][GBP]) / 8.;
        
		bids[AUDNUM] = (logmatrix[AUD][USD] + logmatrix[AUD][EUR] + logmatrix[AUD][JPY] + logmatrix[AUD][GBP] + logmatrix[AUD][CAD] + logmatrix[AUD][CHF] + logmatrix[AUD][NZD]) / 8.;
		asks[AUDNUM] = -(logmatrix[USD][AUD] + logmatrix[EUR][AUD] + logmatrix[JPY][AUD] + logmatrix[GBP][AUD] + logmatrix[CAD][AUD] + logmatrix[CHF][AUD] + logmatrix[NZD][AUD]) / 8.;
        
		bids[CADNUM] = (logmatrix[CAD][USD] + logmatrix[CAD][EUR] + logmatrix[CAD][JPY] + logmatrix[CAD][GBP] + logmatrix[CAD][AUD] + logmatrix[CAD][CHF] + logmatrix[CAD][NZD]) / 8.;
		asks[CADNUM] = -(logmatrix[USD][CAD] + logmatrix[EUR][CAD] + logmatrix[JPY][CAD] + logmatrix[GBP][CAD] + logmatrix[AUD][CAD] + logmatrix[CHF][CAD] + logmatrix[NZD][CAD]) / 8.;
        
		bids[CHFNUM] = (logmatrix[CHF][USD] + logmatrix[CHF][EUR] + logmatrix[CHF][JPY] + logmatrix[CHF][GBP] + logmatrix[CHF][AUD] + logmatrix[CHF][CAD] + logmatrix[CHF][NZD]) / 8.;
		asks[CHFNUM] = -(logmatrix[USD][CHF] + logmatrix[EUR][CHF] + logmatrix[JPY][CHF] + logmatrix[GBP][CHF] + logmatrix[AUD][CHF] + logmatrix[CAD][CHF] + logmatrix[NZD][CHF]) / 8.;
        
		bids[NZDNUM] = (logmatrix[NZD][USD] + logmatrix[NZD][EUR] + logmatrix[NZD][JPY] + logmatrix[NZD][GBP] + logmatrix[NZD][AUD] + logmatrix[NZD][CAD] + logmatrix[NZD][CHF]) / 8.;
		asks[NZDNUM] = -(logmatrix[USD][NZD] + logmatrix[EUR][NZD] + logmatrix[JPY][NZD] + logmatrix[GBP][NZD] + logmatrix[AUD][NZD] + logmatrix[CAD][NZD] + logmatrix[CHF][NZD]) / 8.;
		if ( nonzmask != ((1<<28)-1) )
		{
			for (base=0; base<8; base++)
			{
				if ( (nonzmask & Currencymasks[base]) != Currencymasks[base] )
					bids[base+28] = asks[base+28] = 0;
				//else printf("%s %9.6f | ",CONTRACTS[base+28],_pairaved(bids[base+28],asks[base+28]));
			}
			//printf("keep.%07lx\n",nonzmask);
			return(0);
		}
		if ( 0 && nonzmask != 0 )
		{
			for (base=0; base<8; base++)
				printf("%s.%9.6f | ",CONTRACTS[base+28],_pairaved(bids[base+28],asks[base+28]));
			printf("%07llx\n",(long long)nonzmask);
		}
	}
	return(0);
}

uint64_t prices777_truefx(uint64_t *millistamps,double *bids,double *asks,double *opens,double *highs,double *lows,char *username,char *password,uint64_t idnum)
{
    char *truefxfmt = "http://webrates.truefx.com/rates/connect.html?f=csv&id=jl777:truefxtest:poll:1437671654417&c=EUR/USD,USD/JPY,GBP/USD,EUR/GBP,USD/CHF,AUD/NZD,CAD/CHF,CHF/JPY,EUR/AUD,EUR/CAD,EUR/JPY,EUR/CHF,USD/CAD,AUD/USD,GBP/JPY,AUD/CAD,AUD/CHF,AUD/JPY,EUR/NOK,EUR/NZD,GBP/CAD,GBP/CHF,NZD/JPY,NZD/USD,USD/NOK,USD/SEK";
    
    // EUR/USD,1437569931314,1.09,034,1.09,038,1.08922,1.09673,1.09384 USD/JPY,1437569932078,123.,778,123.,781,123.569,123.903,123.860 GBP/USD,1437569929008,1.56,332,1.56,337,1.55458,1.56482,1.55538 EUR/GBP,1437569931291,0.69,742,0.69,750,0.69710,0.70383,0.70338 USD/CHF,1437569932237,0.96,142,0.96,153,0.95608,0.96234,0.95748 EUR/JPY,1437569932237,134.,960,134.,972,134.842,135.640,135.476 EUR/CHF,1437569930233,1.04,827,1.04,839,1.04698,1.04945,1.04843 USD/CAD,1437569929721,1.30,231,1.30,241,1.29367,1.30340,1.29466 AUD/USD,1437569931700,0.73,884,0.73,890,0.73721,0.74395,0.74200 GBP/JPY,1437569931924,193.,500,193.,520,192.298,193.670,192.649
    char url[1024],userpass[1024],buf[128],base[64],rel[64],*str; cJSON *array; int32_t jpyflag,i,c,n = 0; double pre,pre2,bid,ask,open,high,low; long millistamp;
    //printf("truefx.(%s)(%s).%llu\n",username,password,(long long)idnum);
    url[0] = 0;
    if ( username[0] != 0 && password[0] != 0 )
    {
        if ( idnum == 0 )
        {
            sprintf(userpass,"http://webrates.truefx.com/rates/connect.html?f=csv&s=y&u=%s&p=%s&q=poll",username,password);
            if ( (str= issue_curl(userpass)) != 0 )
            {
                _stripwhite(str,0);
                printf("(%s) -> (%s)\n",userpass,str);
                sprintf(userpass,"%s:%s:poll:",username,password);
                idnum = calc_nxt64bits(str + strlen(userpass));
                free(str);
                printf("idnum.%llu\n",(long long)idnum);
            }
        }
        if ( idnum != 0 )
            sprintf(url,truefxfmt,username,password,(long long)idnum);
    }
    if ( url[0] == 0 )
        sprintf(url,"http://webrates.truefx.com/rates/connect.html?f=csv&s=y");
    if ( (str= issue_curl(url)) != 0 )
    {
        //printf("(%s) -> (%s)\n",url,str);
        while ( str[n + 0] != 0 && str[n] != '\n' && str[n] != '\r' )
        {
            for (i=jpyflag=0; str[n + i]!=' '&&str[n + i]!='\n'&&str[n + i]!='\r'&&str[n + i]!=0; i++)
            {
                if ( i > 0 && str[n+i] == ',' && str[n+i-1] == '.' )
                    str[n+i-1] = ' ', jpyflag = 1;
                else if ( i > 0 && str[n+i-1] == ',' && str[n+i] == '0' && str[n+i+1+2] == ',' )
                {
                    str[n+i] = ' ';
                    if ( str[n+i+1] == '0' )
                        str[n+i+1] = ' ', i++;
                }
            }
            memcpy(base,str+n,3), base[3] = 0;
            memcpy(rel,str+n+4,3), rel[3] = 0;
            str[n + i] = 0;
            //printf("str.(%s) (%s/%s) %d n.%d i.%d\n",str+n,base,rel,str[n],n,i);
            sprintf(buf,"[%s]",str+n+7+1);
            n += i + 1;
            if ( (array= cJSON_Parse(buf)) != 0 )
            {
                if ( is_cJSON_Array(array) != 0 )
                {
                    millistamp = (uint64_t)get_API_float(jitem(array,0));
                    pre = get_API_float(jitem(array,1));
                    bid = get_API_float(jitem(array,2));
                    pre2 = get_API_float(jitem(array,3));
                    ask = get_API_float(jitem(array,4));
                    open = get_API_float(jitem(array,5));
                    high = get_API_float(jitem(array,6));
                    low = get_API_float(jitem(array,7));
                    if ( jpyflag != 0 )
                        bid = pre + (bid / 1000.), ask = pre2 + (ask / 1000.);
                    else bid = pre + (bid / 100000.), ask = pre2 + (ask / 100000.);
                    if ( (c= prices777_contractnum(base,rel)) >= 0 )
                    {
                        if ( BUNDLE.truefx[c] == 0 )
                            BUNDLE.truefx[c] = prices777_initpair(0,0,"truefx",base,rel,0);
                        millistamps[c] = millistamp,opens[c] = open, highs[c] = high, lows[c] = low, bids[c] = bid, asks[c] = ask;
                        if ( Debuglevel > 2 )
                        {
                            if ( jpyflag != 0 )
                                printf("%s%s.%-2d %llu: %.3f %.3f %.3f | %.3f %.3f\n",base,rel,c,(long long)millistamp,open,high,low,bid,ask);
                            else printf("%s%s.%-2d %llu: %.5f %.5f %.5f | %.5f %.5f\n",base,rel,c,(long long)millistamp,open,high,low,bid,ask);
                        }
                    } else printf("unknown basepair.(%s) (%s)\n",base,rel);
                }
                free_json(array);
            } else printf("cant parse.(%s)\n",buf);
        }
        free(str);
    }
    return(idnum);
}

double prices777_fxcm(double lhlogmatrix[8][8],double logmatrix[8][8],double bids[64],double asks[64],double highs[64],double lows[64])
{
    char numstr[64],name[64],*xmlstr,*str; cJSON *json,*obj; int32_t i,j,c,flag,k,n = 0; double bid,ask,high,low;
    memset(bids,0,sizeof(*bids) * NUM_CONTRACTS), memset(asks,0,sizeof(*asks) * NUM_CONTRACTS);
    if ( (xmlstr= issue_curl("http://rates.fxcm.com/RatesXML")) != 0 )
    {
        _stripwhite(xmlstr,0);
        //printf("(%s)\n",xmlstr);
        i = 0;
        if ( strncmp("<?xml",xmlstr,5) == 0 )
            for (; xmlstr[i]!='>'&&xmlstr[i]!=0; i++)
                ;
        if ( xmlstr[i] == '>' )
            i++;
        for (j=0; xmlstr[i]!=0; i++)
        {
            if ( strncmp("<Rates>",&xmlstr[i],strlen("<Rates>")) == 0 )
                xmlstr[j++] = '[', i += strlen("<Rates>")-1;
            else if ( strncmp("<RateSymbol=",&xmlstr[i],strlen("<RateSymbol=")) == 0 )
            {
                if ( j > 1 )
                    xmlstr[j++] = ',';
                memcpy(&xmlstr[j],"{\"Symbol\":",strlen("{\"Symbol\":")), i += strlen("<RateSymbol=")-1, j += strlen("{\"Symbol\":");
            }
            else
            {
                char *strpairs[][2] = { { "<Bid>", "\"Bid\":" }, { "<Ask>", "\"Ask\":" }, { "<High>", "\"High\":" }, { "<Low>", "\"Low\":" }, { "<Direction>", "\"Direction\":" }, { "<Last>", "\"Last\":\"" } };
                for (k=0; k<sizeof(strpairs)/sizeof(*strpairs); k++)
                    if ( strncmp(strpairs[k][0],&xmlstr[i],strlen(strpairs[k][0])) == 0 )
                    {
                        memcpy(&xmlstr[j],strpairs[k][1],strlen(strpairs[k][1]));
                        i += strlen(strpairs[k][0])-1;
                        j += strlen(strpairs[k][1]);
                        break;
                    }
                if ( k == sizeof(strpairs)/sizeof(*strpairs) )
                {
                    char *ends[] = { "</Bid>", "</Ask>", "</High>", "</Low>", "</Direction>", "</Last>", "</Rate>", "</Rates>", ">" };
                    for (k=0; k<sizeof(ends)/sizeof(*ends); k++)
                        if ( strncmp(ends[k],&xmlstr[i],strlen(ends[k])) == 0 )
                        {
                            i += strlen(ends[k])-1;
                            if ( strcmp("</Rate>",ends[k]) == 0 )
                                xmlstr[j++] = '}';
                            else if ( strcmp("</Rates>",ends[k]) == 0 )
                                xmlstr[j++] = ']';
                            else if ( strcmp("</Last>",ends[k]) == 0 )
                                xmlstr[j++] = '\"';
                            else xmlstr[j++] = ',';
                            break;
                        }
                    if ( k == sizeof(ends)/sizeof(*ends) )
                        xmlstr[j++] = xmlstr[i];
                }
            }
        }
        xmlstr[j] = 0;
        if ( (json= cJSON_Parse(xmlstr)) != 0 )
        {
            /*<Rate Symbol="USDJPY">
            <Bid>123.763</Bid>
            <Ask>123.786</Ask>
            <High>123.956</High>
            <Low>123.562</Low>
            <Direction>-1</Direction>
            <Last>08:49:15</Last>*/
            //printf("Parsed stupid XML! (%s)\n",xmlstr);
            if ( is_cJSON_Array(json) != 0 && (n= cJSON_GetArraySize(json)) != 0 )
            {
                for (i=0; i<n; i++)
                {
                    obj = jitem(json,i);
                    flag = 0;
                    c = -1;
                    if ( (str= jstr(obj,"Symbol")) != 0 && strlen(str) < 15 )
                    {
                        strcpy(name,str);
                        if ( strcmp(name,"USDCNH") == 0 )
                            strcpy(name,"USDCNY");
                        copy_cJSON(numstr,jobj(obj,"Bid")), bid = atof(numstr);
                        copy_cJSON(numstr,jobj(obj,"Ask")), ask = atof(numstr);
                        copy_cJSON(numstr,jobj(obj,"High")), high = atof(numstr);
                        copy_cJSON(numstr,jobj(obj,"Low")), low = atof(numstr);
                        if ( (c= prices777_contractnum(name,0)) >= 0 )
                        {
                            bids[c] = bid, asks[c] = ask, highs[c] = high, lows[c] = low;
                            //printf("c.%d (%s) %f %f\n",c,name,bid,ask);
                            flag = 1;
                            if ( BUNDLE.fxcm[c] == 0 )
                            {
                                printf("max.%ld FXCM: not initialized.(%s) %d\n",sizeof(CONTRACTS)/sizeof(*CONTRACTS),name,c);
                                BUNDLE.fxcm[c] = prices777_initpair(0,0,"fxcm",name,0,0);
                            }
                        } else printf("cant find.%s\n",name);//, getchar();
                    }
                    if ( flag == 0 )
                        printf("FXCM: Error finding.(%s) c.%d (%s)\n",name,c,cJSON_Print(obj));
                }
            }
            free_json(json);
        } else printf("couldnt parse.(%s)\n",xmlstr);
        free(xmlstr);
    }
    calc_primary_currencies(lhlogmatrix,lows,highs);
    return(calc_primary_currencies(logmatrix,bids,asks));
}

double prices777_instaforex(double logmatrix[8][8],uint32_t timestamps[NUM_COMBINED+1],double bids[128],double asks[128])
{
    //{"NZDUSD":{"symbol":"NZDUSD","lasttime":1437580206,"digits":4,"change":"-0.0001","bid":"0.6590","ask":"0.6593"},
    char numstr[64],*jsonstr,*str; cJSON *json,*item; int32_t i,c;
    memset(timestamps,0,sizeof(*timestamps) * (NUM_COMBINED + 1)), memset(bids,0,sizeof(*bids) * (NUM_COMBINED + 1)), memset(asks,0,sizeof(*asks) * (NUM_COMBINED + 1));
    if ( (jsonstr= issue_curl("https://quotes.instaforex.com/get_quotes.php?q=NZDUSD,NZDCHF,NZDCAD,NZDJPY,GBPNZD,EURNZD,AUDNZD,CADJPY,CADCHF,USDCAD,EURCAD,GBPCAD,AUDCAD,USDCHF,CHFJPY,EURCHF,GBPCHF,AUDCHF,EURUSD,EURAUD,EURJPY,EURGBP,GBPUSD,GBPJPY,GBPAUD,USDJPY,AUDJPY,AUDUSD,XAUUSD&m=json")) != 0 )
    {
       // printf("(%s)\n",jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            for (i=0; i<=NUM_CONTRACTS; i++)
            {
                if ( i < NUM_CONTRACTS )
                    str = CONTRACTS[i], c = i;
                else str = "XAUUSD", c = prices777_contractnum(str,0);
                if ( (item= jobj(json,str)) != 0 )
                {
                    timestamps[c] = juint(item,"lasttime");
                    copy_cJSON(numstr,jobj(item,"bid")), bids[c] = atof(numstr);
                    copy_cJSON(numstr,jobj(item,"ask")), asks[c] = atof(numstr);
                    //if ( c < NUM_CONTRACTS && Contract_rel[c] == JPY )
                    //    bids[i] /= 100., asks[i] /= 100.;
                    if ( Debuglevel > 2 )
                        printf("%s.(%.6f %.6f) ",str,bids[c],asks[c]);
                    if ( BUNDLE.instaforex[c] == 0 )
                        BUNDLE.instaforex[c] = prices777_initpair(0,0,"instaforex",str,0,0);
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(calc_primary_currencies(logmatrix,bids,asks));
}

int32_t prices777_ecbparse(char *date,double *prices,char *url,int32_t basenum)
{
    char *jsonstr,*relstr,*basestr; int32_t count=0,i,relnum; cJSON *json,*ratesobj,*item;
    if ( (jsonstr= issue_curl(url)) != 0 )
    {
        printf("(%s)\n",jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            copy_cJSON(date,jobj(json,"date"));
            if ( (basestr= jstr(json,"base")) != 0 && strcmp(basestr,CURRENCIES[basenum]) == 0 && (ratesobj= jobj(json,"rates")) != 0 && (item= ratesobj->child) != 0 )
            {
                while ( item != 0 )
                {
                    if ( (relstr= get_cJSON_fieldname(item)) != 0 && (relnum= prices777_basenum(relstr)) >= 0 )
                    {
                        i = basenum*MAX_CURRENCIES + relnum;
                        prices[i] = item->valuedouble;
                        //if ( basenum == JPYNUM )
                       //    prices[i] *= 100.;
                       // else if ( relnum == JPYNUM )
                       //     prices[i] /= 100.;
                        count++;
                        printf("(%02d:%02d %f) ",basenum,relnum,prices[i]);
                    } else printf("cant find.(%s)\n",relstr), getchar();
                    item = item->next;
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(count);
}

int32_t prices777_ecb(char *date,double *prices,int32_t year,int32_t month,int32_t day)
{
    // http://api.fixer.io/latest?base=CNH
    // http://api.fixer.io/2000-01-03?base=USD
    // "USD", "EUR", "JPY", "GBP", "AUD", "CAD", "CHF", "NZD"
    char baseurl[512],tmpdate[64],url[512],checkdate[16]; int32_t basenum,count,i,iter,nonz;
    checkdate[0] = 0;
    if ( year == 0 )
        strcpy(baseurl,"http://api.fixer.io/latest?base=");
    else
    {
        sprintf(checkdate,"%d-%02d-%02d",year,month,day);
        sprintf(baseurl,"http://api.fixer.io/%s?base=",checkdate);
    }
    count = 0;
    for (iter=0; iter<2; iter++)
    {
        for (basenum=0; basenum<sizeof(CURRENCIES)/sizeof(*CURRENCIES); basenum++)
        {
            if ( strcmp(CURRENCIES[basenum],"XAU") == 0 )
                break;
            if ( iter == 0 )
            {
                sprintf(url,"%s%s",baseurl,CURRENCIES[basenum]);
                count += prices777_ecbparse(basenum == 0 ? date : tmpdate,prices,url,basenum);
                if ( (basenum != 0 && strcmp(tmpdate,date) != 0) || (checkdate[0] != 0 && strcmp(checkdate,date) != 0) )
                {
                    printf("date mismatch (%s) != (%s) or checkdate.(%s)\n",tmpdate,date,checkdate);
                    return(-1);
                }
            }
            else
            {
                for (nonz=i=0; i<sizeof(CURRENCIES)/sizeof(*CURRENCIES); i++)
                {
                    if ( strcmp(CURRENCIES[i],"XAU") == 0 )
                        break;
                    if ( prices[MAX_CURRENCIES*basenum + i] != 0. )
                        nonz++;
                    if ( Debuglevel > 2 )
                        printf("%8.5f ",prices[MAX_CURRENCIES*basenum + i]);
                }
                if ( Debuglevel > 2 )
                    printf("%s.%d %d\n",CURRENCIES[basenum],basenum,nonz);
            }
        }
    }
    return(count);
}

struct exchange_info *get_exchange(int32_t exchangeid) { return(&Exchanges[exchangeid]); }
char *exchange_str(int32_t exchangeid) { return(Exchanges[exchangeid].name); }

int32_t exchange_supports(uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    int32_t exchangeid; struct exchange_info *exchange;
    if ( Debuglevel > 1 )
        printf("genpairs.(%llu %llu) starts with n.%d\n",(long long)baseid,(long long)relid,n);
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        if ( exchangeid == INSTANTDEX_UNCONFID )
            continue;
        if ( (exchange= get_exchange(exchangeid)) != 0 )
        {
            if ( exchange->supports != 0 )//&& exchange->ramparse != ramparse_stub )
                n = (*exchange->supports)(exchangeid,assetids,n,baseid,relid);
        } else break;
    }
    return(n);
}

int32_t add_halfbooks(uint64_t *assetids,int32_t n,uint64_t refid,uint64_t baseid,uint64_t relid,int32_t exchangeid)
{
    if ( baseid != refid )
        n = add_exchange_assetid(assetids,n,baseid,refid,exchangeid);
    if ( relid != refid )
        n = add_exchange_assetid(assetids,n,relid,refid,exchangeid);
    return(n);
}

int32_t gen_assetpair_list(uint64_t *assetids,long max,uint64_t baseid,uint64_t relid)
{
    int32_t exchange_supports(uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid);
    int32_t n = 0;
    n = add_exchange_assetid(assetids,n,baseid,relid,INSTANTDEX_EXCHANGEID);
    n = add_exchange_assetid(assetids,n,relid,baseid,INSTANTDEX_EXCHANGEID);
    n = add_halfbooks(assetids,n,NXT_ASSETID,baseid,relid,INSTANTDEX_EXCHANGEID);
    n = add_halfbooks(assetids,n,BTC_ASSETID,baseid,relid,INSTANTDEX_EXCHANGEID);
    n = add_halfbooks(assetids,n,BTCD_ASSETID,baseid,relid,INSTANTDEX_EXCHANGEID);
    n = exchange_supports(assetids,n,baseid,relid);
    if ( Debuglevel > 1 )
        printf("genpairs.(%llu %llu) starts with n.%d\n",(long long)baseid,(long long)relid,n);
    if ( (n % 3) != 0 || n > max )
        printf("gen_assetpair_list: baseid.%llu relid.%llu -> n.%d??? mod.%d\n",(long long)baseid,(long long)relid,n,n%3);
    return(n/3);
}

int32_t NXT_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    if ( baseid != NXT_ASSETID )
        n = add_NXT_assetids(assetids,n,baseid);
    if ( relid != NXT_ASSETID )
        n = add_NXT_assetids(assetids,n,relid);
    return(n);
}

int32_t poloniex_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    char *poloassets[][8] = { { "UNITY", "12071612744977229797" },  { "JLH", "6932037131189568014" },  { "XUSD", "12982485703607823902" },  { "LQD", "4630752101777892988" },  { "NXTI", "14273984620270850703" }, { "CNMT", "7474435909229872610", "6220108297598959542" } };
   return(add_exchange_assetids(assetids,n,BTC_ASSETID,baseid,relid,exchangeid,poloassets,(int32_t)(sizeof(poloassets)/sizeof(*poloassets))));
}

int32_t bter_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    char *bterassets[][8] = { { "UNITY", "12071612744977229797" },  { "ATOMIC", "11694807213441909013" },  { "DICE", "18184274154437352348" },  { "MRKT", "134138275353332190" },  { "MGW", "10524562908394749924" } };
    uint64_t unityid = calc_nxt64bits("12071612744977229797");
    n = add_exchange_assetids(assetids,n,BTC_ASSETID,baseid,relid,exchangeid,bterassets,(int32_t)(sizeof(bterassets)/sizeof(*bterassets)));
    if ( baseid == unityid || relid == unityid )
    {
        n = add_exchange_assetid(assetids,n,unityid,BTC_ASSETID,exchangeid);
        n = add_exchange_assetid(assetids,n,unityid,NXT_ASSETID,exchangeid);
        n = add_exchange_assetid(assetids,n,unityid,CNY_ASSETID,exchangeid);
    }
    return(n);
}

int32_t bittrex_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    return(add_exchange_assetids(assetids,n,BTC_ASSETID,baseid,relid,exchangeid,0,0));
}

int32_t btc38_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    return(add_exchange_assetids(assetids,n,BTC_ASSETID,baseid,relid,exchangeid,0,0));
    return(add_exchange_assetids(assetids,n,CNY_ASSETID,baseid,relid,exchangeid,0,0));
}

int32_t okcoin_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {LTC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t huobi_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    return(_add_related(assetids,n,BTC_ASSETID,CNY_ASSETID,exchangeid,baseid,relid));
}

int32_t bitfinex_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {LTC_ASSETID, USD_ASSETID}, {LTC_ASSETID, BTC_ASSETID}, {DASH_ASSETID, BTC_ASSETID}, {DASH_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t btce_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {LTC_ASSETID, BTC_ASSETID}, {LTC_ASSETID, USD_ASSETID}, {BTC_ASSETID, RUR_ASSETID}, {PPC_ASSETID, BTC_ASSETID}, {PPC_ASSETID, USD_ASSETID}, {LTC_ASSETID, RUR_ASSETID}, {NMC_ASSETID, BTC_ASSETID}, {NMC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t bityes_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {LTC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t coinbase_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t bitstamp_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t lakebtc_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

int32_t exmo_supports(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid)
{
    uint64_t supported[][2] = { {BTC_ASSETID, USD_ASSETID}, {BTC_ASSETID, EUR_ASSETID} , {BTC_ASSETID, RUR_ASSETID} };
    return(add_related(assetids,n,supported,sizeof(supported)/sizeof(*supported),exchangeid,baseid,relid));
}

cJSON *exchanges_json()
{
    struct exchange_info *exchange; int32_t exchangeid,n = 0; char api[4]; cJSON *item,*array = cJSON_CreateArray();
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        item = cJSON_CreateObject();
        exchange = &Exchanges[exchangeid];
        if ( exchange->name[0] == 0 )
            break;
        cJSON_AddItemToObject(item,"name",cJSON_CreateString(exchange->name));
        memset(api,0,sizeof(api));
        if ( exchange->trade != 0 )
        {
            if ( exchange->apikey[0] != 0 )
                api[n++] = 'K';
            if ( exchange->apisecret[0] != 0 )
                api[n++] = 'S';
            if ( exchange->userid[0] != 0 )
                api[n++] = 'U';
            cJSON_AddItemToObject(item,"trade",cJSON_CreateString(api));
        }
        cJSON_AddItemToArray(array,item);
    }
    return(array);
}

int32_t is_exchange_nxt64bits(uint64_t nxt64bits)
{
    int32_t exchangeid;
    struct exchange_info *exchange = 0;
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        exchange = &Exchanges[exchangeid];
        // printf("(%s).(%llu vs %llu) ",exchange->name,(long long)exchange->nxt64bits,(long long)nxt64bits);
        if ( exchange->name[0] == 0 )
            return(0);
        if ( exchange->nxt64bits == nxt64bits )
            return(1);
    }
    printf("no exchangebits match\n");
    return(0);
}

struct exchange_info *find_exchange(int32_t *exchangeidp,char *exchangestr)
{
    int32_t exchangeid;
    struct exchange_info *exchange = 0;
    for (exchangeid=0; exchangeid<MAX_EXCHANGES; exchangeid++)
    {
        exchange = &Exchanges[exchangeid];
        //printf("(%s v %s) ",exchangestr,exchange->name);
        if ( exchange->name[0] == 0 )
        {
            strcpy(exchange->name,exchangestr);
            exchange->exchangeid = exchangeid;
            exchange->nxt64bits = stringbits(exchangestr);
            printf("CREATE EXCHANGE.(%s) id.%d %llu\n",exchangestr,exchangeid,(long long)exchange->nxt64bits);
            break;
        }
        if ( strcmp(exchangestr,exchange->name) == 0 )
            break;
    }
    if ( exchange != 0 && exchangeidp != 0 )
        *exchangeidp = exchange->exchangeid;
    return(exchange);
}

uint64_t submit_to_exchange(int32_t exchangeid,char **jsonstrp,uint64_t assetid,uint64_t qty,uint64_t priceNQT,int32_t dir,uint64_t nxt64bits,char *NXTACCTSECRET,char *triggerhash,char *comment,uint64_t otherNXT,char *base,char *rel,double price,double volume,uint32_t triggerheight)
{
    uint64_t txid = 0;
    char assetidstr[64],*cmd,*retstr = 0;
    int32_t ap_type,decimals;
    struct exchange_info *exchange;
    *jsonstrp = 0;
    expand_nxt64bits(assetidstr,assetid);
    ap_type = get_assettype(&decimals,assetidstr);
    if ( dir == 0 || priceNQT == 0 )
        cmd = (ap_type == 2 ? "transferAsset" : "transferCurrency"), priceNQT = 0;
    else cmd = ((dir > 0) ? (ap_type == 2 ? "placeBidOrder" : "currencyBuy") : (ap_type == 2 ? "placeAskOrder" : "currencySell")), otherNXT = 0;
    if ( exchangeid == INSTANTDEX_NXTAEID || exchangeid == INSTANTDEX_UNCONFID )
    {
        if ( assetid != NXT_ASSETID && qty != 0 && (dir == 0 || priceNQT != 0) )
        {
            printf("submit to exchange.%s (%s) dir.%d\n",Exchanges[exchangeid].name,comment,dir);
            txid = submit_triggered_nxtae(jsonstrp,ap_type == 5,cmd,nxt64bits,NXTACCTSECRET,assetid,qty,priceNQT,triggerhash,comment,otherNXT,triggerheight);
            if ( *jsonstrp != 0 )
                txid = 0;
        }
    }
    else if ( exchangeid < MAX_EXCHANGES && (exchange= &Exchanges[exchangeid]) != 0 && exchange->exchangeid == exchangeid && exchange->trade != 0 )
    {
        printf("submit_to_exchange.(%d) dir.%d price %f vol %f | inv %f %f (%s)\n",exchangeid,dir,price,volume,1./price,price*volume,comment);
        if ( (txid= (*exchange->trade)(&retstr,exchange,base,rel,dir,price,volume)) == 0 )
            printf("illegal combo (%s/%s) ret.(%s)\n",base,rel,retstr!=0?retstr:"");
    }
    return(txid);
}

char *prices777_trade(char *exchangestr,char *base,char *rel,int32_t dir,double price,double volume)
{
    char *retstr; struct exchange_info *exchange;
    printf("check %s/%s\n",base,rel);
    if ( (exchange= find_exchange(0,exchangestr)) != 0 )
    {
        if ( exchange->trade != 0 )
        {
            printf(" issue dir.%d %s/%s price %f vol %f -> %s\n",dir,base,rel,price,volume,exchangestr);
            (*exchange->trade)(&retstr,exchange,base,rel,dir,price,volume);
            return(retstr);
        } else return(clonestr("{\"error\":\"no trade function for exchange\"}\n"));
    } else return(clonestr("{\"error\":\"exchange not active, check SuperNET.conf exchanges array\"}\n"));
}

struct prices777 *prices777_initpair(int32_t needfunc,void (*updatefunc)(struct prices777 *prices,int32_t maxdepth),char *exchange,char *base,char *rel,double decay)
{
    static long allocated;
    struct exchange_pair { char *exchange; void (*updatefunc)(struct prices777 *prices,int32_t maxdepth); int32_t (*supports)(int32_t exchangeid,uint64_t *assetids,int32_t n,uint64_t baseid,uint64_t relid); uint64_t (*trade)(char **retstrp,struct exchange_info *exchange,char *base,char *rel,int32_t dir,double price,double volume); } pairs[] =
    {
        {"poloniex", prices777_poloniex, poloniex_supports, poloniex_trade }, {"bitfinex", prices777_bitfinex, bitfinex_supports },
        {"btc38", prices777_btc38, btc38_supports, btc38_trade }, {"bter", prices777_bter, bter_supports, bter_trade },
        {"btce", prices777_btce, btce_supports, btce_trade }, {"bitstamp", prices777_bitstamp, bitstamp_supports },
        {"bittrex", prices777_bittrex, bittrex_supports, bittrex_trade }, {"okcoin", prices777_okcoin, okcoin_supports },
        {"huobi", prices777_huobi, huobi_supports }, {"bityes", prices777_bityes, bityes_supports },
        {"coinbase", prices777_coinbase, coinbase_supports }, {"lakebtc", prices777_lakebtc, lakebtc_supports },
        {"exmo", prices777_exmo, exmo_supports },
        {"truefx", 0 }, {"ecb", 0 }, {"instaforex", 0 }, {"fxcm", 0 }, {"yahoo", 0 },
    };
    int32_t i,rellen; char basebuf[64],relbuf[64]; struct exchange_info *exchangeptr;
    struct prices777 *prices;
    for (i=0; i<BUNDLE.num; i++)
    {
        if ( strcmp(exchange,BUNDLE.ptrs[i]->exchange) == 0 && strcmp(base,BUNDLE.ptrs[i]->origbase) == 0 )
        {
            if ( (rel == 0 && BUNDLE.ptrs[i]->origrel[0] == 0) || strcmp(rel,BUNDLE.ptrs[i]->origrel) == 0 )
            {
                printf("duplicate initpair.(%s) (%s) (%s)\n",exchange,base,BUNDLE.ptrs[i]->origrel);
                return(0);
            }
        }
    }
    prices = calloc(1,sizeof(*prices));
    strcpy(prices->origbase,base);
    if ( rel != 0 )
        strcpy(prices->origrel,rel);
    allocated += sizeof(*prices);
    safecopy(prices->exchange,exchange,sizeof(prices->exchange));
    if ( is_decimalstr(base) != 0 && strcmp(rel,"NXT") == 0 )
    {
        prices->contractnum = calc_nxt64bits(base);
        prices->ap_mult = get_assetmult(prices->contractnum);
        prices->nxtbooks = calloc(1,sizeof(*prices->nxtbooks));
    }
    else
    {
        safecopy(prices->base,base,sizeof(prices->base)), touppercase(prices->base);
        safecopy(prices->lbase,base,sizeof(prices->lbase)), tolowercase(prices->lbase);
        if ( rel == 0 && prices777_ispair(basebuf,relbuf,base) >= 0 )
        {
            base = basebuf, rel = relbuf;
            //printf("(%s) is a pair (%s)+(%s)\n",base,basebuf,relbuf);
        }
    }
    if ( rel != 0 && rel[0] != 0 )
    {
        rellen = (int32_t)(strlen(rel) + 1);
        safecopy(prices->rel,rel,sizeof(prices->rel)), touppercase(prices->rel);
        safecopy(prices->lrel,rel,sizeof(prices->lrel)), tolowercase(prices->lrel);
        strcpy(prices->contract,prices->base);
        if ( strcmp(prices->rel,&prices->contract[strlen(prices->contract)-3]) != 0 )
            strcat(prices->contract,prices->rel);
        printf("base.(%s) rel.(%s)\n",prices->base,prices->rel);
    }
    else
    {
        rellen = 0;
        strcpy(prices->contract,base);
    }
    printf("%s init_pair.(%s) (%s)(%s).%lld -> (%s)\n",_mbstr(allocated),exchange,base,rel!=0?rel:"",(long long)prices->contractnum,prices->contract);
    strcpy(prices->key,prices->exchange), prices->keysize += strlen(prices->exchange) + 1;
    memcpy(&prices->key[prices->keysize], prices->base,strlen(prices->base)+1), prices->keysize += strlen(prices->base) + 1;
    if ( rellen != 0 )
        memcpy(&prices->key[prices->keysize], prices->rel,rellen), prices->keysize += rellen;
    prices->decay = decay, prices->oppodecay = (1. - decay);
    prices->RTflag = 1;
    if ( (exchangeptr= find_exchange(0,exchange)) != 0 )
    {
        prices->exchangeid = exchangeptr->exchangeid;
        if ( (exchangeptr->updatefunc= updatefunc) == 0 )
        {
            for (i=0; i<sizeof(pairs)/sizeof(*pairs); i++)
                
                if ( strcmp(exchange,pairs[i].exchange) == 0 )
                {
                    if ( (exchangeptr->updatefunc= pairs[i].updatefunc) != 0 )
                    {
                        exchangeptr->trade = pairs[i].trade;
                        exchangeptr->refcount++;
                    }
                    return(prices);
                }
        }
    }
    if ( needfunc != 0 )
        printf("prices777_init error: cant find update function for (%s) (%s) (%s)\n",exchange,base,rel!=0?rel:"");
    printf("initialized.(%s).%lld\n",prices->contract,(long long)prices->contractnum);
    return(prices);
}

void prices777_exchangeloop(void *ptr)
{
    struct prices777 *prices; int32_t i,n; struct exchange_info *exchange = ptr;
    while ( 1 )
    {
        for (i=n=0; i<BUNDLE.num; i++)
        {
            if ( (prices= BUNDLE.ptrs[i]) != 0 && prices->exchangeid == exchange->exchangeid )
            {
                if ( milliseconds() > exchange->lastupdate + exchange->pollgap && milliseconds() > prices->lastupdate + 60000 )
                {
                    (*exchange->updatefunc)(prices,MAX_DEPTH);
                    if ( prices->orderbook[0][0][0] != 0. && prices->orderbook[0][1][0] != 0 )
                        prices->lastprice = _pairaved(prices->orderbook[0][0][0],prices->orderbook[0][1][0]);
                    exchange->lastupdate = milliseconds(), prices->lastupdate = milliseconds();
                    kv777_write(BUNDLE.kv,prices->key,prices->keysize,prices,sizeof(*prices));
                    n++;
                }
            }
        }
        if ( n == 0 )
            sleep(60);
        else sleep(1);
    }
}

void prices777_poll(uint64_t refbaseid,uint64_t refrelid)
{
    int32_t unstringbits(char *buf,uint64_t bits);
    uint64_t assetids[8192],baseid,relid; int32_t i,n,exchangeid,flag; char base[16],rel[16];
    if ( (n= gen_assetpair_list(assetids,sizeof(assetids)/sizeof(*assetids),refbaseid,refrelid)) > 0 )
    {
        for (i=0; i<n; i++)
        {
            baseid = assetids[i*3], relid = assetids[i*3 + 1], exchangeid = (int32_t)assetids[i*3 + 2];
            unstringbits(base,baseid), unstringbits(rel,relid);
            flag = Exchanges[exchangeid].refcount;
            if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,exchange_str(exchangeid),base,rel,0.)) != 0 )
            {
                if ( Exchanges[exchangeid].refcount == flag+1 )
                {
                    printf("First pair for (%s), start polling]\n",exchange_str(exchangeid));
                    portable_thread_create((void *)prices777_exchangeloop,&Exchanges[exchangeid]);
                }
                BUNDLE.num++;
            }
        }
    }
}

int32_t prices777_init(char *jsonstr)
{
    char *btcdexchanges[] = { "poloniex", "bittrex", "bter" };
    char *btcusdexchanges[] = { "bityes", "bitfinex", "bitstamp", "itbit", "okcoin", "coinbase", "btce", "lakebtc", "exmo" };
    cJSON *json=0,*item,*exchanges; int32_t i,n; char *exchange,*base,*rel,*contract; struct exchange_info *exchangeptr;
    if ( BUNDLE.ptrs[0] != 0 )
        return(0);
   // "BTCUSD", "NXTBTC", "SuperNET", "ETHBTC", "LTCBTC", "XMRBTC", "BTSBTC", "XCPBTC",  // BTC priced
    if ( 0 )
    {
        if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,"huobi","BTC","USD",0.)) != 0 )
            BUNDLE.num++;
        if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,"btc38","CNY","NXT",0.)) != 0 )
            BUNDLE.num++;
        if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,"okcoin","LTC","BTC",0.)) != 0 )
            BUNDLE.num++;
        if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,"poloniex","LTC","BTC",0.)) != 0 )
            BUNDLE.num++;
        if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,"poloniex","XMR","BTC",0.)) != 0 )
            BUNDLE.num++;
        if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,"poloniex","BTS","BTC",0.)) != 0 )
            BUNDLE.num++;
        if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,"poloniex","XCP","BTC",0.)) != 0 )
            BUNDLE.num++;
        for (i=0; i<sizeof(btcdexchanges)/sizeof(*btcdexchanges); i++)
            if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,btcusdexchanges[i],"BTC","USD",0.)) != 0 )
                BUNDLE.num++;
    }
    for (i=0; i<sizeof(btcdexchanges)/sizeof(*btcdexchanges); i++)
        if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,btcdexchanges[i],"BTCD","BTC",0.)) != 0 )
            BUNDLE.num++;
    if ( (json= cJSON_Parse(jsonstr)) != 0 && (exchanges= jarray(&n,json,"prices")) != 0 )
    {
        for (i=0; i<n; i++)
        {
            item = jitem(exchanges,i);
            exchange = jstr(item,"exchange"), base = jstr(item,"base"), rel = jstr(item,"rel");
            if ( (base == 0 || rel == 0) && (contract= jstr(item,"contract")) != 0 )
                rel = 0, base = contract;
            else contract = 0;
            if ( (exchangeptr= find_exchange(0,exchange)) != 0 )
            {
                exchangeptr->pollgap = get_API_int(cJSON_GetObjectItem(json,"pollgap"),DEFAULT_POLLGAP);
                extract_cJSON_str(exchangeptr->apikey,sizeof(exchangeptr->apikey),json,"key");
                extract_cJSON_str(exchangeptr->userid,sizeof(exchangeptr->userid),json,"userid");
                extract_cJSON_str(exchangeptr->apisecret,sizeof(exchangeptr->apisecret),json,"secret");
            }
            if ( strcmp(exchange,"truefx") == 0 )
            {
                copy_cJSON(BUNDLE.truefxuser,jobj(item,"truefxuser"));
                copy_cJSON(BUNDLE.truefxpass,jobj(item,"truefxpass"));
                printf("truefx.(%s %s)\n",BUNDLE.truefxuser,BUNDLE.truefxpass);
            }
            else if ( (BUNDLE.ptrs[BUNDLE.num]= prices777_initpair(1,0,exchange,base,rel,jdouble(item,"decay"))) != 0 )
                BUNDLE.num++;
        }
    }
    if ( json != 0 )
        free_json(json);
    for (i=0; i<MAX_EXCHANGES; i++)
    {
        exchangeptr = &Exchanges[i];
        if ( exchangeptr->refcount > 0 )
            portable_thread_create((void *)prices777_exchangeloop,exchangeptr);
    }
    return(0);
}

double prices777_yahoo(char *metal)
{
    // http://finance.yahoo.com/webservice/v1/symbols/allcurrencies/quote?format=json
    // http://finance.yahoo.com/webservice/v1/symbols/XAU=X/quote?format=json
    // http://finance.yahoo.com/webservice/v1/symbols/XAG=X/quote?format=json
    // http://finance.yahoo.com/webservice/v1/symbols/XPT=X/quote?format=json
    // http://finance.yahoo.com/webservice/v1/symbols/XPD=X/quote?format=json
    char url[1024],*jsonstr; cJSON *json,*obj,*robj,*item,*field; double price = 0.;
    sprintf(url,"http://finance.yahoo.com/webservice/v1/symbols/%s=X/quote?format=json",metal);
    if ( (jsonstr= issue_curl(url)) != 0 )
    {
        //printf("(%s)\n",jsonstr);
        if ( (json= cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= jobj(json,"list")) != 0 && (robj= jobj(obj,"resources")) != 0 && (item= jitem(robj,0)) != 0 )
            {
                if ( (robj= jobj(item,"resource")) != 0 && (field= jobj(robj,"fields")) != 0 && (price= jdouble(field,"price")) != 0 )
                    price = 1. / price;
            }
            free_json(json);
        }
        free(jsonstr);
    }
    printf("(%s %f) ",metal,price);
    return(price);
}

cJSON *url_json(char *url)
{
    char *jsonstr; cJSON *json = 0;
    if ( (jsonstr= issue_curl(url)) != 0 )
    {
        //printf("(%s) -> (%s)\n",url,jsonstr);
        json = cJSON_Parse(jsonstr);
        free(jsonstr);
    }
    return(json);
}

cJSON *url_json2(char *url)
{
    char *jsonstr; cJSON *json = 0;
    if ( (jsonstr= issue_curl(url)) != 0 )
    {
        printf("(%s) -> (%s)\n",url,jsonstr);
        json = cJSON_Parse(jsonstr);
        free(jsonstr);
    }
    return(json);
}

void prices777_btcprices(int32_t enddatenum,int32_t numdates)
{
    int32_t i,n,year,month,day,seconds,datenum; char url[1024],date[64],*dstr,*str; uint32_t timestamp,utc32[MAX_SPLINES];
    cJSON *coindesk,*quandl,*btcdhist,*bpi,*array,*item;
    double btcddaily[MAX_SPLINES],cdaily[MAX_SPLINES],qdaily[MAX_SPLINES],ask,high,low,bid,close,vol,quotevol,open,price = 0.;
    coindesk = url_json("http://api.coindesk.com/v1/bpi/historical/close.json");
    sprintf(url,"https://poloniex.com/public?command=returnChartData&currencyPair=BTC_BTCD&start=%ld&end=9999999999&period=86400",time(NULL)-numdates*3600*24);
    if ( (bpi= jobj(coindesk,"bpi")) != 0 )
    {
        datenum = enddatenum;
        memset(utc32,0,sizeof(utc32));
        memset(cdaily,0,sizeof(cdaily));
        if ( datenum == 0 )
        {
            datenum = OS_conv_unixtime(&seconds,(uint32_t)time(NULL));
            printf("got datenum.%d %d %d %d\n",datenum,seconds/3600,(seconds/60)%24,seconds%60);
        }
        for (i=0; i<numdates; i++)
        {
            expand_datenum(date,datenum);
            if ( (price= jdouble(bpi,date)) != 0 )
            {
                utc32[numdates - 1 - i] = OS_conv_datenum(datenum,12,0,0);
                cdaily[numdates - 1 - i] = price * .001;
                //printf("(%s %u %f) ",date,utc32[numdates - 1 - i],price);
            }
            datenum = ecb_decrdate(&year,&month,&day,date,datenum);
        }
        prices777_genspline(&BUNDLE.splines[MAX_CURRENCIES],MAX_CURRENCIES,"coindesk",utc32,cdaily,numdates,cdaily);
        
    } else printf("no bpi\n");
    quandl = url_json("https://www.quandl.com/api/v1/datasets/BAVERAGE/USD.json?rows=64");
    if ( (str= jstr(quandl,"updated_at")) != 0 && (datenum= conv_date(&seconds,str)) > 0 && (array= jarray(&n,quandl,"data")) != 0 )
    {
        printf("datenum.%d data.%d %d\n",datenum,n,cJSON_GetArraySize(array));
        memset(utc32,0,sizeof(utc32)), memset(qdaily,0,sizeof(qdaily));
        for (i=0; i<n&&i<MAX_SPLINES; i++)
        {
            // ["Date","24h Average","Ask","Bid","Last","Total Volume"]
            // ["2015-07-25",289.27,288.84,288.68,288.87,44978.61]
            item = jitem(array,i);
            printf("(%s) ",cJSON_Print(item));
            if ( (dstr= jstr(jitem(item,0),0)) != 0 && (datenum= conv_date(&seconds,dstr)) > 0 )
            {
                price = jdouble(jitem(item,1),0), ask = jdouble(jitem(item,2),0), bid = jdouble(jitem(item,3),0);
                close = jdouble(jitem(item,4),0), vol = jdouble(jitem(item,5),0);
                fprintf(stderr,"%d.[%d %f %f %f %f %f].%d ",i,datenum,price,ask,bid,close,vol,n);
                utc32[numdates - 1 - i] = OS_conv_datenum(datenum,12,0,0), qdaily[numdates - 1 - i] = price * .001;
            }
        }
        prices777_genspline(&BUNDLE.splines[MAX_CURRENCIES+1],MAX_CURRENCIES+1,"quandl",utc32,qdaily,n<MAX_SPLINES?n:MAX_SPLINES,qdaily);
    }
    btcdhist = url_json(url);
    //{"date":1406160000,"high":0.01,"low":0.00125,"open":0.01,"close":0.001375,"volume":1.50179994,"quoteVolume":903.58818412,"weightedAverage":0.00166204},
    if ( (array= jarray(&n,btcdhist,0)) != 0 )
    {
        memset(utc32,0,sizeof(utc32)), memset(btcddaily,0,sizeof(btcddaily));
        //printf("GOT.(%s)\n",cJSON_Print(array));
        for (i=0; i<n; i++)
        {
            item = jitem(array,i);
            timestamp = juint(item,"date"), high = jdouble(item,"high"), low = jdouble(item,"low"), open = jdouble(item,"open");
            close = jdouble(item,"close"), vol = jdouble(item,"volume"), quotevol = jdouble(item,"quoteVolume"), price = jdouble(item,"weightedAverage");
            //printf("[%u %f %f %f %f %f %f %f]",timestamp,high,low,open,close,vol,quotevol,price);
            printf("[%u %d %f]",timestamp,OS_conv_unixtime(&seconds,timestamp),price);
            utc32[i] = timestamp - 12*3600, btcddaily[i] = price * 100.;
        }
        printf("poloniex.%d\n",n);
        prices777_genspline(&BUNDLE.splines[MAX_CURRENCIES+2],MAX_CURRENCIES+2,"btcdhist",utc32,btcddaily,n<MAX_SPLINES?n:MAX_SPLINES,btcddaily);
    }
    // https://poloniex.com/public?command=returnChartData&currencyPair=BTC_BTCD&start=1405699200&end=9999999999&period=86400
}

int32_t prices777_calcmatrix(double matrix[32][32])
{
    int32_t basenum,relnum,nonz,vnum,iter,numbase,numerrs = 0; double sum,vsum,price,price2,basevals[32],errsum=0;
    memset(basevals,0,sizeof(basevals));
    for (iter=0; iter<2; iter++)
    {
        numbase = 32;
        for (basenum=0; basenum<numbase; basenum++)
        {
            for (vsum=sum=vnum=nonz=relnum=0; relnum<numbase; relnum++)
            {
                if ( basenum != relnum )
                {
                    if ( (price= matrix[basenum][relnum]) != 0. )
                    {
                        price /= (MINDENOMS[relnum] * .001);
                        price *= (MINDENOMS[basenum] * .001);
                        if ( iter == 0 )
                            sum += (price), nonz++;//, printf("%.8f ",price);
                        else sum += fabs((price) - (basevals[basenum] / basevals[relnum])), nonz++;
                    }
                    if ( (price2= matrix[relnum][basenum]) != 0. )
                    {
                        price2 *= (MINDENOMS[relnum] * .001);
                        price2 /= (MINDENOMS[basenum] * .001);
                        if ( iter == 0 )
                            vsum += (price2), vnum++;
                        else vsum += fabs(price2 - (basevals[relnum] / basevals[basenum])), vnum++;
                    }
                    //if ( iter == 0 && 1/price2 > price )
                    //    printf("base.%d rel.%d price2 %f vs %f\n",basenum,relnum,1/price2,price);
                }
            }
            if ( iter == 0 )
                sum += 1., vsum += 1.;
            if ( nonz != 0 )
                sum /= nonz;
            if ( vnum != 0 )
                vsum /= vnum;
            if ( iter == 0 )
                basevals[basenum] = (sum + 1./vsum) / 2.;
            else errsum += (sum + vsum)/2, numerrs++;//, printf("(%.8f %.8f) ",sum,vsum);
            //printf("date.%d (%.8f/%d %.8f/%d).%02d -> %.8f\n",i,sum,nonz,vsum,vnum,basenum,basevals[basenum]);
        }
        if ( iter == 0 )
        {
            for (sum=relnum=0; relnum<numbase; relnum++)
                sum += (basevals[relnum]);//, printf("%.8f ",(basevals[relnum]));
            //printf("date.%d sums %.8f and vsums iter.%d\n",i,sum/7,iter);
            sum /= (numbase - 1);
            for (relnum=0; relnum<numbase; relnum++)
                basevals[relnum] /= sum;//, printf("%.8f ",basevals[relnum]);
            //printf("date.%d sums %.8f and vsums iter.%d\n",i,sum,iter);
        }
        else
        {
            for (basenum=0; basenum<numbase; basenum++)
                matrix[basenum][basenum] = basevals[basenum];
        }
    }
    if ( numerrs != 0 )
        errsum /= numerrs;
    return(errsum);
}

int32_t ecb_matrix(double matrix[32][32],char *date)
{
    FILE *fp=0; int32_t n=0,datenum,year,seconds,month,day,loaded = 0; char fname[64],_date[64];
    if ( date == 0 )
        date = _date, memset(_date,0,sizeof(_date));
    sprintf(fname,"ECB/%s",date), os_compatible_path(fname);
    if ( date[0] != 0 && (fp= fopen(fname,"rb")) != 0 )
    {
        if ( fread(matrix,1,sizeof(matrix[0][0])*32*32,fp) == sizeof(matrix[0][0])*32*32 )
            loaded = 1;
        else printf("fread error\n");
        fclose(fp);
    } else printf("ecb_matrix.(%s) load error fp.%p\n",fname,fp);
    if ( loaded == 0 )
    {
        datenum = conv_date(&seconds,date);
        year = datenum / 10000, month = (datenum / 100) % 100, day = (datenum % 100);
        if ( (n= prices777_ecb(date,&matrix[0][0],year,month,day)) > 0 )
        {
            sprintf(fname,"ECB/%s",date), os_compatible_path(fname);
            if ( (fp= fopen(fname,"wb")) != 0 )
            {
                if ( fwrite(matrix,1,sizeof(matrix[0][0])*32*32,fp) == sizeof(matrix[0][0])*32*32 )
                    loaded = 1;
                fclose(fp);
            }
        } else printf("peggy_matrix error loading %d.%d.%d\n",year,month,day);
    }
    if ( loaded == 0 && n == 0 )
    {
        printf("peggy_matrix couldnt process loaded.%d n.%d\n",loaded,n);
        return(-1);
    }
    //"2000-01-03"
    if ( (datenum= conv_date(&seconds,date)) < 0 )
        return(-1);
    printf("loaded.(%s) nonz.%d (%d %d %d) datenum.%d\n",date,n,year,month,day,datenum);
    return(datenum);
}

void price777_update(double *btcusdp,double *btcdbtcp)
{
    int32_t i,n,seconds,datenum; uint32_t timestamp; char url[1024],*dstr,*str;
    double btcddaily=0.,btcusd=0.,ask,high,low,bid,close,vol,quotevol,open,price = 0.;
    //cJSON *btcdtrades,*btcdtrades2,*,*bitcoincharts,;
    cJSON *quandl,*btcdhist,*array,*item,*bitcoinave,*blockchaininfo,*coindesk=0;
    //btcdtrades = url_json("https://poloniex.com/public?command=returnTradeHistory&currencyPair=BTC_BTCD");
    //btcdtrades2 = url_json("https://bittrex.com/api/v1.1/public/getmarkethistory?market=BTC-BTCD&count=50");
    bitcoinave = url_json("https://api.bitcoinaverage.com/ticker/USD/");
    //bitcoincharts = url_json("http://api.bitcoincharts.com/v1/weighted_prices.json");
    blockchaininfo = url_json("https://blockchain.info/ticker");
    coindesk = url_json("http://api.coindesk.com/v1/bpi/historical/close.json");
    sprintf(url,"https://poloniex.com/public?command=returnChartData&currencyPair=BTC_BTCD&start=%ld&end=9999999999&period=86400",time(NULL)-2*3600*24);
    quandl = url_json("https://www.quandl.com/api/v1/datasets/BAVERAGE/USD.json?rows=1");
    if ( quandl != 0 && (str= jstr(quandl,"updated_at")) != 0 && (datenum= conv_date(&seconds,str)) > 0 && (array= jarray(&n,quandl,"data")) != 0 )
    {
        //printf("datenum.%d data.%d %d\n",datenum,n,cJSON_GetArraySize(array));
        for (i=0; i<1; i++)
        {
            // ["Date","24h Average","Ask","Bid","Last","Total Volume"]
            // ["2015-07-25",289.27,288.84,288.68,288.87,44978.61]
            item = jitem(array,i);
            if ( (dstr= jstr(jitem(item,0),0)) != 0 && (datenum= conv_date(&seconds,dstr)) > 0 )
            {
                btcusd = price = jdouble(jitem(item,1),0), ask = jdouble(jitem(item,2),0), bid = jdouble(jitem(item,3),0);
                close = jdouble(jitem(item,4),0), vol = jdouble(jitem(item,5),0);
                //fprintf(stderr,"%d.[%d %f %f %f %f %f].%d ",i,datenum,price,ask,bid,close,vol,n);
            }
        }
    }
    price = 0.;
    for (i=n=0; i<BUNDLE.num; i++)
    {
        if ( strcmp(BUNDLE.ptrs[i]->lbase,"btcd") == 0 && strcmp(BUNDLE.ptrs[i]->lrel,"btc") == 0 && BUNDLE.ptrs[i]->lastprice != 0. )
        {
            price += BUNDLE.ptrs[i]->lastprice;
            n++;
        }
    }
    if ( n != 0 )
    {
        price /= n;
        *btcdbtcp = price;
        printf("set BTCD price %f\n",price);
        BUNDLE.btcdbtc = price;
    }
    else
    {
        btcdhist = url_json(url);
        //{"date":1406160000,"high":0.01,"low":0.00125,"open":0.01,"close":0.001375,"volume":1.50179994,"quoteVolume":903.58818412,"weightedAverage":0.00166204},
        if ( btcdhist != 0 && (array= jarray(&n,btcdhist,0)) != 0 )
        {
            //printf("GOT.(%s)\n",cJSON_Print(array));
            for (i=0; i<1; i++)
            {
                item = jitem(array,i);
                timestamp = juint(item,"date"), high = jdouble(item,"high"), low = jdouble(item,"low"), open = jdouble(item,"open");
                close = jdouble(item,"close"), vol = jdouble(item,"volume"), quotevol = jdouble(item,"quoteVolume"), price = jdouble(item,"weightedAverage");
                //printf("[%u %f %f %f %f %f %f %f]",timestamp,high,low,open,close,vol,quotevol,price);
                //printf("[%u %d %f]",timestamp,OS_conv_unixtime(&seconds,timestamp),price);
                btcddaily = price;
                if ( btcddaily != 0 )
                    BUNDLE.btcdbtc = *btcdbtcp = btcddaily;
            }
            //printf("poloniex.%d\n",n);
        }
        if ( btcdhist != 0 )
            free_json(btcdhist);
    }
    // https://blockchain.info/ticker
    /*
     {
     "USD" : {"15m" : 288.22, "last" : 288.22, "buy" : 288.54, "sell" : 288.57,  "symbol" : "$"},
     "ISK" : {"15m" : 38765.88, "last" : 38765.88, "buy" : 38808.92, "sell" : 38812.95,  "symbol" : "kr"},
     "HKD" : {"15m" : 2234, "last" : 2234, "buy" : 2236.48, "sell" : 2236.71,  "symbol" : "$"},
     "TWD" : {"15m" : 9034.19, "last" : 9034.19, "buy" : 9044.22, "sell" : 9045.16,  "symbol" : "NT$"},
     "CHF" : {"15m" : 276.39, "last" : 276.39, "buy" : 276.69, "sell" : 276.72,  "symbol" : "CHF"},
     "EUR" : {"15m" : 262.46, "last" : 262.46, "buy" : 262.75, "sell" : 262.78,  "symbol" : "€"},
     "DKK" : {"15m" : 1958.92, "last" : 1958.92, "buy" : 1961.1, "sell" : 1961.3,  "symbol" : "kr"},
     "CLP" : {"15m" : 189160.6, "last" : 189160.6, "buy" : 189370.62, "sell" : 189390.31,  "symbol" : "$"},
     "CAD" : {"15m" : 375.45, "last" : 375.45, "buy" : 375.87, "sell" : 375.91,  "symbol" : "$"},
     "CNY" : {"15m" : 1783.67, "last" : 1783.67, "buy" : 1785.65, "sell" : 1785.83,  "symbol" : "¥"},
     "THB" : {"15m" : 10046.98, "last" : 10046.98, "buy" : 10058.14, "sell" : 10059.18,  "symbol" : "฿"},
     "AUD" : {"15m" : 394.77, "last" : 394.77, "buy" : 395.2, "sell" : 395.25,  "symbol" : "$"},
     "SGD" : {"15m" : 395.08, "last" : 395.08, "buy" : 395.52, "sell" : 395.56,  "symbol" : "$"},
     "KRW" : {"15m" : 335991.51, "last" : 335991.51, "buy" : 336364.55, "sell" : 336399.52,  "symbol" : "₩"},
     "JPY" : {"15m" : 35711.99, "last" : 35711.99, "buy" : 35751.64, "sell" : 35755.35,  "symbol" : "¥"},
     "PLN" : {"15m" : 1082.74, "last" : 1082.74, "buy" : 1083.94, "sell" : 1084.06,  "symbol" : "zł"},
     "GBP" : {"15m" : 185.84, "last" : 185.84, "buy" : 186.04, "sell" : 186.06,  "symbol" : "£"},
     "SEK" : {"15m" : 2471.02, "last" : 2471.02, "buy" : 2473.76, "sell" : 2474.02,  "symbol" : "kr"},
     "NZD" : {"15m" : 436.89, "last" : 436.89, "buy" : 437.37, "sell" : 437.42,  "symbol" : "$"},
     "BRL" : {"15m" : 944.91, "last" : 944.91, "buy" : 945.95, "sell" : 946.05,  "symbol" : "R$"},
     "RUB" : {"15m" : 16695.05, "last" : 16695.05, "buy" : 16713.58, "sell" : 16715.32,  "symbol" : "RUB"}
     }*/
     /*{
        "24h_avg": 281.22,
        "ask": 280.12,
        "bid": 279.33,
        "last": 279.58,
        "timestamp": "Sun, 02 Aug 2015 09:36:34 -0000",
        "total_vol": 39625.8
    }*/
  
    if ( bitcoinave != 0 )
    {
        if ( (price= jdouble(bitcoinave,"24h_avg")) > SMALLVAL )
        {
            //printf("bitcoinave %f %f\n",btcusd,price);
            dxblend(&btcusd,price,0.5);
        }
        free_json(bitcoinave);
    }
    if ( quandl != 0 )
        free_json(quandl);
    if ( coindesk != 0 )
        free_json(coindesk);
    if ( blockchaininfo != 0 )
    {
        if ( (item= jobj(blockchaininfo,"USD")) != 0 && item != 0 && (price= jdouble(item,"15m")) > SMALLVAL )
        {
            //printf("blockchaininfo %f %f\n",btcusd,price);
            dxblend(&btcusd,price,0.5);
        }
        free_json(blockchaininfo);
    }
    if ( btcusd != 0 )
        BUNDLE.btcusd = *btcusdp = btcusd;
    
    
    // https://poloniex.com/public?command=returnChartData&currencyPair=BTC_BTCD&start=1405699200&end=9999999999&period=86400

    // https://poloniex.com/public?command=returnTradeHistory&currencyPair=BTC_BTCD
    //https://bittrex.com/api/v1.1/public/getmarkethistory?market=BTC-BTCD&count=50
    /*{"success":true,"message":"","result":[{"Id":8551089,"TimeStamp":"2015-07-25T16:00:41.597","Quantity":59.60917089,"Price":0.00642371,"Total":0.38291202,"FillType":"FILL","OrderType":"BUY"},{"Id":8551088,"TimeStamp":"2015-07-25T16:00:41.597","Quantity":7.00000000,"Price":0.00639680,"Total":0.04477760,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8551087,"TimeStamp":"2015-07-25T16:00:41.597","Quantity":6.51000000,"Price":0.00639679,"Total":0.04164310,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8551086,"TimeStamp":"2015-07-25T16:00:41.597","Quantity":6.00000000,"Price":0.00633300,"Total":0.03799800,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8551085,"TimeStamp":"2015-07-25T16:00:41.593","Quantity":4.76833955,"Price":0.00623300,"Total":0.02972106,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8551084,"TimeStamp":"2015-07-25T16:00:41.593","Quantity":5.00000000,"Price":0.00620860,"Total":0.03104300,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8551083,"TimeStamp":"2015-07-25T16:00:41.593","Quantity":4.91803279,"Price":0.00620134,"Total":0.03049839,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8551082,"TimeStamp":"2015-07-25T16:00:41.593","Quantity":4.45166432,"Price":0.00619316,"Total":0.02756986,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8551081,"TimeStamp":"2015-07-25T16:00:41.59","Quantity":2.00000000,"Price":0.00619315,"Total":0.01238630,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8547525,"TimeStamp":"2015-07-25T06:20:43.69","Quantity":1.23166045,"Price":0.00623300,"Total":0.00767693,"FillType":"FILL","OrderType":"BUY"},{"Id":8547524,"TimeStamp":"2015-07-25T06:20:43.69","Quantity":5.00000000,"Price":0.00613300,"Total":0.03066500,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8547523,"TimeStamp":"2015-07-25T06:20:43.687","Quantity":10.00000000,"Price":0.00609990,"Total":0.06099900,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8547522,"TimeStamp":"2015-07-25T06:20:43.687","Quantity":0.12326502,"Price":0.00609989,"Total":0.00075190,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8547521,"TimeStamp":"2015-07-25T06:20:43.687","Quantity":3.29000000,"Price":0.00609989,"Total":0.02006863,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8547520,"TimeStamp":"2015-07-25T06:20:43.687","Quantity":5.00000000,"Price":0.00604400,"Total":0.03022000,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8547519,"TimeStamp":"2015-07-25T06:20:43.683","Quantity":12.80164947,"Price":0.00603915,"Total":0.07731108,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8547518,"TimeStamp":"2015-07-25T06:20:43.683","Quantity":10.00000000,"Price":0.00602715,"Total":0.06027150,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8547517,"TimeStamp":"2015-07-25T06:20:43.683","Quantity":4.29037397,"Price":0.00600000,"Total":0.02574224,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8547516,"TimeStamp":"2015-07-25T06:20:43.683","Quantity":77.55994092,"Price":0.00598921,"Total":0.46452277,"FillType":"PARTIAL_FILL","OrderType":"BUY"},{"Id":8547515,"TimeStamp":"2015-07-25T06:20:43.68","Quantity":0.08645064,"Price":0.00598492,"Total":0.00051740,"FillType":"PARTIAL_FILL","OrderType":"BUY"}]}
     */
    
    // https://api.bitcoinaverage.com/ticker/global/all
    /* {
     "AED": {
     "ask": 1063.28,
     "bid": 1062.1,
     "last": 1062.29,
     "timestamp": "Sat, 25 Jul 2015 17:13:14 -0000",
     "volume_btc": 0.0,
     "volume_percent": 0.0
     },*/
    
    // http://api.bitcoincharts.com/v1/weighted_prices.json
    // {"USD": {"7d": "279.79", "30d": "276.05", "24h": "288.55"}, "IDR": {"7d": "3750799.88", "30d": "3636926.02", "24h": "3860769.92"}, "ILS": {"7d": "1033.34", "30d": "1031.58", "24h": "1092.36"}, "GBP": {"7d": "179.51", "30d": "175.30", "24h": "185.74"}, "DKK": {"30d": "1758.61"}, "CAD": {"7d": "364.04", "30d": "351.27", "24h": "376.12"}, "MXN": {"30d": "4369.33"}, "XRP": {"7d": "35491.70", "30d": "29257.39", "24h": "36979.02"}, "SEK": {"7d": "2484.50", "30d": "2270.94"}, "SGD": {"7d": "381.93", "30d": "373.69", "24h": "393.94"}, "HKD": {"7d": "2167.99", "30d": "2115.77", "24h": "2232.12"}, "AUD": {"7d": "379.42", "30d": "365.85", "24h": "394.93"}, "CHF": {"30d": "250.61"}, "timestamp": 1437844509, "CNY": {"7d": "1724.99", "30d": "1702.32", "24h": "1779.48"}, "LTC": {"7d": "67.46", "30d": "51.97", "24h": "61.61"}, "NZD": {"7d": "425.01", "30d": "409.33", "24h": "437.86"}, "THB": {"30d": "8632.82"}, "EUR": {"7d": "257.32", "30d": "249.88", "24h": "263.42"}, "ARS": {"30d": "3271.98"}, "NOK": {"30d": "2227.54"}, "RUB": {"7d": "16032.32", "30d": "15600.38", "24h": "16443.39"}, "INR": {"30d": "16601.17"}, "JPY": {"7d": "34685.73", "30d": "33617.77", "24h": "35652.79"}, "CZK": {"30d": "6442.13"}, "BRL": {"7d": "946.76", "30d": "900.77", "24h": "964.09"}, "NMC": {"7d": "454.06", "30d": "370.39", "24h": "436.71"}, "PLN": {"7d": "1041.81", "30d": "1024.96", "24h": "1072.49"}, "ZAR": {"30d": "3805.55"}}
    

}

double blend_price(double *volp,double wtA,cJSON *jsonA,double wtB,cJSON *jsonB)
{
    //A.{"ticker":{"base":"BTS","target":"CNY","price":"0.02958291","volume":"3128008.39295500","change":"0.00019513","markets":[{"market":"BTC38","price":"0.02960000","volume":3051650.682955},{"market":"Bter","price":"0.02890000","volume":76357.71}]},"timestamp":1438490881,"success":true,"error":""}
    // B.{"id":"bts\/cny","price":"0.02940000","price_before_24h":"0.02990000","volume_first":"3048457.6857147217","volume_second":"90629.45859575272","volume_btc":"52.74","best_market":"btc38","latest_trade":"2015-08-02 03:57:38","coin1":"BitShares","coin2":"CNY","markets":[{"market":"btc38","price":"0.02940000","volume":"3048457.6857147217","volume_btc":"52.738317962865"},{"market":"bter","price":"0.04350000","volume":"0","volume_btc":"0"}]}
    double priceA,priceB,priceB24,price,volA,volB; cJSON *obj;
    priceA = priceB = priceB24= price = volA = volB = 0.;
    if ( jsonA != 0 && (obj= jobj(jsonA,"ticker")) != 0 )
    {
        priceA = jdouble(obj,"price");
        volA = jdouble(obj,"volume");
    }
    if ( jsonB != 0 )
    {
        priceB = jdouble(jsonB,"price");
        priceB24 = jdouble(jsonB,"price_before_24h");
        volB = jdouble(jsonB,"volume_first");
    }
    //printf("priceA %f volA %f, priceB %f %f volB %f\n",priceA,volA,priceB,priceB24,volB);
    if ( priceB > SMALLVAL && priceB24 > SMALLVAL )
        priceB = (priceB * .1) + (priceB24 * .9);
    else if ( priceB < SMALLVAL )
        priceB = priceB24;
    if ( priceA*volA < SMALLVAL )
        price = priceB;
    else if ( priceB*volB < SMALLVAL )
        price = priceA;
    else price = (wtA * priceA) + (wtB * priceB);
    *volp = (volA + volB);
    return(price);
}

void _crypto_update(double cryptovols[2][8][2],struct prices777_data *dp,int32_t selector)
{
    char *cryptonatorA = "https://www.cryptonator.com/api/full/%s-%s"; //unity-btc
    char *cryptocoinchartsB = "http://api.cryptocoincharts.info/tradingPair/%s_%s"; //bts_btc
    char *cryptostrs[9] = { "btc", "nxt", "unity", "eth", "ltc", "xmr", "bts", "xcp", "etc" };
    int32_t iter,i,j; double btcusd,btcdbtc,cnyusd,prices[8][2],volumes[8][2];
    char base[16],rel[16],url[512],*str; cJSON *jsonA,*jsonB;
    cnyusd = BUNDLE.cnyusd;
    btcusd = BUNDLE.btcusd;
    btcdbtc = BUNDLE.btcdbtc;
    //printf("update with btcusd %f btcd %f cnyusd %f cnybtc %f\n",btcusd,btcdbtc,cnyusd,cnyusd/btcusd);
    if ( btcusd < SMALLVAL || btcdbtc < SMALLVAL )
    {
        price777_update(&btcusd,&btcdbtc);
        printf("price777_update with btcusd %f btcd %f\n",btcusd,btcdbtc);
    }
    memset(prices,0,sizeof(prices));
    memset(volumes,0,sizeof(volumes));
    for (j=0; j<sizeof(cryptostrs)/sizeof(*cryptostrs); j++)
    {
        str = cryptostrs[j];
        if ( strcmp(str,"etc") == 0 )
        {
            if ( prices[3][0] > SMALLVAL )
                break;
            i = 3;
        } else i = j;
        for (iter=0; iter<1; iter++)
        {
            if ( i == 0 && iter == 0 )
                strcpy(base,"btcd"), strcpy(rel,"btc");
            else strcpy(base,str), strcpy(rel,iter==0?"btc":"cny");
            //if ( selector == 0 )
            {
                sprintf(url,cryptonatorA,base,rel);
                jsonA = url_json(url);
            }
            //else
            {
                sprintf(url,cryptocoinchartsB,base,rel);
                jsonB = url_json(url);
            }
            prices[i][iter] = blend_price(&volumes[i][iter],0.4,jsonA,0.6,jsonB);
            if ( iter == 1 )
            {
                if ( btcusd > SMALLVAL )
                {
                    prices[i][iter] *= cnyusd / btcusd;
                    volumes[i][iter] *= cnyusd / btcusd;
                } else prices[i][iter] = volumes[i][iter] = 0.;
            }
            cryptovols[0][i][iter] = _pairaved(cryptovols[0][i][iter],prices[i][iter]);
            cryptovols[1][i][iter] = _pairaved(cryptovols[1][i][iter],volumes[i][iter]);
            if ( Debuglevel > 2 )
                printf("(%f %f).%d:%d ",cryptovols[0][i][iter],cryptovols[1][i][iter],i,iter);
            //if ( cnyusd < SMALLVAL || btcusd < SMALLVAL )
            //    break;
        }
    }
}

void crypto_update0()
{
   // _crypto_update(BUNDLE.cryptovols,&BUNDLE.data,0);
    while ( 1 )
    {
   //     _crypto_update(BUNDLE.cryptovols,&BUNDLE.data,0);
        sleep(1000);
    }
}

void crypto_update1()
{
    _crypto_update(BUNDLE.cryptovols,&BUNDLE.data,1);
    while ( 1 )
    {
        _crypto_update(BUNDLE.cryptovols,&BUNDLE.data,1);
        sleep(1000);
    }
}

char *Yahoo_metals[] = { "XAU", "XAG", "XPT", "XPD" };
void prices777_RTupdate(double cryptovols[2][8][2],double RTmetals[4],double *RTprices,struct prices777_data *dp)
{
    char *cryptostrs[8] = { "btc", "nxt", "unity", "eth", "ltc", "xmr", "bts", "xcp" };
    int32_t iter,i,c,baserel,basenum,relnum; double cnyusd,btcusd,btcdbtc,bid,ask,price,vol,prices[8][2],volumes[8][2];
    char base[16],rel[16];
    price777_update(&btcusd,&btcdbtc);
    memset(prices,0,sizeof(prices));
    memset(volumes,0,sizeof(volumes));
    for (i=0; i<sizeof(cryptostrs)/sizeof(*cryptostrs); i++)
        for (iter=0; iter<2; iter++)
            prices[i][iter] = cryptovols[0][i][iter], volumes[i][iter] = cryptovols[1][i][iter];
    if ( prices[0][0] > SMALLVAL )
        dxblend(&btcdbtc,prices[0][0],.9);
    dxblend(&dp->btcdbtc,btcdbtc,.995);
    if ( BUNDLE.btcdbtc < SMALLVAL )
        BUNDLE.btcdbtc = dp->btcdbtc;
    if ( (cnyusd= BUNDLE.cnyusd) > SMALLVAL )
    {
        if ( prices[0][1] > SMALLVAL )
        {
            //printf("cnyusd %f, btccny %f -> btcusd %f %f\n",cnyusd,prices[0][1],prices[0][1]*cnyusd,btcusd);
            btcusd = prices[0][1] * cnyusd;
            if ( dp->btcusd < SMALLVAL )
                dp->btcusd = btcusd;
            else dxblend(&dp->btcusd,btcusd,.995);
            if ( BUNDLE.btcusd < SMALLVAL )
                BUNDLE.btcusd = dp->btcusd;
            if ( BUNDLE.data.btcusd < SMALLVAL )
                BUNDLE.data.btcusd = dp->btcusd;
            printf("cnyusd %f, btccny %f -> btcusd %f %f -> %f %f %f\n",cnyusd,prices[0][1],prices[0][1]*cnyusd,btcusd,dp->btcusd,BUNDLE.btcusd,BUNDLE.data.btcusd);
        }
    }
    for (i=1; i<sizeof(cryptostrs)/sizeof(*cryptostrs); i++)
    {
        if ( (vol= volumes[i][0]+volumes[i][1]) > SMALLVAL )
        {
            price = ((prices[i][0] * volumes[i][0]) + (prices[i][1] * volumes[i][1])) / vol;
            if ( Debuglevel > 2 )
                printf("%s %f v%f + %f v%f -> %f %f\n",cryptostrs[i],prices[i][0],volumes[i][0],prices[i][1],volumes[i][1],price,dp->cryptos[i]);
            dxblend(&dp->cryptos[i],price,.995);
        }
    }
    btcusd = BUNDLE.btcusd;
    btcdbtc = BUNDLE.btcdbtc;
    if ( Debuglevel > 2 )
        printf("    update with btcusd %f btcd %f\n",btcusd,btcdbtc);
    if ( btcusd < SMALLVAL || btcdbtc < SMALLVAL )
    {
        price777_update(&btcusd,&btcdbtc);
        if ( Debuglevel > 2 )
            printf("     price777_update with btcusd %f btcd %f\n",btcusd,btcdbtc);
    } else BUNDLE.btcusd = btcusd, BUNDLE.btcdbtc = btcdbtc;
    for (c=0; c<sizeof(CONTRACTS)/sizeof(*CONTRACTS); c++)
    {
        for (iter=0; iter<3; iter++)
        {
            switch ( iter )
            {
                case 0: bid = dp->tbids[c], ask = dp->tasks[c]; break;
                case 1: bid = dp->fbids[c], ask = dp->fasks[c]; break;
                case 2: bid = dp->ibids[c], ask = dp->iasks[c]; break;
            }
             if ( (price= _pairaved(bid,ask)) > SMALLVAL )
            {
                printf("%.6f ",price);
                dxblend(&RTprices[c],price,.995);
                if ( 0 && (baserel= prices777_ispair(base,rel,CONTRACTS[c])) >= 0 )
                {
                    basenum = (baserel >> 8) & 0xff, relnum = baserel & 0xff;
                    if ( basenum < 32 && relnum < 32 )
                    {
                        //printf("%s.%d %f <- %f\n",CONTRACTS[c],c,RTmatrix[basenum][relnum],RTprices[c]);
                        //dxblend(&RTmatrix[basenum][relnum],RTprices[c],.999);
                    }
                }
                if ( strcmp(CONTRACTS[c],"XAUUSD") == 0 )
                    dxblend(&RTmetals[0],price,.995);
            }
        }
    }
    for (i=0; i<sizeof(Yahoo_metals)/sizeof(*Yahoo_metals); i++)
        if ( BUNDLE.data.metals[i] != 0 )
            dxblend(&RTmetals[i],BUNDLE.data.metals[i],.995);
}

int32_t prices777_getmatrix(double *basevals,double *btcusdp,double *btcdbtcp,double Hmatrix[32][32],double *RTprices,char *contracts[],int32_t num,uint32_t timestamp)
{
    int32_t i,j,c; char name[16]; double btcusd,btcdbtc;
    memcpy(Hmatrix,BUNDLE.data.ecbmatrix,sizeof(BUNDLE.data.ecbmatrix));
    prices777_calcmatrix(Hmatrix);
    /*for (i=0; i<32; i++)
    {
        for (j=0; j<32; j++)
            printf("%.6f ",Hmatrix[i][j]);
        printf("%s\n",CURRENCIES[i]);
    }*/
    btcusd = BUNDLE.btcusd;
    btcdbtc = BUNDLE.btcdbtc;
    if ( btcusd > SMALLVAL )
        dxblend(btcusdp,btcusd,.9);
    if ( btcdbtc > SMALLVAL )
        dxblend(btcdbtcp,btcdbtc,.9);
    // char *cryptostrs[8] = { "btc", "nxt", "unity", "eth", "ltc", "xmr", "bts", "xcp" };
    // "BTCUSD", "NXTBTC", "SuperNET", "ETHBTC", "LTCBTC", "XMRBTC", "BTSBTC", "XCPBTC",  // BTC priced
    for (i=0; i<num; i++)
    {
        if ( contracts[i] == 0 )
            continue;
        if ( i == num-1 && strcmp(contracts[i],"BTCUSD") == 0 )
        {
            RTprices[i] = *btcusdp;
            continue;
        }
        else if ( i == num-2 && strcmp(contracts[i],"BTCCNY") == 0 )
        {
            continue;
        }
        else if ( i == num-3 && strcmp(contracts[i],"BTCRUB") == 0 )
        {
            continue;
        }
        else if ( i == num-4 && strcmp(contracts[i],"XAUUSD") == 0 )
        {
            continue;
        }
        if ( strcmp(contracts[i],"NXTBTC") == 0 )
            RTprices[i] = BUNDLE.data.cryptos[1];
        else if ( strcmp(contracts[i],"SuperNET") == 0 )
            RTprices[i] = BUNDLE.data.cryptos[2];
        else if ( strcmp(contracts[i],"ETHBTC") == 0 )
            RTprices[i] = BUNDLE.data.cryptos[3];
        else if ( strcmp(contracts[i],"LTCBTC") == 0 )
            RTprices[i] = BUNDLE.data.cryptos[4];
        else if ( strcmp(contracts[i],"XMRBTC") == 0 )
            RTprices[i] = BUNDLE.data.cryptos[5];
        else if ( strcmp(contracts[i],"BTSBTC") == 0 )
            RTprices[i] = BUNDLE.data.cryptos[6];
        else if ( strcmp(contracts[i],"XCPBTC") == 0 )
            RTprices[i] = BUNDLE.data.cryptos[7];
        else if ( i < 32 )
        {
            basevals[i] = Hmatrix[i][i];
            printf("(%s %f).%d ",CURRENCIES[i],basevals[i],i);
        }
        else if ( (c= prices777_contractnum(contracts[i],0)) >= 0 )
        {
            RTprices[i] = BUNDLE.data.RTprices[c];
            //if ( is_decimalstr(contracts[i]+strlen(contracts[i])-2) != 0 )
            //    cprices[i] *= .0001;
        }
        else
        {
            for (j=0; j<sizeof(Yahoo_metals)/sizeof(*Yahoo_metals); j++)
            {
                sprintf(name,"%sUSD",Yahoo_metals[j]);
                if ( contracts[i] != 0 && strcmp(name,contracts[i]) == 0 )
                {
                    RTprices[i] = BUNDLE.data.RTmetals[j];
                    break;
                }
            }
        }
        if ( Debuglevel > 2 )
            printf("(%f %f) i.%d num.%d %s %f\n",*btcusdp,*btcdbtcp,i,num,contracts[i],RTprices[i]);
        //printf("RT.(%s %f) ",contracts[i],RTprices[i]);
    }
    return(BUNDLE.data.ecbdatenum);
}

int32_t prices_idle(struct plugin_info *plugin)
{
    static double lastupdate,lastdayupdate; static int32_t didinit; static portable_mutex_t mutex;
    int32_t i,datenum; struct prices777_data *dp = &BUNDLE.tmp;
    *dp = BUNDLE.data;
    if ( didinit == 0 )
    {
        portable_mutex_init(&mutex);
        prices777_init(BUNDLE.jsonstr);
    }
    if ( milliseconds() > lastupdate + 60000 )
    {
        if ( milliseconds() > lastdayupdate + 60000*60 )
        {
            if ( (datenum= ecb_matrix(dp->ecbmatrix,dp->edate)) > 0 )
            {
                dp->ecbdatenum = datenum;
                dp->ecbyear = dp->ecbdatenum / 10000,  dp->ecbmonth = (dp->ecbdatenum / 100) % 100,  dp->ecbday = (dp->ecbdatenum % 100);
                expand_datenum(dp->edate,datenum);
                memcpy(dp->RTmatrix,dp->ecbmatrix,sizeof(dp->RTmatrix));
            }
            lastdayupdate = milliseconds();
        }
        for (i=0; i<sizeof(Yahoo_metals)/sizeof(*Yahoo_metals); i++)
            BUNDLE.data.metals[i] = prices777_yahoo(Yahoo_metals[i]);
        BUNDLE.truefxidnum = prices777_truefx(dp->tmillistamps,dp->tbids,dp->tasks,dp->topens,dp->thighs,dp->tlows,BUNDLE.truefxuser,BUNDLE.truefxpass,(uint32_t)BUNDLE.truefxidnum);
        prices777_fxcm(dp->flhlogmatrix,dp->flogmatrix,dp->fbids,dp->fasks,dp->fhighs,dp->flows);
        prices777_instaforex(dp->ilogmatrix,dp->itimestamps,dp->ibids,dp->iasks);
        double btcdbtc,btcusd;
        price777_update(&btcusd,&btcdbtc);
        if ( btcusd > SMALLVAL )
            dxblend(&dp->btcusd,btcusd,0.99);
        if ( btcdbtc > SMALLVAL )
            dxblend(&dp->btcdbtc,btcdbtc,0.99);
        if ( BUNDLE.data.btcusd == 0 )
            BUNDLE.data.btcusd = dp->btcusd;
        if ( BUNDLE.data.btcdbtc == 0 )
            BUNDLE.data.btcdbtc = dp->btcdbtc;
        if ( dp->ecbmatrix[USD][USD] > SMALLVAL && dp->ecbmatrix[CNY][CNY] > SMALLVAL )
            BUNDLE.cnyusd = (dp->ecbmatrix[CNY][CNY] / dp->ecbmatrix[USD][USD]);
        lastupdate = milliseconds();
        portable_mutex_lock(&mutex);
        BUNDLE.data = *dp;
        portable_mutex_unlock(&mutex);
        //kv777_write(BUNDLE.kv,"data",5,&BUNDLE.data,sizeof(BUNDLE.data));
        prices777_RTupdate(BUNDLE.cryptovols,BUNDLE.data.RTmetals,BUNDLE.data.RTprices,&BUNDLE.data);
        //printf("update finished\n");
        void peggy();
        peggy();
        didinit = 1;
    }
    return(0);
}

double prices777_baseprice(uint32_t timestamp,int32_t basenum)
{
    double btc,btcd,btcdusd,usdval;
    btc = 1000. * _pairaved(prices777_splineval(&BUNDLE.splines[MAX_CURRENCIES+0],timestamp,0),prices777_splineval(&BUNDLE.splines[MAX_CURRENCIES+1],timestamp,0));
    btcd = .01 * prices777_splineval(&BUNDLE.splines[MAX_CURRENCIES+2],timestamp,0);
    if ( btc != 0. && btcd != 0. )
    {
        btcdusd = (btc * btcd);
        usdval = prices777_splineval(&BUNDLE.splines[USD],timestamp,0);
        if ( basenum == USD )
            return(1. / btcdusd);
        else return(prices777_splineval(&BUNDLE.splines[basenum],timestamp,0) / (btcdusd * usdval));
    }
    return(0.);
}

void prices777_sim(uint32_t now,int32_t numiters)
{
    double btca,btcb,btcd,btc,btcdusd,basevals[MAX_CURRENCIES],btcdprices[MAX_CURRENCIES+1];
    int32_t i,j,datenum,seconds; uint32_t timestamp,starttime = (uint32_t)time(NULL);
    for (i=0; i<numiters; i++)
    {
        timestamp = now - (rand() % (3600*24*64));
        btca = 1000. * prices777_splineval(&BUNDLE.splines[MAX_CURRENCIES+0],timestamp,0);
        btcb = 1000. * prices777_splineval(&BUNDLE.splines[MAX_CURRENCIES+1],timestamp,0);
        btc = _pairaved(btca,btcb);
        btcd = .01 * prices777_splineval(&BUNDLE.splines[MAX_CURRENCIES+2],timestamp,0);
        btcdusd = (btc * btcd);
        datenum = OS_conv_unixtime(&seconds,timestamp);
        for (j=0; j<MAX_CURRENCIES; j++)
        {
            basevals[j] = prices777_splineval(&BUNDLE.splines[j],timestamp,0);
            btcdprices[j] = basevals[j] / (btcdusd * basevals[USD]);
        }
        if ( (i % 100000) == 0 )
        {
            printf("%d:%02d:%02d %.8f %.8f -> USD %.8f (EURUSD %.8f %.8f) ",datenum,seconds/3600,(seconds%3600)/60,btc,btcd,btcdusd,btcdprices[EUR]/btcdprices[USD],basevals[EUR]/basevals[USD]);
            for (j=0; j<MAX_CURRENCIES; j++)
                printf("%.8f ",btcdprices[j]);
            printf("\n");
        }
    }
    printf("sim took %ld seconds\n",time(NULL) - starttime);
}

int32_t prices777_matrix(double pricevals[32][MAX_SPLINES],uint64_t *maskp,double matrix[][32][32],int32_t numdates)
{
    int32_t i,basenum,datenums[MAX_SPLINES],datenum=-1,year,month,day,ind,seconds,checkdate,numbase=32,firstdatenum=-1,numerrs = 0;
    uint32_t utc32[MAX_SPLINES]; char date[64]; double basevals[32],splinevals[32][MAX_SPLINES],refvals[32][MAX_SPLINES],errsum; FILE *fp;
    ensure_directory("ECB");
    if ( (fp= fopen("ECB/splines","rb")) != 0 )
    {
        fread(BUNDLE.splines,1,sizeof(BUNDLE.splines),fp);
        fclose(fp);
        return(OS_conv_unixtime(&seconds,BUNDLE.splines[0].utc32[0] + 3600*24*2));
    }
    strcpy(date,"2015-07-24"); *maskp = 0;
    memset(splinevals,0,sizeof(splinevals)), memset(utc32,0,sizeof(utc32)), memset(refvals,0,sizeof(refvals)), memset(datenums,0,sizeof(datenums));
    prices777_btcprices(0,MAX_SPLINES);
    if ( (datenum= ecb_matrix(matrix[numdates - 1],date)) > 0 )
    {
        (*maskp) |= (1LL << (numdates - 1));
        datenums[numdates - 1] = firstdatenum = datenum;
        for (i=1; i<numdates; i++)
        {
            datenum = ecb_decrdate(&year,&month,&day,date,datenum);
            //printf("decr -> %d (%s)\n",datenum,date);
            if ( (checkdate= ecb_matrix(matrix[numdates - 1 - i],date)) == datenum )
            {
                datenums[numdates - 1 - i] = datenum;
                (*maskp) |= (1LL << (numdates - 1 - i));
                //printf("loaded (%s) %llx\n",date,(long long)(1LL << (numdates - 1 - i)));
            } else printf("checkdate.%d != datenum.%d\n",checkdate,datenum);
        }
    } else return(-1);
    for (errsum=i=ind=0; i<numdates; i++)
    {
        if ( ((1LL << i) & (*maskp)) != 0 )
        {
            memset(basevals,0,sizeof(basevals));
            prices777_calcmatrix(matrix[i]);
            for (basenum=0; basenum<numbase; basenum++)
            {
                splinevals[basenum][ind] = matrix[i][basenum][basenum];
                refvals[basenum][i] = matrix[i][basenum][basenum];
            }
            utc32[ind++] = OS_conv_datenum(datenums[i],14 - is_DST(datenums[i]),0,0);
        }
    }
    printf("ind.%d errsum %.8f/%d -> aveerr %.8f\n",ind,errsum,numerrs,errsum/numerrs);
    for (basenum=0; basenum<numbase; basenum++)
        prices777_genspline(&BUNDLE.splines[basenum],basenum,CURRENCIES[basenum],utc32,splinevals[basenum],ind,refvals[basenum]);
    if ( 1 && (fp= fopen("ECB/splines","wb")) != 0 )
    {
        fwrite(BUNDLE.splines,1,sizeof(BUNDLE.splines),fp);
        fclose(fp);
        return(0);
    }
    prices777_sim(OS_conv_datenum(firstdatenum,14 - is_DST(firstdatenum),0,0),100000);
    return(firstdatenum);
}

void prices777_getlist(char *retbuf)
{
    int32_t i,j; struct prices777 *prices; char pair[16],*jsonstr; cJSON *json,*array,*item;
    json = cJSON_CreateObject();
    array = cJSON_CreateArray();
    for (i=0; i<sizeof(CURRENCIES)/sizeof(*CURRENCIES); i++)
        cJSON_AddItemToArray(array,cJSON_CreateString(CURRENCIES[i]));
    cJSON_AddItemToObject(json,"currency",array);
    array = cJSON_CreateArray();
    for (i=0; i<32; i++)
        for (j=0; j<32; j++)
        {
            if ( i != j )
            {
                sprintf(pair,"%s%s",CURRENCIES[i],CURRENCIES[j]);
                cJSON_AddItemToArray(array,cJSON_CreateString(pair));
            }
        }
    cJSON_AddItemToObject(json,"pairs",array);
    array = cJSON_CreateArray();
    for (i=0; i<sizeof(CONTRACTS)/sizeof(*CONTRACTS); i++)
        cJSON_AddItemToArray(array,cJSON_CreateString(CONTRACTS[i]));
    cJSON_AddItemToObject(json,"contract",array);
    array = cJSON_CreateArray();
    for (i=0; i<BUNDLE.num; i++)
    {
        if ( (prices= BUNDLE.ptrs[i]) != 0 )
        {
            item = cJSON_CreateObject();
            cJSON_AddItemToObject(item,prices->exchange,cJSON_CreateString(prices->contract));
            cJSON_AddItemToObject(item,"base",cJSON_CreateString(prices->base));
            if ( prices->rel[0] != 0 )
                cJSON_AddItemToObject(item,"rel",cJSON_CreateString(prices->rel));
            //printf("(%s) (%s) (%s)\n",prices->contract,prices->base,prices->rel);
            cJSON_AddItemToArray(array,item);
        }
    }
    cJSON_AddItemToObject(json,"result",cJSON_CreateString("success"));
    cJSON_AddItemToObject(json,"list",array);
    jsonstr = cJSON_Print(json), _stripwhite(jsonstr,' '), free_json(json);
    strcpy(retbuf,jsonstr), free(jsonstr);
    printf("list -> (%s)\n",retbuf);
}

double prices777_getprice(char *retbuf,char *base,char *rel,char *contract)
{
    int32_t i,c,basenum,relnum,n = 0; double yprice,daily,revdaily,price,bid,ask;
    struct prices777 *prices; struct prices777_data *dp = &BUNDLE.data;
    price = yprice = daily = revdaily = 0.;
    prices777_ispair(base,rel,contract);
    if ( base[0] != 0 && rel[0] != 0 )
    {
        basenum = prices777_basenum(base), relnum = prices777_basenum(rel);
        if ( basenum >= 0 && relnum >= 0 && basenum < MAX_CURRENCIES && relnum < MAX_CURRENCIES )
            daily = dp->dailyprices[basenum*MAX_CURRENCIES + relnum], revdaily = dp->dailyprices[relnum*MAX_CURRENCIES + basenum];
    }
    for (i=0; i<sizeof(Yahoo_metals)/sizeof(*Yahoo_metals); i++)
        if ( strncmp(Yahoo_metals[i],contract,3) == 0 && strcmp(contract+3,"USD") == 0 )
        {
            yprice = dp->metals[i];
            break;
        }
    sprintf(retbuf,"{\"result\":\"success\",\"contract\":\"%s\",\"base\":\"%s\",\"rel\":\"%s\"",contract,base,rel);
    for (i=0; i<BUNDLE.num; i++)
    {
        if ( (prices= BUNDLE.ptrs[i]) != 0 )
        {
            //printf("(%s) (%s) (%s)\n",prices->contract,prices->base,prices->rel);
            if ( strcmp(contract,prices->contract) == 0 && (bid= prices->orderbook[0][0][0]) != 0 && (ask= prices->orderbook[0][1][0]) != 0 )
            {
                price += (bid + ask), n += 2;
                printf("%s add %f %f -> %f [%f]\n",prices->exchange,bid,ask,price,price/n);
                sprintf(retbuf+strlen(retbuf),",\"%s\":{\"bid\":%.8f,\"ask\":%.8f}",prices->exchange,bid,ask);
            }
        }
    }
    if ( (c= prices777_contractnum(contract,0)) >= 0 )
    {
        if ( dp->tbids[c] != 0. && dp->tasks[c] != 0. )
        {
            price += (dp->tbids[c] + dp->tasks[c]), n += 2;
            sprintf(retbuf+strlen(retbuf),",\"truefx\":{\"millistamp\":\"%llu\",\"bid\":%.8f,\"ask\":%.8f,\"open\":%.8f,\"high\":%.8f,\"low\":%.8f}",(long long)dp->tmillistamps[c],dp->tbids[c],dp->tasks[c],dp->topens[c],dp->thighs[c],dp->tlows[c]);
        }
        if ( dp->fbids[c] != 0. && dp->fasks[c] != 0. )
        {
            price += (dp->fbids[c] + dp->fasks[c]), n += 2;
            sprintf(retbuf+strlen(retbuf),",\"fxcm\":{\"bid\":%.8f,\"ask\":%.8f,\"high\":%.8f,\"low\":%.8f}",dp->fbids[c],dp->fasks[c],dp->fhighs[c],dp->flows[c]);
        }
        if ( dp->ibids[c] != 0. && dp->iasks[c] != 0. )
        {
            price += (dp->ibids[c] + dp->iasks[c]), n += 2;
            sprintf(retbuf+strlen(retbuf),",\"instaforex\":{\"timestamp\":%u,\"bid\":%.8f,\"ask\":%.8f}",dp->itimestamps[c],dp->ibids[c],dp->iasks[c]);
        }
        if ( yprice != 0. )
            sprintf(retbuf+strlen(retbuf),",\"yahoo\":{\"price\":%.8f}",yprice);
        if ( daily != 0. || revdaily != 0. )
            sprintf(retbuf+strlen(retbuf),",\"ecb\":{\"date\":\"%s\",\"daily\":%.8f,\"reverse\":%.8f}",dp->edate,daily,revdaily);
    }
    if ( n > 0 )
        price /= n;
    sprintf(retbuf+strlen(retbuf),",\"aveprice\":%.8f,\"n\":%d}",price,n);
    return(price);
}

int32_t PLUGNAME(_process_json)(char *forwarder,char *sender,int32_t valid,struct plugin_info *plugin,uint64_t tag,char *retbuf,int32_t maxlen,char *jsonstr,cJSON *json,int32_t initflag,char *tokenstr)
{
    char *resultstr,*methodstr;
    retbuf[0] = 0;
    plugin->allowremote = 1;
    printf("<<<<<<<<<<<< INSIDE PLUGIN! process %s (%s)\n",plugin->name,jsonstr);
    if ( initflag > 0 )
    {
        // configure settings
        init_Currencymasks();
        BUNDLE.jsonstr = clonestr(jsonstr);
        BUNDLE.kv = kv777_init("DB","prices",0);
#define INSIDE_BTCD
#ifdef INSIDE_BTCD
        int32_t opreturns_init(uint32_t blocknum,uint32_t blocktimestamp,char *path);
        opreturns_init(0,(uint32_t)time(NULL),"peggy");
#endif
        printf("BUNDLE.kv.%p\n",BUNDLE.kv);
        strcpy(retbuf,"{\"result\":\"prices init\"}");
    }
    else
    {
        if ( plugin_result(retbuf,json,tag) > 0 )
            return((int32_t)strlen(retbuf));
        resultstr = cJSON_str(cJSON_GetObjectItem(json,"result"));
        methodstr = cJSON_str(cJSON_GetObjectItem(json,"method"));
        retbuf[0] = 0;
        if ( methodstr == 0 || methodstr[0] == 0 )
        {
            printf("(%s) has not method\n",jsonstr);
            return(0);
        }
        if ( resultstr != 0 && strcmp(resultstr,"registered") == 0 )
        {
            plugin->registered = 1;
            strcpy(retbuf,"{\"result\":\"activated\"}");
        }
        else if ( strcmp(methodstr,"quote") == 0 )
        {
            double price; char base[16],rel[16],*contract = jstr(json,"contract");
            if ( contract == 0 )
                contract = jstr(json,"c");
            if ( contract != 0 && contract[0] != 0 )
            {
                if ( (price = prices777_getprice(retbuf,base,rel,contract)) == 0. )
                    sprintf(retbuf,"{\"error\":\"zero price returned\",\"contract\":\"%s\"}",contract);
            } else sprintf(retbuf,"{\"error\":\"no contract (c) specified\"}");
        }
        else if ( strcmp(methodstr,"list") == 0 )
            prices777_getlist(retbuf);
    }
    return(plugin_copyretstr(retbuf,maxlen,0));
}
#include "../agents/plugin777.c"
