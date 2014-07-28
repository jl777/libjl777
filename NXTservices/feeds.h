//
//  feeds.h
//  xcode
//
//  Created by jl777 on 7/24/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_feeds_h
#define xcode_feeds_h

#define _issue_curl(curl_handle,label,url) bitcoind_RPC(curl_handle,label,url,0,0,0)

int32_t bid_orderbook_tx(struct orderbook_tx *tx,int32_t type,uint64_t nxt64bits,uint64_t obookid,double price,double volume);
int32_t ask_orderbook_tx(struct orderbook_tx *tx,int32_t type,uint64_t nxt64bits,uint64_t obookid,double price,double volume);
uint64_t create_raw_orders(uint64_t assetA,uint64_t assetB);
struct raw_orders *find_raw_orders(uint64_t obookid);
int32_t is_orderbook_bid(int32_t polarity,struct raw_orders *raw,struct orderbook_tx *tx);
struct price_data *get_price_data(uint64_t obookid);

struct exchange_quote { uint32_t jdatetime; float highbid,lowask; };
#define EXCHANGE_QUOTES_INCR ((int32_t)(4096L / sizeof(struct exchange_quote)))
#define INITIAL_PIXELTIME 60

struct exchange_state
{
    double bidminmax[2],askminmax[2],hbla[2];
    char name[64],url[512],url2[512],base[64],rel[64],lbase[64],lrel[64];
    int32_t updated,polarity,numbids,numasks,numbidasks;
    uint32_t type,writeflag;
    uint64_t obookid,feedid,baseid,relid,basemult,relmult;
    struct price_data P;
    FILE *fp;
    double lastmilli;
    struct orderbook_tx **orders;
};

double calc_pixeltimes(double timefactor,double *pixeltimes,uint32_t firstjdatetime,uint32_t lastjdatetime,uint32_t width)
{
    int32_t i,j,iter;
    double jdatetime,incr,factor;
    if ( width >= (lastjdatetime - firstjdatetime + 1) )
    {
        for (j=0,i=width-1; i>=0; i--)
        {
            pixeltimes[i] = lastjdatetime - j++;
            if ( pixeltimes[i] < firstjdatetime )
                break;
        }
    }
    else
    {
        // horribly inefficient at first, but once it gets close it should converge quickly
        factor = (timefactor != 0.) ? timefactor : 1.001;
        for (iter=0; iter<100; iter++)
        {
            incr = INITIAL_PIXELTIME;
            jdatetime = lastjdatetime;
            memset(pixeltimes,0,width*sizeof(*pixeltimes));
            for (i=width-1; i>=0; i--)
            {
                pixeltimes[i] = jdatetime;
                //printf("(%f %f) ",pixeltimes[i],jdatetime);
                jdatetime -= incr;
                incr *= factor;
                if ( jdatetime < firstjdatetime )
                    break;
            }
            printf("iter.%d incr.%f factor %f i.%d jdatetime %f [%f] vs firstjdatetime %d num.%d\n",iter,incr,factor,i,jdatetime,jdatetime-firstjdatetime,firstjdatetime,lastjdatetime - firstjdatetime + 1);
            if ( jdatetime > firstjdatetime )
                factor *= 1.001;
            if ( i < width/100 || fabs(jdatetime-firstjdatetime) / (lastjdatetime - firstjdatetime + 1) < .01 )
                break;
            else if ( i > 0 )
                factor /= 1.00025;
        }
        //for (i=1; i<width; i++)
        //    printf("%.3f ",pixeltimes[i]-pixeltimes[i-1]);
        //printf("pixeltimes\n");
    }
    return(factor);
}

int32_t get_quotes_inrange(int32_t *firstjp,int32_t *lastjp,struct exchange_quote *qps,int32_t numqps,float mintime,float maxtime)
{
    uint32_t jdatetime;
    int32_t first,last,i;
    first = last = -1;
    for (i= *firstjp; i<numqps; i++)
    {
        if ( i < 0 )
            i = 0;
        jdatetime = qps[i].jdatetime;
        if ( jdatetime >= mintime && jdatetime <= maxtime )
        {
            if ( first < 0 )
                first = i;
            last = i;
        }
        else if ( jdatetime > maxtime )
            break;
    }
    *firstjp = first;
    *lastjp = last;
    if ( first >= 0 && last >= 0 )
        return(last - first + 1);
    return(0);
}

