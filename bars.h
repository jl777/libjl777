//
//  bars.h
//  xcode
//
//  Created by jl777 on 7/25/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_bars_h
#define xcode_bars_h

#define MAX_PRICE 1000000.f
#define NUM_BARPRICES 16

#define BARI_FIRSTBID 0
#define BARI_FIRSTASK 1
#define BARI_LOWBID 2
#define BARI_HIGHASK 3
#define BARI_HIGHBID 4
#define BARI_LOWASK 5
#define BARI_LASTBID 6
#define BARI_LASTASK 7

#define BARI_ARBBID 8
#define BARI_ARBASK 9
#define BARI_MINBID 10
#define BARI_MAXASK 11
#define BARI_VIRTBID 10
#define BARI_VIRTASK 11
#define BARI_AVEBID 12
#define BARI_AVEASK 13
#define BARI_MEDIAN 14
#define BARI_AVEPRICE 15


#define TRADEVALS_FIFOSIZE (60+(2*((HARDCODED_CONTAMINATION_PIXELS+1) * 180))) //TRADEVALS_MAXPRIME)))
#define HARDCODED_CONTAMINATION_PIXELS 6
#define TRADEVALS_MINPRIME 0
#define TRADEVALS_MAXPRIME 64
#define TRADEVALS_WIDTH(j) ((j)+1)

#define SUB_ZFLOATS(dest,src) (dest).bar.V -= (src).bar.V, (dest).nonz.V -= (src).nonz.V
#define ADD_ZFLOATS(dest,src) (dest).bar.V += (src).bar.V, (dest).nonz.V += (src).nonz.V
#define invert_price(x) (((x) == 0.) ? 0. : (1. / (x)))

double _pairaved(double valA,double valB)
{
	if ( valA != 0. && valB != 0. )
		return((valA + valB) / 2.);
	else if ( valA != 0. ) return(valA);
	else return(valB);
}

double _pairave(float valA,float valB)
{
	if ( valA != 0.f && valB != 0.f )
		return((valA + valB) / 2.);
	else if ( valA != 0.f ) return(valA);
	else return(valB);
}


void invert_bar(float invbar[NUM_BARPRICES],float bar[NUM_BARPRICES])
{
    int32_t bari;
    if ( bar[BARI_FIRSTBID] != 0.f )
    {
        for (bari=0; bari<NUM_BARPRICES; bari++)
            invbar[bari] = invert_price(bar[bari]);
    }
}

void add_zfloats(float dest[2][NUM_BARPRICES],float zbar[2][NUM_BARPRICES])
{
    int32_t i;
    for (i=0; i<NUM_BARPRICES; i++)
    {
        dest[0][i] += zbar[0][i];
        dest[1][i] += zbar[1][i];
    }
}

void sub_zfloats(float dest[2][NUM_BARPRICES],float zbar[2][NUM_BARPRICES])
{
    int32_t i;
    for (i=0; i<NUM_BARPRICES; i++)
    {
        dest[0][i] -= zbar[0][i];
        dest[1][i] -= zbar[1][i];
    }
}

void update_barsfifo(float oldest[TRADEVALS_MAXPRIME][2][NUM_BARPRICES],float recent[TRADEVALS_MAXPRIME][HARDCODED_CONTAMINATION_PIXELS][2][NUM_BARPRICES],float barsfifo[TRADEVALS_FIFOSIZE][2][NUM_BARPRICES],int32_t ind,float newbar[NUM_BARPRICES]) // MUST be called for every ind
{
	int32_t j,fifomod,slot,width;
	float border[2][NUM_BARPRICES],newzbar[2][NUM_BARPRICES];
	fifomod = (ind % TRADEVALS_FIFOSIZE);
    memset(newzbar,0,sizeof(newzbar));
    for (j=0; j<NUM_BARPRICES; j++)
        if ( newbar[j] != 0.f )
            newzbar[0][j] = newbar[j], newzbar[1][j] = 1.f;
	memcpy(oldest,barsfifo[fifomod],sizeof(oldest[0]) * NUM_BARPRICES);
	for (j=0; j<TRADEVALS_MAXPRIME; j++)
	{
		width = TRADEVALS_WIDTH(j);
		add_zfloats(recent[j][0],newzbar);
		for (slot=0; slot<HARDCODED_CONTAMINATION_PIXELS+1; slot++)
		{
			memcpy(border,barsfifo[(ind - (slot+1)*width + 1) % TRADEVALS_FIFOSIZE],sizeof(border));
			if ( slot < HARDCODED_CONTAMINATION_PIXELS )
			{
				sub_zfloats(recent[j][slot],border);
				if ( slot < HARDCODED_CONTAMINATION_PIXELS-1 )
					add_zfloats(recent[j][slot+1],border);
                else add_zfloats(oldest[j],border);
            }
			else sub_zfloats(oldest[j],border);
		}
		//for (bari=0; bari<16; bari++)
		//	printf("(%8.6f %.0f) ",(svm->oldest[j].bar.s[bari] / MAX(1,svm->oldest[j].nonz.s[bari])),svm->oldest[j].nonz.s[bari]);
		//printf("oldest.j%d\n",j);
	}
	memcpy(barsfifo[fifomod],newbar,sizeof(barsfifo[fifomod]));
}

