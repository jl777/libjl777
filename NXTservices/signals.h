//
//  signals.h
//  xcode
//
//  Created by jl777 on 7/29/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_signals_h
#define xcode_signals_h

#ifdef notyet
struct price_data *dp;
char name[512];
uint32_t jdatetime,m,iter,newminuteflag,lastminute = 0;
struct exchange_state *eps[NUM_EXCHANGES];
if ( 0 && n != 0 )
{
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
            //else if ( iter == 1 && m > 0 )
            //    tradebot_event_processor(jdatetime,get_price_data(obooks[i]),eps,m,obooks[i],0,changedmasks[i]);
                    else if ( iter == 2 && newminuteflag != 0 )
                        tradebot_event_processor(jdatetime,get_price_data(obooks[i]),eps,m,obooks[i],1,0);
                        }
        if ( iter == 2 && newminuteflag != 0 )
            tradebot_event_processor(jdatetime,0,0,0,0,1,1);
            }
}
#endif

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

void recalc_price_display(uint32_t ujdatetime,struct tradebot_ptrs *ptrs,char *name,struct price_data *dp,struct orderbook_tx **orders,int32_t numorders)
{
    float mintime,maxtime,*bar;
    double jdatetime;
    struct exchange_quote *qp;
    int32_t i,j,n,firstj,lastj,numsplines;
    printf("recalc %s numquotes.%d\n",name,dp->numquotes);
    //recalc_bars(ptrs,orders,numorders,dp,ujdatetime);
    //return;
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
                dp->lastprice = price = (pricesum / nonz);
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


#endif