int32_t calcspline(double dSplines[][4],double *jdatetimes,double *f,int32_t n)
{
	double c[NUM_PRICEDATA_SPLINES],dd[NUM_PRICEDATA_SPLINES],dl[NUM_PRICEDATA_SPLINES],du[NUM_PRICEDATA_SPLINES],gaps[NUM_PRICEDATA_SPLINES];
	double gap,vx,vy,vw,vz,dSpline[4];
    int32_t i,numsplines;
	numsplines = n;
	if ( n < 4 )
		return(0);
    for (i=0; i<n-1; i++)
        gaps[i] = (jdatetimes[i+1] - jdatetimes[i]);
	for (i=0; i<n-3; i++)
		dl[i] = du[i] = gaps[i+1];
	for (i=0; i<n-2; i++)
	{
		dd[i] = 2.0 * (gaps[i] + gaps[i+1]);
		c[i]  = (3.0 / (double)gaps[i+1]) * (f[i+2] - f[i+1]) - (3.0 / (double)gaps[i]) * (f[i+1] - f[i]);
		//printf("%d.(%.0f %.0f %f %f %f %f)\n",i,gaps[i],gaps[i+1],f[i+2] - f[i+1],(f[i+1] - f[i]),dd[i],c[i]);
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
	for (i=0; i<n-1; i++)
	{
		gap = gaps[i];
		vx = f[i];
		vz = c[i];
		vw = (c[i+1] - c[i]) / (3. * gap);
		vy = ((f[i+1] - f[i]) / gap) - (gap * (c[i+1] + 2.*c[i]) / 3.);
		//vSplines[i] = (float4){ (float)vx, (float)vy, (float)vz, (float)vw };
		dSpline[0] = vx, dSpline[1] = vy, dSpline[2] = vz, dSpline[3] = vw; //= (double4){ vx, vy, vz, vw };
		printf("%3d: %s [%9.6f %9.6f %9.6f %9.6f]\n",i,jdatetime_str((uint32_t)jdatetimes[i]),(vx),vy*1000*1000,vz*1000*1000*1000,vw*1000*1000*1000*1000);
		memcpy(dSplines[i],dSpline,sizeof(dSpline));
    }
 	return(numsplines);
}

int32_t calc_spline_projections(int32_t splinei,double *pricep,double *slopep,double *accelp,double dSplines[][4],double *jdatetimes,int32_t num,int32_t jdatetime)
{
    double gap = 0.;
    *pricep = *slopep = *accelp = 0.;
    //printf("calc_spline_projections splinei.%d of %d j%d [%s %f]\n",splinei,num,jdatetime,jdatetime_str((int)jdatetimes[splinei]),jdatetimes[splinei+1]-jdatetimes[splinei]);
    /*if ( splinei > num-1 || jdatetime >= jdatetimes[splinei+1] )
        splinei = 0;
    if ( jdatetime >= jdatetimes[num-2] )
    {
        if ( jdatetime < jdatetimes[num-1] )
            gap = jdatetime - jdatetimes[num-2];
        else return(0);
    }
    else*/
        if ( jdatetime < jdatetimes[1] || jdatetime > jdatetimes[num-1] )
        return(0);
    //if ( gap == 0. )
    {
        for (splinei=0; splinei<num-1; splinei++)
        {
            if ( jdatetime >= jdatetimes[splinei] && (splinei == num-2 || jdatetime < jdatetimes[splinei+1]) )
            {
                gap = jdatetime - jdatetimes[splinei];
                break;
            }
        }
    }
    if ( gap != 0. )
    {
        //printf("splinei.%d gap %f\n",splinei,gap);
        *pricep = _extrapolate_Spline(dSplines[splinei],gap);
        *slopep = _extrapolate_Slope(dSplines[splinei],gap);
        *accelp = _extrapolate_Accel(dSplines[splinei],gap);
    }
    else
        printf("error finding spline entry: %s %s %f splinei.%d of %d\n",jdatetime_str(jdatetime),jdatetime_str2((int)jdatetimes[num-2]),jdatetimes[num-1]-jdatetimes[num-2],splinei,num);
    return(splinei);
}

void set_default_barwts(double *wts)
{
    int32_t i;
    double sum = 0.;
    wts[BARI_FIRSTBID] = .1;
    wts[BARI_FIRSTASK] = .1;
    wts[BARI_LOWBID] = .025;
    wts[BARI_HIGHASK] = .025;
    wts[BARI_HIGHBID] = .2;
    wts[BARI_LOWASK] = .2;
    wts[BARI_LASTBID] = .25;
    wts[BARI_LASTASK] = .25;
    
    wts[BARI_ARBBID] = .1;
    wts[BARI_ARBASK] = .1;
    wts[BARI_MINBID] = .1;
    wts[BARI_MAXASK] = .1;
    wts[BARI_VIRTBID] = .1;
    wts[BARI_VIRTASK] = .1;
    wts[BARI_AVEBID] = .5;
    wts[BARI_AVEASK] = .5;
    wts[BARI_MEDIAN] = 1.;
    wts[BARI_AVEPRICE] = 1.;
    for (i=0; i<NUM_BARPRICES; i++)
         sum += wts[i];
    for (i=0; i<NUM_BARPRICES; i++)
        wts[i] /= sum;
}

double calc_barwt(double *denp,float bar[NUM_BARPRICES],double wts[NUM_BARPRICES])
{
    int32_t i;
    double sum,den;
    sum = den = 0.;
    for (i=0; i<NUM_BARPRICES; i++)
    {
        if ( bar[i] != 0.f )
        {
            den += wts[i];
            sum += wts[i] * bar[i];
        }
    }
    (*denp) += den;
    return(sum);
}

int32_t calc_pricedata_splinevals(double *splinevals,double *jdatetimes,float *bars,double *pixeltimes,int32_t width)
{
    double blended,val,sum,avetime,wts[NUM_BARPRICES],den,densum,timedist;
    int32_t i,j,m,incr,n = 0;
    set_default_barwts(wts);
    incr = (width / NUM_PRICEDATA_SPLINES);
    m = (NUM_PRICEDATA_SPLINES - 1);
    blended = 0.;
    for (i=width-1; i>=incr; i-=incr)
    {
        avetime = _dbufave(&pixeltimes[i - incr + 1],incr);
        densum = sum = 0.;
        for (j=i-incr+1; j<=i; j++)
        {
            den = 0.;
            val = calc_barwt(&den,&bars[j * NUM_BARPRICES],wts);
            timedist = (avetime - pixeltimes[j]) + 60;
            timedist *= timedist;
            sum += (val / timedist);
            densum += (den / timedist);
        }
        if ( densum != 0. )
        {
            sum /= densum;
            splinevals[m] = sum;
            jdatetimes[m] = avetime;
            m--;
        }
    }
    blended = 0.;
    for (i=0; i<NUM_PRICEDATA_SPLINES-1; i++)
    {
        if ( splinevals[i] != 0. )
        {
            splinevals[n] = splinevals[i];
            dxblend(&blended,splinevals[n],.5);
            jdatetimes[n] = jdatetimes[i];
            printf("(%s %.8f) ",jdatetime_str((int)jdatetimes[n]),splinevals[n]);
            n++;
        }
    }
    if ( n > 1 )
    {
        splinevals[n] = blended;
        jdatetimes[n] = jdatetimes[n-1] + 2*(jdatetimes[n-1] - jdatetimes[n-2]);
        printf("[%s %.8f] ",jdatetime_str((int)jdatetimes[n]),splinevals[n]);
        n++;
    }
    printf("n.%d\n",n);
    return(n);
}

void output_image(char *name,struct price_data *dp)
{
    int32_t i;
    double yval,logave;
    dp->aveprice = _dbufave(dp->splineprices,dp->screenwidth);
    dp->absslope = _dbufabs(dp->slopes,dp->screenwidth);
    dp->absaccel = _dbufabs(dp->accels,dp->screenwidth);
    if ( dp->aveprice != 0. )
        logave = log(dp->aveprice);
    else logave = 1.;
    if ( dp->absslope == 0. )
        dp->absslope = 1.;
    if ( dp->absaccel == 0. )
        dp->absaccel = 1.;
    for (i=0; i<dp->screenwidth; i++)
    {
        disp_yval(0x333333,0,dp->display,i,dp->screenwidth,dp->screenheight);
        if ( dp->splineprices[i] != 0. )
        {
            yval = _calc_pricey(log(dp->splineprices[i]),logave);
            //printf("(%f %f %f) ",dp->aveprices[i],dp->splineprices[i],yval);
            disp_yval(0x00ff00,yval,dp->display,i,dp->screenwidth,dp->screenheight);
            
            yval = _calc_pricey(log(dp->aveprices[i]),logave);
            disp_yval(0x004400,yval,dp->display,i,dp->screenwidth,dp->screenheight);
            
            //yval = _calc_pricey(log(dp->lowasks[i]),logave);
            //disp_yval(0xff0000,yval,dp->displayi,dp->screenwidth,dp->screenheight);
            
            //yval = _calc_pricey(log(dp->highbids[i]),logave);
            //disp_yval(0x0000ff,yval,dp->display,i,dp->screenwidth,dp->screenheight);
            
            yval = 100000000 * dp->slopes[i];
            disp_yval(0xff00ff,yval,dp->display,i,dp->screenwidth,dp->screenheight);
            
            yval = 100000000 * 70000000 * dp->accels[i];
            disp_yval(0x0000ff,yval,dp->display,i,dp->screenwidth,dp->screenheight);
        }
    }
    //printf("%s\n",name);
   /* dp->avedisp = output_line(0,1,dp->aveprice,dp->displine,dp->splineprices,dp->screenwidth,0,dp->display,dp->screenwidth,dp->screenheight);
    dp->aveslope = output_line(0,0,dp->absslope,dp->dispslope,dp->slopes,dp->screenwidth,0,dp->display,dp->screenwidth,dp->screenheight);
    dp->aveaccel = output_line(3,0,dp->absaccel,dp->dispaccel,dp->accels,dp->screenwidth,0,dp->display,dp->screenwidth,dp->screenheight);
    printf("output_image %f %f %f (%s) %f %f %f\n",dp->aveprice,dp->absslope*1000000,dp->absaccel,name,dp->avedisp,dp->aveslope,dp->aveaccel);
    rescale_doubles(dp->displine,dp->screenwidth,(dp->screenheight * .4 ) / dp->avedisp);
    rescale_doubles(dp->dispslope,dp->screenwidth,(dp->screenheight * .33 ) / dp->aveslope);
    rescale_doubles(dp->dispaccel,dp->screenwidth,(dp->screenheight * .25 ) / dp->aveaccel);
    output_line(0,0,1.,dp->displine,dp->displine,dp->screenwidth,0x44ff44,dp->display,dp->screenwidth,dp->screenheight);
    output_line(0,0,1.,dp->dispslope,dp->dispslope,dp->screenwidth,0xff3300,dp->display,dp->screenwidth,dp->screenheight);
    output_line(0,0,1.,dp->dispaccel,dp->dispaccel,dp->screenwidth,0x0088ff,dp->display,dp->screenwidth,dp->screenheight);*/
    output_jpg(name,dp->display,dp->screenwidth,dp->screenheight);
    add_image(name);
}

void clear_price_data_display(struct price_data *dp)
{
    if ( dp->screenwidth == 0 )
        dp->screenwidth = Screenwidth;
    if ( dp->screenheight == 0 )
        dp->screenheight = Screenheight;
    if ( dp->display == 0 )
        dp->display = calloc(dp->screenheight * dp->screenwidth,sizeof(*dp->display));
    if ( dp->pixeltimes == 0 )
        dp->pixeltimes = calloc(dp->screenwidth,sizeof(*dp->pixeltimes));
    if ( dp->bars == 0 )
        dp->bars = calloc(dp->screenwidth,sizeof(*dp->bars) * NUM_BARPRICES);
    memset(dp->highbids,0,sizeof(dp->highbids));
    memset(dp->lowasks,0,sizeof(dp->lowasks));
    memset(dp->aveprices,0,sizeof(dp->aveprices));
    memset(dp->jdatetimes,0,sizeof(dp->jdatetimes));
    memset(dp->splineprices,0,sizeof(dp->splineprices));
    memset(dp->slopes,0,sizeof(dp->slopes));
    memset(dp->accels,0,sizeof(dp->accels));
    memset(dp->displine,0,sizeof(dp->displine));
    memset(dp->dispslope,0,sizeof(dp->dispslope));
    memset(dp->dispaccel,0,sizeof(dp->dispaccel));
    memset(dp->splinevals,0,sizeof(dp->splinevals));
    memset(dp->avebar,0,sizeof(dp->avebar));
    memset(dp->dSplines,0,sizeof(dp->dSplines));
    memset(dp->bars,0,dp->screenwidth * sizeof(*dp->bars) * NUM_BARPRICES);
    memset(dp->pixeltimes,0,dp->screenwidth * sizeof(*dp->pixeltimes));
    memset(dp->display,0,dp->screenheight * dp->screenwidth * sizeof(*dp->display));
}

void recalc_price_display(char *name,struct price_data *dp)
{
    float mintime,maxtime,*bar;
    double jdatetime;
    struct exchange_quote *qp;
    int32_t i,j,n,firstj,lastj,numsplines;
    printf("recalc %s numquotes.%d\n",name,dp->numquotes);
    clear_price_data_display(dp);
    dp->timefactor = calc_pixeltimes(dp->timefactor,dp->pixeltimes,dp->firstjdatetime,dp->lastjdatetime,dp->screenwidth-MAX_LOOKAHEAD);
    firstj = 0;
    for (i=0; i<dp->screenwidth-MAX_LOOKAHEAD; i++)
    {
        bar = &dp->bars[i * NUM_BARPRICES];
        mintime = dp->pixeltimes[i];
        maxtime = dp->pixeltimes[i+1];
        if ( mintime != 0.f && maxtime != 0.f )
        {
            if ( (n= get_quotes_inrange(&firstj,&lastj,dp->allquotes,dp->numquotes,mintime,maxtime)) != 0 )
            {
                //printf("i.%d: %d quotes between %f and %f, firstj.%d lastj.%d numquotes.%d\n",i,n,mintime,maxtime,firstj,lastj,dp->numquotes);
                if ( firstj >= 0 && lastj < dp->numquotes && firstj <= lastj && lastj >= 0 )
                {
                    for (j=firstj; j<=lastj; j++)
                    {
                        qp = &dp->allquotes[j];
                        update_bar(bar,qp->highbid,qp->lowask);
                    }
                    calc_barprice_aves(bar);
                    _merge_bars(dp->avebar,bar);
                    firstj = lastj;
                }
                else firstj = 0;
            }
        }
        dp->highbids[i] = update_filtered_buf(&dp->bidfb,bar[BARI_HIGHBID],widths8);
        dp->lowasks[i] = update_filtered_buf(&dp->askfb,bar[BARI_LOWASK],widths8);
        dp->aveprices[i] = update_filtered_buf(&dp->avefb,_pairaved(bar[BARI_AVEPRICE],bar[BARI_MEDIAN]),widths16);
    }
    calc_barprice_aves(dp->avebar);
    n = calc_pricedata_splinevals(dp->splinevals,dp->jdatetimes,dp->bars,dp->pixeltimes,dp->screenwidth);
    dp->numsplines = numsplines = calcspline(dp->dSplines,dp->jdatetimes,dp->splinevals,n);
    if ( numsplines > 0 )
    {
        //for (i=0; i<dp->numsplines; i++)
        //    printf("%s ",jdatetime_str((int)dp->jdatetimes[i]));
        for (i=j=0; i<dp->screenwidth; i++)
        {
            if ( (jdatetime= dp->pixeltimes[i]) != 0 )
            {
                j = calc_spline_projections(j,&dp->splineprices[i],&dp->slopes[i],&dp->accels[i],dp->dSplines,dp->jdatetimes,numsplines,jdatetime);
                update_filtered_buf(&dp->avefb,dp->splineprices[i],widths8);
                if ( i > 19 )
                {
                    dp->slopes[i] = update_filtered_buf(&dp->slopefb,_dbufave(dp->avefb.slopes,4),widths8);
                    if ( i > 19*2 )
                        dp->accels[i] = -update_filtered_buf(&dp->accelfb,_dbufave(dp->slopefb.slopes,4),widths4);
                }
                //printf("(%.6f %.6f) ",dp->aveprices[i],dp->splineprices[i]);
            }
        }
        //printf("splineprices\n");
        output_image(name,dp);
    }
}

void update_contract_pair(char *name,struct price_data *dp,int32_t width,int32_t height,uint64_t obookid,struct exchange_state *eps[],int32_t m)
{
    /*void free_orderbook(struct orderbook *op);
    struct orderbook *create_orderbook(uint64_t obookid,int32_t polarity);
    struct orderbook *op;
    struct raw_orders *raw;
    struct orderbook_tx **allorders;
    ,total;
    if ( (raw= find_raw_orders(obookid)) != 0 )
    {
        op = create_orderbook(obookid,eps[0]->polarity);
        //sort_orderbook(op,allorders,total,raw);
        free_orderbook(op);
    }*/
    double wts[NUM_BARPRICES];
    float bar[NUM_BARPRICES];
    int32_t i,j,nonz,*splineis;
    double jdatetime,price,slope,slopesum,accel,accelsum,pricesum;
    struct price_data *_dp;
    //if ( strcmp(dp->base,"BTCD") != 0 )
    //    return;
    if ( m <= 0 )
        return;
    dp->firstjdatetime = dp->lastjdatetime = 0;
    for (i=0; i<m; i++)
    {
        if ( eps[i]->P.firstjdatetime != 0 && (dp->firstjdatetime == 0 || eps[i]->P.firstjdatetime < dp->firstjdatetime) )
            dp->firstjdatetime = eps[i]->P.firstjdatetime;
        if ( eps[i]->P.lastjdatetime != 0 && (dp->lastjdatetime == 0 || eps[i]->P.lastjdatetime > dp->lastjdatetime) )
            dp->lastjdatetime = eps[i]->P.lastjdatetime;
    }
    clear_price_data_display(dp);
    dp->timefactor = calc_pixeltimes(dp->timefactor,dp->pixeltimes,dp->firstjdatetime,dp->lastjdatetime,dp->screenwidth-MAX_LOOKAHEAD);
    splineis = calloc(m,sizeof(*splineis));
    set_default_barwts(wts);
    for (i=j=0; i<dp->screenwidth; i++)
    {
        if ( (jdatetime= dp->pixeltimes[i]) != 0 )
        {
            memset(bar,0,sizeof(bar));
            slopesum = accelsum = pricesum = 0.;
            for (j=nonz=0; j<m; j++)
            {
                _dp = &eps[j]->P;
                if ( _dp->numsplines > 0 )
                {
                    splineis[j] = calc_spline_projections(splineis[j],&price,&slope,&accel,_dp->dSplines,_dp->jdatetimes,_dp->numsplines,jdatetime);
                    update_bar(bar,price - _dp->halfspread,price + _dp->halfspread);
                    if ( price != 0 )
                    {
                        nonz++;
                        if ( _dp->polarity*dp->polarity < 0 )
                        {
                            price = 1. / price;
                            slope = -_dp->slopes[i];
                            accel = -_dp->accels[i];
                        }
                        else
                        {
                            slope = _dp->slopes[i];
                            accel = _dp->accels[i];
                        }
                        pricesum += price;
                        slopesum += slope;
                        accelsum += accel;
                    }
                 }
            }
            if ( nonz != 0 )
            {
                calc_barprice_aves(bar);
                /*price = calc_barwt(&den,bar,wts);
                if ( price != 0. && den != 0. )
                {
                    price /= den;
                    printf("%f ",price);
                }*/
                price = (pricesum / nonz);
                //printf("%f ",price);
                dp->slopes[i] = slopesum / nonz;
                dp->accels[i] = accelsum / nonz;
                dp->splineprices[i] = update_filtered_buf(&dp->avefb,price,widths8);
            }
        }
    }
    free(splineis);
    output_image(name,dp);
}

void add_exchange_quote(char *name,char *base,char *rel,int32_t recalcflag,struct price_data *dp,struct exchange_quote *qp)
{
    char fname[512];
    if ( qp->highbid == 0 || qp->lowask == 0 || qp->lowask < qp->highbid )
    {
        printf("warning %s %s %s: illegal quote %f %f\n",name,base,rel,qp->highbid,qp->lowask);
        return;
    }
    if ( dp->numquotes >= dp->maxquotes )
    {
        dp->maxquotes += EXCHANGE_QUOTES_INCR;
        dp->allquotes = realloc(dp->allquotes,sizeof(*dp->allquotes) * dp->maxquotes);
        memset(&dp->allquotes[dp->numquotes],0,sizeof(*dp->allquotes) * (dp->maxquotes - dp->numquotes));
    }
    dp->allquotes[dp->numquotes++] = *qp;
    dp->bidsum += qp->highbid;
    dp->asksum += qp->lowask;
    dp->halfspread = (dp->asksum - dp->bidsum) / (dp->numquotes << 1);
    if ( dp->firstjdatetime == 0 || qp->jdatetime < dp->firstjdatetime )
        dp->firstjdatetime = qp->jdatetime;
    if ( qp->jdatetime > dp->lastjdatetime )
        dp->lastjdatetime = qp->jdatetime;
    printf("%12s %s %5s/%-5s %.8f %.8f\n",name,jdatetime_str(qp->jdatetime),base,rel,qp->highbid,qp->lowask);
    if ( recalcflag != 0 )
    {
        sprintf(fname,"%s_%s_%s",name,base,rel);
        //if ( strcmp(name,"bittrex") == 0 )
            recalc_price_display(fname,dp);
    }
}

uint64_t ensure_orderbook(uint64_t *baseidp,uint64_t *relidp,int32_t *polarityp,char *base,char *rel)
{
    uint64_t baseid,relid,obookid;
    struct raw_orders *raw;
    *baseidp = baseid = get_orderbook_assetid(base);
    *relidp = relid = get_orderbook_assetid(rel);
    if ( baseid == 0 || relid == 0 )
        return(0);
    obookid = create_raw_orders(baseid,relid);
    raw = find_raw_orders(obookid);
    if ( raw->assetA == baseid && raw->assetB == relid )
        *polarityp = 1;
    else if ( raw->assetA == relid && raw->assetB == baseid )
        *polarityp = -1;
    else *polarityp = 0;
    return(obookid);
}

int32_t load_exchange_quote(struct exchange_quote *qp,FILE *fp)
{
    if ( fread(qp,1,sizeof(*qp),fp) == sizeof(*qp) )
        return(0);
    else return(-1);
}

struct exchange_state *init_exchange_state(int32_t writeflag,char *name,char *base,char *rel,uint64_t feedid,uint32_t type,int32_t quotefile)
{
    char fname[512];
    struct exchange_quote Q;
    int32_t i,n;
    struct exchange_state *ep;
    long fpos,entrysize = (sizeof(uint32_t) + 2*sizeof(float));
    ep = calloc(1,sizeof(*ep));
    memset(ep,0,sizeof(*ep));
    ep->writeflag = writeflag;
    safecopy(ep->name,name,sizeof(ep->name));
    safecopy(ep->base,base,sizeof(ep->base));
    safecopy(ep->lbase,base,sizeof(ep->lbase));
    touppercase(ep->base); tolowercase(ep->lbase);
    safecopy(ep->rel,rel,sizeof(ep->rel));
    safecopy(ep->lrel,rel,sizeof(ep->lrel));
    touppercase(ep->rel); tolowercase(ep->lrel);
    ep->basemult = ep->relmult = 1L;
    ep->feedid = feedid;
    ep->type = type;
    ep->obookid = ensure_orderbook(&ep->baseid,&ep->relid,&ep->polarity,base,rel);
    ep->P.obookid = ep->obookid;
    ep->P.polarity = ep->polarity;
    safecopy(ep->P.base,base,sizeof(ep->P.base));
    safecopy(ep->P.rel,rel,sizeof(ep->P.rel));
    if ( ep->obookid == 0 )
    {
        printf("couldnt initialize orderbook for %s %s %s\n",name,base,rel);
        free(ep);
        return(0);
    }
    if ( strcmp(name,"NXT") == 0 )
    {
        ep->basemult = get_asset_mult(ep->baseid);
        if ( ep->relid != ORDERBOOK_NXTID || ep->basemult == 0 )
        {
            printf("NXT only supports trading against NXT not %s || basemult.%llu is zero\n",ep->rel,(long long)ep->basemult);
            free(ep);
            return(0);
        }
    }
    if ( quotefile != 0 )
    {
        ensure_directory("exchangedata");
        sprintf(fname,"exchangedata/%s_%s_%s",name,base,rel);
        ep->fp = fopen(fname,writeflag!=0?"rb+":"rb");
        if ( ep->fp != 0 )
        {
            fseek(ep->fp,0,SEEK_END);
            fpos = ftell(ep->fp);
            n = (int32_t)(fpos / entrysize);
            if ( (fpos % entrysize) != 0 )
                fpos = n * entrysize;
            if ( writeflag != 0 )
                fseek(ep->fp,fpos,SEEK_SET);
            else
            {
                rewind(ep->fp);
                for (i=0; i<n; i++)
                {
                    if ( load_exchange_quote(&Q,ep->fp) == 0 )
                        add_exchange_quote(ep->name,ep->base,ep->rel,0,&ep->P,&Q);
                    else
                    {
                        printf("error loading %s %s %s %d of %d\n",ep->name,ep->base,ep->rel,i,n);
                        break;
                    }
                }
            }
        }
        else if ( writeflag != 0 ) ep->fp = fopen(fname,"wb");
        printf("exchange file.(%s) %p\n",fname,ep->fp);
    }
    return(ep);
}

void prep_exchange_state(struct exchange_state *ep)
{
    memset(ep->bidminmax,0,sizeof(ep->bidminmax));
    memset(ep->askminmax,0,sizeof(ep->askminmax));
}

int32_t generate_quote_entry(struct exchange_state *ep)
{
    struct exchange_quote Q;
    ep->updated = 0;
    if ( ep->hbla[0] == 0. || ep->bidminmax[1] != ep->hbla[0] )
        ep->hbla[0] = ep->bidminmax[1], ep->updated = 1;
    if ( ep->hbla[1] == 0. || ep->askminmax[0] != ep->hbla[1] )
        ep->hbla[1] = ep->askminmax[0], ep->updated = 1;
    if ( ep->updated != 0 && ep->fp != 0 && ep->hbla[0] != 0. && ep->hbla[1] != 0. )
    {
        Q.jdatetime = conv_unixtime((uint32_t)time(NULL));
        Q.highbid = ep->hbla[0];
        Q.lowask = ep->hbla[1];
        if ( ep->writeflag != 0 && fwrite(&Q,1,sizeof(Q),ep->fp) != sizeof(Q) )
            printf("error writing quote to %s\n",ep->name);
        else
        {
            if ( ep->writeflag == 0 )
                add_exchange_quote(ep->name,ep->base,ep->rel,1,&ep->P,&Q);
            else printf("%12s %s %5s/%-5s %.8f (%.8f %.8f) %.8f (%.8f %.8f)\n",ep->name,jdatetime_str(Q.jdatetime),ep->base,ep->rel,Q.highbid,ep->bidminmax[0],ep->bidminmax[1],Q.lowask,ep->askminmax[0],ep->askminmax[1]);
        }
        fflush(ep->fp);
    }
    return(ep->updated);
}

struct orderbook_tx **conv_quotes(int32_t *nump,int32_t type,uint64_t nxt64bits,uint64_t obookid,int32_t polarity,double *bids,int32_t numbids,double *asks,int32_t numasks)
{
    int32_t iter,i,ret,m,n = 0;
    double *quotes;
    struct orderbook_tx *tx,**orders = 0;
    if ( (m= (numbids + numasks)) == 0 )
        return(0);
    orders = (struct orderbook_tx **)calloc(m,sizeof(*orders));
    tx = 0;
    for (iter=-1; iter<=1; iter+=2)
    {
        if ( iter < 0 )
        {
            quotes = asks;
            m = numasks;
        }
        else
        {
            quotes = bids;
            m = numbids;
        }
        for (i=0; i<m; i++)
        {
            if ( tx == 0 )
                tx = (struct orderbook_tx *)calloc(1,sizeof(*tx));
            if ( iter*polarity > 0 )
                ret = bid_orderbook_tx(tx,type,nxt64bits,obookid,quotes[i*2],quotes[i*2+1]);
            else ret = ask_orderbook_tx(tx,type,nxt64bits,obookid,quotes[i*2],quotes[i*2+1]);
            if ( ret == 0 )
            {
                orders[n++] = tx;
                tx = 0;
            }
        }
    }
    if ( tx != 0 )
        free(tx);
    *nump = n;
    return(orders);
}

int32_t parse_json_quotes(double minmax[2],double *quotes,cJSON *array,int32_t maxdepth,char *pricefield,char *volfield)
{
    cJSON *item;
    int32_t i,n;
    double price;
    n = cJSON_GetArraySize(array);
    if ( maxdepth != 0 && n > maxdepth )
        n = maxdepth;
    for (i=0; i<n; i++)
    {
        item = cJSON_GetArrayItem(array,i);
        if ( pricefield != 0 && volfield != 0 )
        {
            quotes[(i<<1)] = price = get_API_float(cJSON_GetObjectItem(item,pricefield));
            quotes[(i<<1) + 1] = get_API_float(cJSON_GetObjectItem(item,volfield));
        }
        else if ( is_cJSON_Array(item) != 0 && cJSON_GetArraySize(item) == 2 ) // big assumptions about order within nested array!
        {
            quotes[(i<<1)] = price = get_API_float(cJSON_GetArrayItem(item,0));
            quotes[(i<<1) + 1] = get_API_float(cJSON_GetArrayItem(item,1));
        }
        else
        {
            printf("unexpected case in parse_json_quotes\n");
            continue;
        }
        if ( minmax[0] == 0. || price < minmax[0] )
            minmax[0] = price;
        if ( minmax[1] == 0. || price > minmax[1] )
            minmax[1] = price;
    }
    return(n);
}

struct orderbook_tx **parse_json_orderbook(struct exchange_state *ep,int32_t maxdepth,cJSON *json,char *resultfield,char *bidfield,char *askfield,char *pricefield,char *volfield)
{
    cJSON *obj = 0,*bidobj,*askobj;
    double *bids,*asks;
    struct orderbook_tx **orders = 0;
    if ( resultfield == 0 )
        obj = json;
    if ( maxdepth == 0 )
        maxdepth = 100;
    //printf("resultfield.%p obj.%p json.%p\n",resultfield,obj,json);
    if ( resultfield == 0 || (obj= cJSON_GetObjectItem(json,resultfield)) != 0 )
    {
        //printf("inside\n");
        bids = (double *)calloc(maxdepth,sizeof(double) * 2);
        asks = (double *)calloc(maxdepth,sizeof(double) * 2);
        if ( (bidobj= cJSON_GetObjectItem(obj,bidfield)) != 0 && is_cJSON_Array(bidobj) != 0 )
            ep->numbids = parse_json_quotes(ep->bidminmax,bids,bidobj,maxdepth,pricefield,volfield);
        if ( (askobj= cJSON_GetObjectItem(obj,askfield)) != 0 && is_cJSON_Array(askobj) != 0 )
            ep->numasks = parse_json_quotes(ep->askminmax,asks,askobj,maxdepth,pricefield,volfield);
        orders = conv_quotes(&ep->numbidasks,ep->type,ep->feedid,ep->obookid,ep->polarity,bids,ep->numbids,asks,ep->numasks);
        free(bids);
        free(asks);
        generate_quote_entry(ep);
        if ( orders != 0 )
        {
            if ( ep->orders != 0 )
                free(ep->orders);
            ep->orders = orders;
        }
    }
    return(ep->orders);
}

struct orderbook_tx **parse_cryptsy(struct exchange_state *ep,int32_t maxdepth) // "BTC-BTCD"
{
    static char *marketstr = "[{\"42\":141},{\"AC\":199},{\"AGS\":253},{\"ALF\":57},{\"AMC\":43},{\"ANC\":66},{\"APEX\":257},{\"ARG\":48},{\"AUR\":160},{\"BC\":179},{\"BCX\":142},{\"BEN\":157},{\"BET\":129},{\"BLU\":251},{\"BQC\":10},{\"BTB\":23},{\"BTCD\":256},{\"BTE\":49},{\"BTG\":50},{\"BUK\":102},{\"CACH\":154},{\"CAIx\":221},{\"CAP\":53},{\"CASH\":150},{\"CAT\":136},{\"CGB\":70},{\"CINNI\":197},{\"CLOAK\":227},{\"CLR\":95},{\"CMC\":74},{\"CNC\":8},{\"CNL\":260},{\"COMM\":198},{\"COOL\":266},{\"CRC\":58},{\"CRYPT\":219},{\"CSC\":68},{\"DEM\":131},{\"DGB\":167},{\"DGC\":26},{\"DMD\":72},{\"DOGE\":132},{\"DRK\":155},{\"DVC\":40},{\"EAC\":139},{\"ELC\":12},{\"EMC2\":188},{\"EMD\":69},{\"EXE\":183},{\"EZC\":47},{\"FFC\":138},{\"FLT\":192},{\"FRAC\":259},{\"FRC\":39},{\"FRK\":33},{\"FST\":44},{\"FTC\":5},{\"GDC\":82},{\"GLC\":76},{\"GLD\":30},{\"GLX\":78},{\"GLYPH\":229},{\"GUE\":241},{\"HBN\":80},{\"HUC\":249},{\"HVC\":185},{\"ICB\":267},{\"IFC\":59},{\"IXC\":38},{\"JKC\":25},{\"JUDGE\":269},{\"KDC\":178},{\"KEY\":255},{\"KGC\":65},{\"LGD\":204},{\"LK7\":116},{\"LKY\":34},{\"LTB\":202},{\"LTC\":3},{\"LTCX\":233},{\"LYC\":177},{\"MAX\":152},{\"MEC\":45},{\"MIN\":258},{\"MINT\":156},{\"MN1\":187},{\"MN2\":196},{\"MNC\":7},{\"MRY\":189},{\"MYR\":200},{\"MZC\":164},{\"NAN\":64},{\"NAUT\":207},{\"NAV\":252},{\"NBL\":32},{\"NEC\":90},{\"NET\":134},{\"NMC\":29},{\"NOBL\":264},{\"NRB\":54},{\"NRS\":211},{\"NVC\":13},{\"NXT\":159},{\"NYAN\":184},{\"ORB\":75},{\"OSC\":144},{\"PHS\":86},{\"Points\":120},{\"POT\":173},{\"PPC\":28},{\"PSEUD\":268},{\"PTS\":119},{\"PXC\":31},{\"PYC\":92},{\"QRK\":71},{\"RDD\":169},{\"RPC\":143},{\"RT2\":235},{\"RYC\":9},{\"RZR\":237},{\"SAT2\":232},{\"SBC\":51},{\"SC\":225},{\"SFR\":270},{\"SHLD\":248},{\"SMC\":158},{\"SPA\":180},{\"SPT\":81},{\"SRC\":88},{\"STR\":83},{\"SUPER\":239},{\"SXC\":153},{\"SYNC\":271},{\"TAG\":117},{\"TAK\":166},{\"TEK\":114},{\"TES\":223},{\"TGC\":130},{\"TOR\":250},{\"TRC\":27},{\"UNB\":203},{\"UNO\":133},{\"URO\":247},{\"USDe\":201},{\"UTC\":163},{\"VIA\":261},{\"VOOT\":254},{\"VRC\":209},{\"VTC\":151},{\"WC\":195},{\"WDC\":14},{\"XC\":210},{\"XJO\":115},{\"XLB\":208},{\"XPM\":63},{\"XXX\":265},{\"YAC\":11},{\"YBC\":73},{\"ZCC\":140},{\"ZED\":170},{\"ZET\":85}]";

    int32_t i,m;
    cJSON *json,*obj,*marketjson;
    char *jsonstr,market[128];
    if ( ep->url[0] == 0 )
    {
        if ( strcmp(ep->rel,"BTC") != 0 )
        {
            printf("parse_cryptsy: only BTC markets supported\n");
            return(0);
        }
        marketjson = cJSON_Parse(marketstr);
        if ( marketjson == 0 )
        {
            printf("parse_cryptsy: cant parse.(%s)\n",marketstr);
            return(0);
        }
        m = cJSON_GetArraySize(marketjson);
        market[0] = 0;
        for (i=0; i<m; i++)
        {
            obj = cJSON_GetArrayItem(marketjson,i);
            printf("(%s) ",get_cJSON_fieldname(obj));
            if ( strcmp(ep->base,get_cJSON_fieldname(obj)) == 0 )
            {
                printf("set market -> %llu\n",(long long)obj->child->valueint);
                sprintf(market,"%llu",(long long)obj->child->valueint);
                break;
            }
        }
        free(marketjson);
        if ( market[0] == 0 )
        {
            printf("parse_cryptsy: no marketid\n");
            return(0);
        }
        sprintf(ep->url,"http://pubapi.cryptsy.com/api.php?method=singleorderdata&marketid=%s",market);
    }
    prep_exchange_state(ep);
    jsonstr = _issue_curl(0,ep->name,ep->url);
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( get_cJSON_int(json,"success") == 1 && (obj= cJSON_GetObjectItem(json,"return")) != 0 )
            {
                parse_json_orderbook(ep,maxdepth,obj,ep->base,"buyorders","sellorders","price","quantity");
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(ep->orders);
}

struct orderbook_tx **parse_bittrex(struct exchange_state *ep,int32_t maxdepth) // "BTC-BTCD"
{
    cJSON *json,*obj;
    char *jsonstr,market[128];
    prep_exchange_state(ep);
    if ( ep->url[0] == 0 )
    {
        sprintf(market,"%s-%s",ep->rel,ep->base);
        sprintf(ep->url,"https://bittrex.com/api/v1.1/public/getorderbook?market=%s&type=both&depth=%d",market,maxdepth);
    }
    jsonstr = _issue_curl(0,ep->name,ep->url);
    //printf("(%s) -> (%s)\n",ep->url,jsonstr);
   // {"success":true,"message":"","result":{"buy":[{"Quantity":1.69284935,"Rate":0.00122124},{"Quantity":130.39771416,"Rate":0.00122002},{"Quantity":77.31781088,"Rate":0.00122000},{"Quantity":10.00000000,"Rate":0.00120150},{"Quantity":412.23470195,"Rate":0.00119500}],"sell":[{"Quantity":8.58353086,"Rate":0.00123019},{"Quantity":10.93796714,"Rate":0.00127744},{"Quantity":17.96825904,"Rate":0.00128000},{"Quantity":2.80400381,"Rate":0.00129999},{"Quantity":200.00000000,"Rate":0.00130000}]}}
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= cJSON_GetObjectItem(json,"success")) != 0 && is_cJSON_True(obj) != 0 )
                parse_json_orderbook(ep,maxdepth,json,"result","buy","sell","Rate","Quantity");
            free_json(json);
        }
        free(jsonstr);
    }
    return(ep->orders);
}