float _nonz_min(float oldval,float newval)
{
	if ( newval != 0.f && (oldval == 0.f || newval < oldval) )
		return(newval);
	else return(oldval);
}

float _nonz_max(float oldval,float newval)
{
	if ( newval != 0.f && (oldval == 0.f || newval > oldval) )
		return(newval);
	else return(oldval);
}

float check_price(float price)
{
	if ( isnan(price) != 0 || fabs(price) > MAX_PRICE )
		return(0.f);
	else return(price);
}

void update_bar(float bar[NUM_BARPRICES],float bid,float ask)
{
    bid = check_price(bid);
    ask = check_price(ask);
    if ( bid != 0.f && ask != 0.f )
    {
        bar[BARI_LASTBID] = bid;
        bar[BARI_LASTASK] = ask;
        if ( bar[BARI_FIRSTBID] == 0 )
        {
            bar[BARI_HIGHBID] = bar[BARI_LOWBID] = bar[BARI_FIRSTBID] = bid;
            bar[BARI_HIGHASK] = bar[BARI_LOWASK] = bar[BARI_FIRSTASK] = ask;
        }
        else
        {
            if ( bid > bar[BARI_HIGHBID] ) bar[BARI_HIGHBID] = bid;
            if ( bid < bar[BARI_LOWBID] ) bar[BARI_LOWBID] = bid;
            if ( ask > bar[BARI_HIGHASK] ) bar[BARI_HIGHASK] = ask;
            if ( ask < bar[BARI_LOWASK] ) bar[BARI_LOWASK] = ask;
        }
        //printf("%d.(%f %f) ",i,exp(bid),exp(ask));
    }
}

void barinsert_arbvirts(float bar[NUM_BARPRICES],float arbbid,float arbask,float virtbid,float virtask)
{
    bar[BARI_ARBBID] = arbbid;
    bar[BARI_ARBASK] = arbask;
    bar[BARI_VIRTBID] = virtbid;
    bar[BARI_VIRTASK] = virtask;
}

void calc_barprice_aves(float bar[NUM_BARPRICES])
{
	int32_t n;
	float price,min,max;
	double bidsum,asksum;
	//printf("calc_barprice_aves\n");
	bar[BARI_FIRSTBID] = check_price(bar[BARI_FIRSTBID]);
	if ( (max= bar[BARI_FIRSTBID]) != 0 )
	{
		bidsum = max;
		n = 1;
		if ( (price= bar[BARI_LASTBID]) != 0 ) { n++, bidsum += price; if ( price > max ) max = price; }
		if ( (price= bar[BARI_HIGHBID]) != 0 ) { n++, bidsum += price; if ( price > max ) max = price; }
		if ( (price= bar[BARI_LOWBID]) != 0 ) { n++, bidsum += price; if ( price > max ) max = price; }
		if ( (price= bar[BARI_ARBBID]) != 0 ) { n++, bidsum += price; if ( price > max ) max = price; }
		if ( (price= bar[BARI_VIRTBID]) != 0 ) { n++, bidsum += price; if ( price > max ) max = price; }
		bidsum /= n;
		if ( bar[BARI_AVEBID] != bidsum )
			bar[BARI_AVEBID] = bidsum;
	} else bidsum = 0.;
	bar[BARI_FIRSTASK] = check_price(bar[BARI_FIRSTASK]);
	if ( (min= bar[BARI_FIRSTASK]) != 0 )
	{
		asksum = min;
		n = 1;
		if ( (price= bar[BARI_LASTASK]) != 0 ) { n++, asksum += price; if ( price < min ) min = price; }
		if ( (price= bar[BARI_HIGHASK]) != 0 ) { n++, asksum += price; if ( price < min ) min = price; }
		if ( (price= bar[BARI_LOWASK]) != 0 ) { n++, asksum += price; if ( price < min ) min = price; }
		if ( (price= bar[BARI_ARBASK]) != 0 ) { n++, asksum += price; if ( price < min ) min = price; }
		if ( (price= bar[BARI_VIRTASK]) != 0 ) { n++, asksum += price; if ( price < min ) min = price; }
		asksum /= n;
		if ( bar[BARI_AVEASK] != asksum )
			bar[BARI_AVEASK] = asksum;
	} else asksum = 0.;
	if ( bidsum != 0. && asksum != 0. )
	{
        bar[BARI_AVEPRICE] = (bidsum + asksum) / 2.;
        bar[BARI_MEDIAN] = (min + max) / 2.;
	}
}