struct orderbook_tx **parse_poloniex(struct exchange_state *ep,int32_t maxdepth)
{
    cJSON *json;
    char *jsonstr,market[128];
    prep_exchange_state(ep);
    if ( ep->url[0] == 0 )
    {
        sprintf(market,"%s_%s",ep->rel,ep->base);
        sprintf(ep->url,"https://poloniex.com/public?command=returnOrderBook&currencyPair=%s&depth=%d",market,maxdepth);
    }
    jsonstr = _issue_curl(0,ep->name,ep->url);
    //{"asks":[[7.975e-5,200],[7.98e-5,108.46834002],[7.998e-5,1.2644054],[8.0e-5,1799],[8.376e-5,1.7528442],[8.498e-5,30.25055195],[8.499e-5,570.01953171],[8.5e-5,1399.91458777],[8.519e-5,123.82790941],[8.6e-5,1000],[8.696e-5,1.3914002],[8.7e-5,1000],[8.723e-5,112.26190114],[8.9e-5,181.28118327],[8.996e-5,1.237759],[9.0e-5,270.096],[9.049e-5,2993.99999999],[9.05e-5,3383.48549983],[9.068e-5,2.52582092],[9.1e-5,30],[9.2e-5,40],[9.296e-5,1.177861],[9.3e-5,81.59999998],[9.431e-5,2],[9.58e-5,1.074289],[9.583e-5,3],[9.644e-5,858.48948115],[9.652e-5,3441.55358115],[9.66e-5,15699.9569377],[9.693e-5,23.5665998],[9.879e-5,1.0843656],[9.881e-5,2],[9.9e-5,700],[9.999e-5,123.752],[0.0001,34.04293],[0.00010397,1.742916],[0.00010399,11.24446],[0.00010499,1497.79999999],[0.00010799,1.2782902],[0.000108,1818.80661458],[0.00011,1395.27245417],[0.00011407,0.89460453],[0.00011409,0.89683778],[0.0001141,0.906],[0.00011545,458.09939081],[0.00011599,5],[0.00011798,1.0751625],[0.00011799,5],[0.00011999,5.86],[0.00012,279.64865088]],"bids":[[7.415e-5,4495],[7.393e-5,1.8650999],[7.392e-5,974.53828463],[7.382e-5,896.34272554],[7.381e-5,3000],[7.327e-5,1276.26600246],[7.326e-5,77.32705432],[7.32e-5,190.98472093],[7.001e-5,2.2642602],[7.0e-5,1112.19485714],[6.991e-5,2000],[6.99e-5,5000],[6.951e-5,500],[6.914e-5,91.63013181],[6.9e-5,500],[6.855e-5,500],[6.85e-5,238.86947265],[6.212e-5,5.2800413],[6.211e-5,4254.38737723],[6.0e-5,1697.3335],[5.802e-5,3.1241932],[5.801e-5,4309.60179279],[5.165e-5,20],[5.101e-5,6.2903434],[5.1e-5,100],[5.0e-5,5000],[4.5e-5,15],[3.804e-5,16.67907],[3.803e-5,30],[3.002e-5,1400],[3.001e-5,15.320937],[3.0e-5,10000],[2.003e-5,32.345771],[2.002e-5,50],[2.0e-5,25000],[1.013e-5,79.250137],[1.012e-5,200],[1.01e-5,200000],[2.0e-7,5000],[1.9e-7,5000],[1.4e-7,1644.2107],[1.2e-7,1621.8622],[1.1e-7,10000],[1.0e-7,100000],[6.0e-8,4253.7528],[4.0e-8,3690.3146],[3.0e-8,100000],[1.0e-8,100000]],"isFrozen":"0"}
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            parse_json_orderbook(ep,maxdepth,json,0,"bids","asks",0,0);
            free_json(json);
        }
        free(jsonstr);
    }
    return(ep->orders);
}

struct orderbook_tx **parse_bter(struct exchange_state *ep,int32_t maxdepth)
{
    cJSON *json,*obj;
    char resultstr[512],*jsonstr;
    prep_exchange_state(ep);
    if ( ep->url[0] == 0 )
        sprintf(ep->url,"http://data.bter.com/api/1/depth/%s_%s",ep->lbase,ep->lrel);
    jsonstr = _issue_curl(0,ep->name,ep->url);
    //printf("(%s) -> (%s)\n",ep->url,jsonstr);
    //{"result":"true","asks":[["0.00008035",100],["0.00008030",2030],["0.00008024",100],["0.00008018",643.41783554],["0.00008012",100]
    if ( jsonstr != 0 )
    {
        if ( (json = cJSON_Parse(jsonstr)) != 0 )
        {
            if ( (obj= cJSON_GetObjectItem(json,"result")) != 0 )
            {
                copy_cJSON(resultstr,obj);
                if ( strcmp(resultstr,"true") == 0 )
                {
                    maxdepth = 0; // since bter ask is wrong order, need to scan entire list
                    parse_json_orderbook(ep,maxdepth,json,0,"bids","asks",0,0);
                }
            }
            free_json(json);
        }
        free(jsonstr);
    }
    return(ep->orders);
}

struct orderbook_tx **parse_mintpal(struct exchange_state *ep,int32_t maxdepth)
{
    cJSON *json,*bidobj,*askobj;
    char *buystr,*sellstr;
    prep_exchange_state(ep);
    if ( ep->url[0] == 0 )
        sprintf(ep->url,"https://api.mintpal.com/v1/market/orders/%s/%s/BUY",ep->base,ep->rel);
    if ( ep->url2[0] == 0 )
        sprintf(ep->url2,"https://api.mintpal.com/v1/market/orders/%s/%s/SELL",ep->base,ep->rel);
    buystr = _issue_curl(0,ep->name,ep->url);
    sellstr = _issue_curl(0,ep->name,ep->url2);
//{"count":3,"type":"BUY","orders":[{"price":"0.00000003","amount":"137066327.96066495","total":"4.11198982"},{"price":"0.00000002","amount":"293181381.39291047","total":"5.86362760"},{"price":"0.00000001","amount":"493836943.18472463","total":"4.93836939"}]}
    if ( buystr != 0 && sellstr != 0 )
    {
        bidobj = askobj = 0;
        if ( (json = cJSON_Parse(buystr)) != 0 )
        {
            bidobj = cJSON_DetachItemFromObject(json,"orders");
            free_json(json);
        }
        if ( (json = cJSON_Parse(sellstr)) != 0 )
        {
            askobj = cJSON_DetachItemFromObject(json,"orders");
            free_json(json);
        }
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"bids",bidobj);
        cJSON_AddItemToObject(json,"asks",askobj);
        parse_json_orderbook(ep,maxdepth,json,0,"bids","asks","price","amount");
        free_json(json);
    }
    if ( buystr != 0 )
        free(buystr);
    if ( sellstr != 0 )
        free(sellstr);
    return(ep->orders);
}