void init_bar(float bar[NUM_BARPRICES],float quotes[][2],int32_t n)
{
	int32_t i;
	//for (i=0; i<n; i++)
	//	printf("%d.(%7.6f %7.6f) ",i,exp(quotes[i].x),exp(quotes[i].y));
	//printf("<<<< starting\n");
    memset(bar,0,sizeof(*bar) * NUM_BARPRICES);
	for (i=0; i<n; i++)
        update_bar(bar,quotes[i][0],quotes[i][1]);
    calc_barprice_aves(bar);
}

void _merge_bars(float bar[NUM_BARPRICES],float newbar[NUM_BARPRICES])
{
	if ( bar[BARI_FIRSTBID] == 0.f )
		bar[BARI_FIRSTBID] = newbar[BARI_FIRSTBID];
    if ( bar[BARI_FIRSTASK] == 0.f )
        bar[BARI_FIRSTASK] = newbar[BARI_FIRSTASK];
    bar[BARI_LOWBID] = _nonz_min(bar[BARI_LOWBID],newbar[BARI_LOWBID]);
    bar[BARI_HIGHBID] = _nonz_max(bar[BARI_HIGHBID],newbar[BARI_HIGHBID]);
    bar[BARI_ARBBID] = _nonz_max(bar[BARI_ARBBID],newbar[BARI_ARBBID]);
    bar[BARI_VIRTBID] = _nonz_max(bar[BARI_VIRTBID],newbar[BARI_VIRTBID]);
    bar[BARI_VIRTASK] = _nonz_min(bar[BARI_VIRTASK],newbar[BARI_VIRTASK]);
    bar[BARI_ARBASK] = _nonz_min(bar[BARI_ARBASK],newbar[BARI_ARBASK]);
    bar[BARI_LOWASK] = _nonz_min(bar[BARI_LOWASK],newbar[BARI_LOWASK]);
    bar[BARI_HIGHASK] = _nonz_max(bar[BARI_HIGHASK],newbar[BARI_HIGHASK]);
    if ( newbar[BARI_LASTBID] != 0.f )
        bar[BARI_LASTBID] = newbar[BARI_LASTBID];
    if ( newbar[BARI_LASTASK] != 0.f )
        bar[BARI_LASTASK] = newbar[BARI_LASTASK];
}

void merge_bars(float bar[NUM_BARPRICES],float newbar[NUM_BARPRICES])
{
	_merge_bars(bar,newbar);
	calc_barprice_aves(bar);
}

void calc_diffbar(float diffbar[NUM_BARPRICES],float bar[NUM_BARPRICES],float prevbar[NUM_BARPRICES])
{
	int32_t bari;
    memset(diffbar,0,sizeof(*diffbar) * NUM_BARPRICES);
	for (bari=0; bari<NUM_BARPRICES; bari++)
	{
		if ( bar[bari] != 0.f && prevbar[bari] != 0.f )
			diffbar[bari] = (bar[bari] - prevbar[bari]);
    }
}

void calc_pairaves(float aves[NUM_BARPRICES/2],float bar[NUM_BARPRICES])
{
	int32_t bari;
	for (bari=0; bari<NUM_BARPRICES; bari+=2)
		aves[bari>>1] = _pairave(bar[bari],bar[bari+1]);
}


#endif