cJSON *conv_NXT_quotejson(cJSON *json,char *fieldname,uint64_t ap_mult)
{
    //"priceNQT": "12900",
    //"asset": "4551058913252105307",
    //"order": "8128728940342496249",
    //"quantityQNT": "20000000",
    int32_t i,n;
    double price,vol,factor;
    cJSON *srcobj,*srcitem,*inner,*array = 0;
    factor = (double)ap_mult / SATOSHIDEN;
    srcobj = cJSON_GetObjectItem(json,fieldname);
    if ( srcobj != 0 )
    {
        if ( (n= cJSON_GetArraySize(srcobj)) > 0 )
        {
            array = cJSON_CreateArray();
            for (i=0; i<n; i++)
            {
                srcitem = cJSON_GetArrayItem(srcobj,i);
                price = (double)get_satoshi_obj(srcitem,"priceNQT") / ap_mult;
                vol = (double)get_satoshi_obj(srcitem,"quantityQNT") * factor;
                inner = cJSON_CreateArray();
                cJSON_AddItemToArray(inner,cJSON_CreateNumber(price));
                cJSON_AddItemToArray(inner,cJSON_CreateNumber(vol));
                cJSON_AddItemToArray(array,inner);
            }
        }
        free_json(json);
        json = array;
    }
    else free_json(json), json = 0;
    return(json);
}

struct orderbook_tx **parse_NXT(struct exchange_state *ep,int32_t maxdepth)
{
    cJSON *json,*bidobj,*askobj;
    char *buystr,*sellstr;
    if ( ep->relid != ORDERBOOK_NXTID )
    {
        printf("NXT only supports trading against NXT not %s\n",ep->rel);
        return(0);
    }
    prep_exchange_state(ep);
    if ( ep->url[0] == 0 )
        sprintf(ep->url,"%s=getBidOrders&asset=%llu&limit=%d",NXTSERVER,(long long)ep->baseid,maxdepth);
    if ( ep->url2[0] == 0 )
        sprintf(ep->url2,"%s=getAskOrders&asset=%llu&limit=%d",NXTSERVER,(long long)ep->baseid,maxdepth);
    buystr = _issue_curl(0,ep->name,ep->url);
    sellstr = _issue_curl(0,ep->name,ep->url2);
    //{"count":3,"type":"BUY","orders":[{"price":"0.00000003","amount":"137066327.96066495","total":"4.11198982"},{"price":"0.00000002","amount":"293181381.39291047","total":"5.86362760"},{"price":"0.00000001","amount":"493836943.18472463","total":"4.93836939"}]}
    if ( buystr != 0 && sellstr != 0 )
    {
        bidobj = askobj = 0;
        if ( (json = cJSON_Parse(buystr)) != 0 )
            bidobj = conv_NXT_quotejson(json,"bidOrders",ep->basemult);
        if ( (json = cJSON_Parse(sellstr)) != 0 )
            askobj = conv_NXT_quotejson(json,"askOrders",ep->basemult);
        json = cJSON_CreateObject();
        cJSON_AddItemToObject(json,"bids",bidobj);
        cJSON_AddItemToObject(json,"asks",askobj);
        parse_json_orderbook(ep,maxdepth,json,0,"bids","asks",0,0);
        free_json(json);
    }
    if ( buystr != 0 )
        free(buystr);
    if ( sellstr != 0 )
        free(sellstr);
    return(ep->orders);
}

#ifdef old
int32_t get_bitstamp_obook(int32_t *numbidsp,uint64_t *bids,int32_t *numasksp,uint64_t *asks,int32_t max)
{
    memset(bids,0,sizeof(*bids) * max);
    memset(asks,0,sizeof(*asks) * max);
    return(parse_bidasks(numbidsp,bids,numasksp,asks,max,_issue_curl(Global_iDEX->curl_handle,"bitstamp","https://www.bitstamp.net/api/order_book/")));
}

int32_t get_btce_obook(char *econtract,int32_t *numbidsp,uint64_t *bids,int32_t *numasksp,uint64_t *asks,int32_t max)
{
    char url[128];
    memset(bids,0,sizeof(*bids) * max);
    memset(asks,0,sizeof(*asks) * max);
    sprintf(url,"https://btc-e.com/api/2/%s/depth",econtract);
    return(parse_bidasks(numbidsp,bids,numasksp,asks,max,_issue_curl(Global_iDEX->curl_handle,"btce",url)));
}


int32_t get_okcoin_obook(char *econtract,int32_t *numbidsp,uint64_t *bids,int32_t *numasksp,uint64_t *asks,int32_t max)
{
    char url[128];
    memset(bids,0,sizeof(*bids) * max);
    memset(asks,0,sizeof(*asks) * max);
    sprintf(url,"https://www.okcoin.com/api/depth.do?symbol=%s",econtract);
    return(parse_bidasks(numbidsp,bids,numasksp,asks,max,_issue_curl(Global_iDEX->curl_handle,"okcoin",url)));
}

void disp_quotepairs(char *base,char *rel,char *label,uint64_t *bids,int32_t numbids,uint64_t *asks,int32_t numasks)
{
    int32_t i;
    char left[256],right[256];
    for (i=0; i<numbids&&i<numasks; i++)
    {
        left[0] = right[0] = 0;
        if ( i < numbids )
            sprintf(left,"%s%10.5f %s%9.2f | bid %s%9.3f",base,dstr(bids[(i<<1)+1]),rel,dstr(bids[(i<<1)]*((double)bids[(i<<1)+1]/SATOSHIDEN)),rel,dstr(bids[i<<1]));
        if ( i < numasks )
            sprintf(right," %s%9.3f ask | %s%9.2f %s%10.5f",rel,dstr(asks[(i<<1)]),rel,dstr(asks[(i<<1)]*((double)asks[(i<<1)+1]/SATOSHIDEN)),base,dstr(asks[(i<<1)+1]));
        printf("(%-8s.%-2d) %32s%-32s\n",label,i,left,right);
    }
}

void load_orderbooks()
{
    int numbids,numasks;
    uint64_t bids[10],asks[sizeof(bids)/sizeof(*bids)];
    //if ( get_bitstamp_obook(&numbids,bids,&numasks,asks,(sizeof(bids)/sizeof(*bids))/2) > 0 )
    //    disp_quotepairs("B","$","bitstamp",bids,numbids,asks,numasks);
    if ( get_btce_obook("btc_usd",&numbids,bids,&numasks,asks,(sizeof(bids)/sizeof(*bids))/2) > 0 )
        disp_quotepairs("B","$","btc-c",bids,numbids,asks,numasks);
    if ( get_okcoin_obook("btc_cny",&numbids,bids,&numasks,asks,(sizeof(bids)/sizeof(*bids))/2) > 0 )
        disp_quotepairs("B","Y","okcoin",bids,numbids,asks,numasks);
}
#endif

char *exchange_names[] = { "NXT", "bter", "bittrex", "cryptsy", "poloniex", "mintpal" };
#define NUM_EXCHANGES ((int32_t)(sizeof(exchange_names)/sizeof(*exchange_names)))
typedef struct orderbook_tx **(*exchange_func)(struct exchange_state *ep,int32_t maxdepth);
exchange_func exchange_funcs[NUM_EXCHANGES] =
{
    parse_NXT, parse_bter, parse_bittrex, parse_cryptsy, parse_poloniex, parse_mintpal
};

struct exchange_state **Activefiles; int32_t Numactivefiles;
double Lastmillis[NUM_EXCHANGES];

int32_t extract_baserel(char *base,char *rel,cJSON *inner)
{
    if ( cJSON_GetArraySize(inner) == 2 )
    {
        copy_cJSON(base,cJSON_GetArrayItem(inner,0));
        copy_cJSON(rel,cJSON_GetArrayItem(inner,1));
        if ( base[0] != 0 && rel[0] != 0 )
            return(0);
    }
    return(-1);
}

int32_t _search_list(uint64_t x,uint64_t *ptr,int32_t n)
{
    int32_t i;
    for (i=0; i<n; i++)
        if ( x == ptr[i] )
            return(i);
    return(-1);
}

void *poll_exchanges(void *writeflagp)
{
    struct orderbook_tx **orders;
    struct exchange_state *ep;
    struct price_data *dp;
    uint64_t *obooks;
    char name[512];
    uint32_t jdatetime,lastminute = 0;
    struct exchange_state *eps[NUM_EXCHANGES];
    int32_t i,j,n,m,iter,newminuteflag,writeflag,maxdepth = 8;
    writeflag = *(int32_t *)writeflagp;
    printf("exchange mode: writeflag.%d numactive.%d maxdepth.%d\n",writeflag,Numactivefiles,maxdepth);
    while ( Numactivefiles > 0 )
    {
        n = 0;
        obooks = calloc(Numactivefiles,sizeof(*obooks)); // maybe dynamically added
        for (i=0; i<Numactivefiles; i++)
        {
            ep = Activefiles[i];
            j = (int32_t)(ep->feedid - 0xfeed0000L);
            if ( Lastmillis[j] == 0. || milliseconds() > Lastmillis[j] + 1000 )
            {
                //printf("%.3f Last %.3f: check %s\n",milliseconds(),Lastmillis[j],ep->name);
                if ( ep->lastmilli == 0. || milliseconds() > ep->lastmilli + 10000 )
                {
                    //printf("%.3f lastmilli %.3f: %s: %s %s\n",milliseconds(),ep->lastmilli,ep->name,ep->base,ep->rel);
                    orders = (*exchange_funcs[j])(ep,maxdepth);
                    if ( ep->updated && _search_list(ep->obookid,obooks,n) < 0 )
                        obooks[n++] = ep->obookid;
                    ep->lastmilli = Lastmillis[j] = milliseconds();
                }
            }
        }
        if ( n != 0 )
        {
            void tradebot_event_processor(uint32_t jdatetime,struct price_data *dp,struct exchange_state **eps,int32_t numexchanges,uint64_t obookid,int32_t newminuteflag);
            newminuteflag = jdatetime = 0;
            for (iter=0; iter<3; iter++)
            {
                if ( iter == 1 )
                {
                    jdatetime = actual_gmt_jdatetime();
                    if ( jdatetime/60 >= lastminute+1 )
                    {
                        printf("new minute (%s) %d %d\n",jdatetime_str(jdatetime),jdatetime,jdatetime%60);
                        lastminute = jdatetime/60;
                        newminuteflag = 1;
                    }
                }
                memset(eps,0,sizeof(eps));
                for (i=0; i<n; i++)
                {
                    for (j=m=0; j<Numactivefiles; j++)
                        if ( Activefiles[j]->obookid == obooks[i] )
                            eps[m++] = Activefiles[j];
                    if ( iter == 0 )
                    {
                        if ( m != 0 )
                        {
                            //printf("have %d exchanges that quote %s %s\n",m,eps[0]->base,eps[0]->rel);
                            dp = get_price_data(obooks[i]);
                            safecopy(dp->rel,eps[0]->rel,sizeof(dp->rel));
                            safecopy(dp->base,eps[0]->base,sizeof(dp->base));
                            dp->polarity = eps[0]->polarity;
                            sprintf(name,"%s_%s",dp->base,dp->rel);
                            update_contract_pair(name,dp,Screenwidth,Screenheight,obooks[i],eps,m);
                        }
                    }
                    else if ( iter == 1 && m > 0 )
                        tradebot_event_processor(jdatetime,get_price_data(obooks[i]),eps,m,obooks[i],0);
                    else if ( iter == 2 && newminuteflag != 0 )
                        tradebot_event_processor(jdatetime,get_price_data(obooks[i]),eps,m,obooks[i],1);
                }
            }
        }
        free(obooks);
        sleep(1);
    }
    return(0);
}

int32_t init_exchanges(cJSON *confobj,int32_t exchangeflag)
{
    static int32_t pollarg;
    int32_t i,j,n;
    cJSON *array,*item;
    char base[64],rel[64];
    Numactivefiles = 0;
    pollarg = exchangeflag;
    //return(0);
    for (i=0; i<(int)(sizeof(exchange_names)/sizeof(*exchange_names)); i++)
    {
        if ( confobj == 0 )
            break;
        array = cJSON_GetObjectItem(confobj,exchange_names[i]);
        printf("array for %s %p\n",exchange_names[i],array);
        if ( array == 0 )
            continue;
        if ( (n= cJSON_GetArraySize(array)) > 0 )
        {
            for (j=0; j<n; j++)
            {
                item = cJSON_GetArrayItem(array,j);
                if ( extract_baserel(base,rel,item) == 0 )
                {
                    Activefiles = realloc(Activefiles,sizeof(*Activefiles) * (Numactivefiles+1));
                    Activefiles[Numactivefiles] = init_exchange_state(exchangeflag,exchange_names[i],base,rel,0xfeed0000L + i,ORDERBOOK_FEED,1);
                    if ( Activefiles[Numactivefiles] != 0 )
                        Numactivefiles++;
                }
            }
        }
    }
    if ( exchangeflag != 0 )
    {
        poll_exchanges(&pollarg);
        exit(0);
    }
    return(Numactivefiles);
}

#endif
