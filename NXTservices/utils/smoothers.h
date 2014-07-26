#ifndef H_SMOOTHERS_H
#define H_SMOOTHERS_H

/*
 *  smoother.h
 *  smallsvm
 *
 *  Created by jimbo laptop on 7/11/13.
 *  Copyright 2013 jimbosan. All rights reserved.
 *
 */

void disp_yval(int32_t color,float yval,uint32_t *bitmap,int32_t x,int32_t rowwidth,int32_t height);
void disp_dot(float radius,int32_t color,float yval,uint32_t *bitmap,int32_t x,int32_t rowwidth,int32_t height);
uint32_t scale_color(uint32_t color,float strength);

#define LEFTMARGIN 0
#define TIMEIND_PIXELS 512
#define NUM_ACTIVE_PIXELS 1024
#define MAX_ACTIVE_WIDTH (NUM_ACTIVE_PIXELS + TIMEIND_PIXELS)

#define NUM_REQFUNC_SPLINES 32
#define MAX_LOOKAHEAD 60
#define MAX_SCREENWIDTH 2048
#define MAX_AMPLITUDE 100.
#ifndef MIN
#define MIN(x,y) (((x)<=(y)) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) (((x)>=(y)) ? (x) : (y))
#endif

//#define calc_predisplinex(startweekind,clumpsize,weekind) (((weekind) - (startweekind))/(clumpsize))
#define _extrapolate_Spline(Spline,gap) ((double)(Spline[0]) + ((gap) * ((double)(Spline[1]) + ((gap) * ((double)(Spline[2]) + ((gap) * (double)(Spline[3])))))))
#define _extrapolate_Slope(Spline,gap) ((double)(Spline[1]) + ((gap) * ((double)(Spline[2]) + ((gap) * (double)(Spline[3])))))

struct madata
{
	double sum;
	double ave,slope,diff,pastanswer;
	double signchange_slope,dirchange_slope,answerchange_slope,diffchange_slope;
	double oldest,lastval,accel2,accel;
	int32_t numitems,maxitems,next,maid;
	int32_t changes,slopechanges,accelchanges,diffchanges;
	int32_t signchanges,dirchanges,answerchanges;
	char signchange,dirchange,answerchange,islogprice;
	double RTvals[20],derivs[4],derivbufs[4][12];
	struct madata **stored;
#ifdef INSIDE_OPENCL
	int32_t pad;
#endif
	double rotbuf[];
};

struct filtered_buf
{
	double coeffs[512],projden[512],emawts[512];
	double buf[256+256],projbuf[256+256],prevprojbuf[256+256],avebuf[(256+256)/16];
	double slopes[4];
	double emadiffsum,lastval,lastave,diffsum,Idiffsum,refdiffsum,RTsum;
	int32_t middlei,len;
};

int32_t Filter_middlei,Filterlen;
double Filtercoeffs[5000],Filterprojden[5000],EMAWTS15[15];
int32_t widths1[19] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
int32_t widths2[19] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
int32_t widths3[19] = { 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
int32_t widths4[19] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
int32_t widths8[19] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };
int32_t widths12[19] = { 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12 };
int32_t widths16[19] = { 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 };
int32_t RTwidths[19] = { 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7 };

int32_t smallprimes[168] =
{
	2,      3,      5,      7,     11,     13,     17,     19,     23,     29,
	31,     37,     41,     43,     47,     53,     59,     61,     67,     71,
	73,     79,     83,     89,     97,    101,    103,    107,    109,    113,
	127,    131,    137,    139,    149,    151,    157,    163,    167,    173,
	179,    181,    191,    193,    197,    199,    211,    223,    227,    229,
	233,    239,    241,    251,    257,    263,    269,    271,    277,    281,
	283,    293,    307,    311,    313,    317,    331,    337,    347,    349,
	353,    359,    367,    373,    379,    383,    389,    397,    401,    409,
	419,    421,    431,    433,    439,    443,    449,    457,    461,    463,
	467,    479,    487,    491,    499,    503,    509,    521,    523,    541,
	547,    557,    563,    569,    571,    577,    587,    593,    599,    601,
	607,    613,    617,    619,    631,    641,    643,    647,    653,    659,
	661,    673,    677,    683,    691,    701,    709,    719,    727,    733,
	739,    743,    751,    757,    761,    769,    773,    787,    797,    809,
	811,    821,    823,    827,    829,    839,    853,    857,    859,    863,
	877,    881,    883,    887,    907,    911,    919,    929,    937,    941,
	947,    953,    967,    971,    977,    983,    991,    997
};

double Display_scale;
uint32_t forex_colors[16],*Display_bitmap,*Display_bitmaps[91];
uint32_t Screenheight = 768,Screenwidth = (MAX_ACTIVE_WIDTH + 16);

double xdisp_sqrt(double x) { return((x < 0.) ? -sqrt(-x) : sqrt(x)); }
double xdisp_cbrt(double x) { return((x < 0.) ? -cbrt(-x) : cbrt(x)); }
double xdisp_log(double x) { return((x < 0.) ? -log(-(x)+1) : log((x)+1)); }
double xdisp_log10(double x) { return((x < 0.) ? -log10(-(x)+1) : log10((x)+1)); }

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

double _pairdiff(double valA,double valB)
{
	if ( valA != 0. && valB != 0. )
		return((valA - valB));
	else return(0.);
}

double calc_dpreds(double dpreds[6])
{
    double sum,net,both;
	sum = (dpreds[0] + dpreds[2]);
	net = (dpreds[1] - dpreds[3]);
	both = MAX(1,(dpreds[1] + dpreds[3]));
	//return(sum/both);
	if ( net*sum > 0 )
		return((net * fabs(sum))/(both * both));
	else return(0.);
}

double calc_dpreds_ave(double dpreds[6])
{
    double sum,both;
	sum = (dpreds[0] + dpreds[2]);
	both = MAX(1,(dpreds[1] + dpreds[3]));
	return(sum/both);
}

double calc_dpreds_abs(double dpreds[6])
{
    double both;
	both = (dpreds[1] + dpreds[3]);
	if ( both >= 1. )
		return((dpreds[0] - dpreds[2]) / both);
	else return(0);
}

double calc_dpreds_metric(double dpreds[6])
{
    double both;
	both = (dpreds[1] + dpreds[3]);
	if ( both >= 1. )
		return((dpreds[1] - dpreds[3]) / both);
	else return(0);
}

void update_dpreds(double dpreds[6],double pred)
{
	if ( pred > SMALLVAL ) dpreds[0] += pred, dpreds[1] += 1.;
	else if ( pred < -SMALLVAL ) dpreds[2] += pred, dpreds[3] += 1.;
    if ( pred != 0. )
    {
        if ( dpreds[4] == 0. || pred < dpreds[4] )
            dpreds[4] = pred;
        if ( dpreds[5] == 0. || pred > dpreds[5] )
            dpreds[5] = pred;
    }
    //printf("[%f %f %f %f] ",dpreds.x,dpreds.y,dpreds.z,dpreds.w);
}

float _xblend(float *destp,float val,float decay)
{
	float oldval;
	if ( (oldval = *destp) != 0. )
		return((oldval * decay) + ((1. - decay) * val));
	else
		return(val);
}

double _dxblend(double *destp,double val,double decay)
{
	double oldval;
	if ( (oldval = *destp) != 0. )
		return((oldval * decay) + ((1. - decay) * val));
	else
		return(val);
}

double xblend(float *destp,double val,double decay)
{
	float newval,slope;
	if ( isnan(*destp) != 0 )
		*destp = 0.;
	if ( isnan(val) != 0 )
		return(0.);
	newval = _xblend(destp,val,decay);
	if ( newval < SMALLVAL && newval > -SMALLVAL )
	{
		// non-zero marker for actual values close to or even equal to zero
		if ( newval < 0. )
			newval = -SMALLVAL;
		else
			newval = SMALLVAL;
	}
	if ( *destp != 0. && newval != 0. )
	{
		//slope = ((newval / *destp) - 1.);
		slope = (newval - *destp);
	}
	else
		slope = 0.;
	*destp = newval;
	return(slope);
}

double dxblend(double *destp,double val,double decay)
{
	double newval,slope;
    if ( val == 0. )
        return(0);
	if ( isnan(*destp) != 0 )
		*destp = 0.;
	if ( isnan(val) != 0 )
		return(0.);
	if ( *destp == 0 )
	{
		*destp = val;
		return(0);
	}
	newval = _dxblend(destp,val,decay);
	if ( newval < SMALLVAL && newval > -SMALLVAL )
	{
		// non-zero marker for actual values close to or even equal to zero
		if ( newval < 0. )
			newval = -SMALLVAL;
		else
			newval = SMALLVAL;
	}
	if ( *destp != 0. && newval != 0. )
	{
		//slope = ((newval / *destp) - 1.);
		slope = (newval - *destp);
	}
	else
		slope = 0.;
	*destp = newval;
	return(slope);
}

double conv_rawparse(double mult,double val)
{
	double logprice;
	if ( (logprice= log((val*mult)/10000.)) == 0. )
		return(SMALLVAL);
	else return(logprice);
}

double parsemultval(double parsemult,double logval)
{
	return((exp(logval) * 10000.) / parsemult);
}

double calc_logprice(double rawprice)
{
	if ( rawprice < 10. || rawprice > 60000. )
		return(0.);
	if ( rawprice == 10000. )
		return(2*SMALLVAL);
	rawprice = log(rawprice / 10000.);
	if ( fabs(rawprice) < SMALLVAL )
	{
		if ( rawprice > 0. ) return(2. * SMALLVAL);
		else if ( rawprice < 0. ) return(-2. * SMALLVAL);
		else return(0.);
	}
	return(rawprice);
}

float _bufave(float *buf,int32_t len)
{
	int32_t i,n;
	double sum;
	sum = 0.;
	n = 0;
	for (i=0; i<len; i++)
	{
		if ( fabs(buf[i]) > SMALLVAL )
		{
			n++;
			sum += buf[i];
		}
	}
	if ( n != 0 )
		sum /= n;
	if ( fabs(sum) <= SMALLVAL )
		sum = 0.;
	return(sum);
}

double _dbufabsave(double *buf,int32_t len)
{
	int32_t i,n;
	float val;
	double sum;
	sum = 0.;
	n = 0;
	for (i=0; i<len; i++)
	{
		val = fabs(buf[i]);
		if ( val > SMALLVAL )
		{
			n++;
			sum += val;
		}
	}
	if ( n != 0 )
		sum /= n;
	if ( fabs(sum) <= SMALLVAL )
		sum = 0.;
	return(sum);
}

double _dbufmagnitude(double *buf,int32_t len)
{
	int32_t i,n;
	float val;
	double sum;
	sum = 0.;
	n = 0;
	for (i=0; i<len; i++)
	{
		val = buf[i];
		if ( val != 0 )
		{
			n++;
			sum += val * val;
		}
	}
	if ( n != 0 )
		sum = sqrt(sum / n);
	if ( fabs(sum) <= SMALLVAL )
		sum = 0.;
	return(sum);
}

double _bufaved(float *buf,int32_t len)
{
	int32_t i,n;
	double sum;
	sum = 0.;
	n = 0;
	for (i=0; i<len; i++)
	{
		if ( fabs(buf[i]) > 0.0000000001 )
		{
			n++;
			sum += buf[i];
		}
	}
	if ( n != 0 )
		sum /= n;
	if ( fabs(sum) <= 0.0000000001 )
		sum = 0.;
	return(sum);
}

double _dbufave(double *buf,int32_t len)
{
	int32_t i,n;
	double sum;
	sum = 0.;
	n = 0;
	for (i=0; i<len; i++)
	{
		if ( fabs(buf[i]) > 0.0000000001 )
		{
			n++;
			sum += buf[i];
		}
	}
	if ( n != 0 )
		sum /= n;
	if ( fabs(sum) <= 0.0000000001 )
		sum = 0.;
	return(sum);
}

double _dfifoave(double *dfifo,int32_t fifomod,int32_t fifosize,int32_t clumpsize)
{
    int32_t residue,starti = fifomod - clumpsize + 1;
	if ( clumpsize >= fifosize )
		return(_dbufave(dfifo,fifosize));
	else
	{
		if ( starti >= 0 )
			return(_dbufave(&dfifo[starti],clumpsize));
		else
		{
			residue = clumpsize-fifomod;
			return((residue*_dbufave(&dfifo[fifosize-residue],residue) + fifomod*_dbufave(&dfifo[0],fifomod)) / clumpsize);
		}
	}
}

#define _dbufsqrtave(buf,len) xdisp_sqrt(_dbufave(buf,len))
#define _dbufcbrtave(buf,len) xdisp_cbrt(_dbufave(buf,len))
#define _dbuflogave(buf,len) xdisp_log(_dbufave(buf,len))
#define _dbuflog10ave(buf,len) xdisp_log10(_dbufave(buf,len))

double _oscillator_phasediff(double phase,double amplitude,double displacement)
{
	double expected_phase;
	if ( amplitude < 1. )
		amplitude = 1.;
	if ( displacement > amplitude )
		displacement = amplitude;
	else if ( displacement < -amplitude )
		displacement = -amplitude;
	expected_phase = asin(displacement / amplitude);
	//printf("p(%f/%f -> %f) ",displacement,amplitude,expected_phase);
	if ( fabs(sin(expected_phase) - displacement/amplitude) > SMALLVAL )
	{
		//	printf("sin(expected_phase %f) %f != (%f / %f) %f\n",expected_phase,sin(expected_phase),displacement,amplitude,displacement/amplitude);
	}
	return(expected_phase - phase);
}

double _oscillator_angular_freqdiff(double angular_freq,double displacement,double accel)
{
	double expected_angular_freq;
	
	//if ( accel*displacement >= 0. )
	//	return(-angular_freq);
	expected_angular_freq = xdisp_sqrt(-accel/displacement);
	return(expected_angular_freq - angular_freq);
}

double _oscillator_amplitude_from_vaf(double angular_freq,double slope,double accel)
{
	double left,right,amplitude;
	
	//if ( fabs(angular_freq) < MIN_HARMONIC_VAL || fabs(accel) < MIN_HARMONIC_VAL || fabs(slope) < MIN_HARMONIC_VAL )
	if ( angular_freq == 0. || accel == 0. || slope == 0. )
	{
		//static unsigned long debugmsg;
		//if ( debugmsg++ < 10 )
		//	printf("_oscillator_amplitude_from_vaf ret 0. (%f %f %f)\n",accel,slope,angular_freq);
		return(0.);
	}
	
	left = (accel * accel) / (angular_freq * angular_freq * angular_freq * angular_freq);
	right = (slope / angular_freq);
	amplitude = sqrt(left + right*right);
	if ( amplitude > MAX_AMPLITUDE )
		amplitude = MAX_AMPLITUDE;
	//printf("vaf %f %f %f -> amp %f\n",angular_freq,slope,accel,amplitude);
	return(amplitude);
}

/////////// other smoothing functions

double _smooth_doubles(double *dest,int32_t weekind)
{
	static double filter[8] = { 0.2905619972,1.539912853,0.3033266023,1.466074838,1.146540574,0.5808175934,0.9764347770, 1.392661532, };
    int32_t i;
	if ( weekind < 14 )
        return(0.);
    for (i=0; i<15; i++)
        if ( dest[weekind-i] == 0 )
            return(_dbufave(dest+weekind-14,15));
	return(((dest[weekind] + dest[weekind-14]) * filter[0] +
			(dest[weekind-1] + dest[weekind-13]) * filter[1] +
			(dest[weekind-2] + dest[weekind-12]) * filter[2] +
			(dest[weekind-3] + dest[weekind-11]) * filter[3] +
			(dest[weekind-4] + dest[weekind-10]) * filter[4] +
			(dest[weekind-5] + dest[weekind-9]) * filter[5] +
			(dest[weekind-6] + dest[weekind-8]) * filter[6] +
			(dest[weekind-7] * filter[7])) / 14.);
}

double _smooth_floats(float *buf,int32_t ind)
{
    int32_t i;
	static double filter[8] = { 0.2905619972,1.539912853,0.3033266023,1.466074838,1.146540574,0.5808175934,0.9764347770, 1.392661532, };
	if ( ind < 14 )
        return(0.);
    for (i=0; i<19; i++)
        if ( buf[ind-i] == 0 )
            return(_bufaved(buf+ind-18,19));
    
    return(((buf[ind] + buf[ind-14]) * filter[0] +
			(buf[ind-1] + buf[ind-13]) * filter[1] +
			(buf[ind-2] + buf[ind-12]) * filter[2] +
			(buf[ind-3] + buf[ind-11]) * filter[3] +
			(buf[ind-4] + buf[ind-10]) * filter[4] +
			(buf[ind-5] + buf[ind-9]) * filter[5] +
			(buf[ind-6] + buf[ind-8]) * filter[6] +
			(buf[ind-7] * filter[7])) / 14.);
}

double _ema15_floats(float *buf)
{
    int32_t i;
    double sum = 0.;
    for (i=0; i<15; i++)
        sum += EMAWTS15[i] * buf[-i];
    return(sum);
}

double _EMAsmooth_floats(float *buf,int32_t ind)
{
    int32_t i;
    double dbuf[15];
	//static double filter[8] = { 0.2905619972,1.539912853,0.3033266023,1.466074838,1.146540574,0.5808175934,0.9764347770, 1.392661532, };
	if ( ind < 14 )
        return(0.);
    for (i=0; i<30; i++)
        if ( buf[ind-i] == 0 )
            return(_bufaved(buf+ind-14,15));
    for (i=0; i<15; i++)
        dbuf[i] = _ema15_floats(buf+ind-i);
    return(_dbufave(dbuf,15));
    //return(((dbuf[0] + dbuf[14]) * filter[0] + (dbuf[1] + dbuf[13]) * filter[1] + (dbuf[2] + dbuf[12]) * filter[2] +
	//		(dbuf[3] + dbuf[11]) * filter[3] + (dbuf[4] + dbuf[10]) * filter[4] + (dbuf[5] + dbuf[9]) * filter[5] +
	//		(dbuf[6] + dbuf[8]) * filter[6] + (dbuf[7] * filter[7])) / 14.);
}

double _prime_smooth(float *buf,int32_t ind,int32_t mode)
{
    double dbuf[15];
    int32_t i,offset;
    if ( ind < smallprimes[14]*15 )
        return(0);
    for (i=0; i<15; i++)
    {
        if ( mode == 0 )
            offset = (i==14) ? 0 : -smallprimes[13-i]*15;
        else if ( mode == 1 )
            offset = (i==14) ? 0 : -smallprimes[13-i]-14;
        else
            offset = -14+i;
        dbuf[i] = _smooth_floats(buf,ind+offset);
        // printf("%f ",dbuf[i]);
    }
    // printf("-> %f\n",_smooth_doubles(dbuf,14));
    return(_smooth_doubles(dbuf,14));
}

double _smooth_slope(double dpreds[6],float *buf,int32_t ind)
{
	static double slopeden=393216, _slope19[19] = { -23,-264,-1281,-3152,-2700,7056,28028,45552,36894,0,-36894,-45552,-28028,-7056,2700,3152,1281,264,23 };
	double vals[19],val,slope = 0.;
	int32_t i;
    for (i=0; i<19; i++)
        vals[i] = buf[ind-i];
    for (i=1; i<18; i++)
        if ( vals[i] == 0 )
            vals[i] = _pairaved(vals[i-1],vals[i+1]);
    if ( vals[0] == 0 )
        vals[0] = _pairaved(vals[1],vals[2]);
    if ( vals[18] == 0 )
        vals[18] = _pairaved(vals[17],vals[16]);
    val = 0;
    for (i=18; i>=0; i--)
    {
        if ( vals[i] == 0 )
            vals[i] = val;
        else val = vals[i];
    }
	for (i=0; i<19; i++)
    {
        if ( vals[i] == 0 )
        {
            //printf("_smooth_slope zero at %d\n",i);
            return(0);
        }
 		slope += vals[i] * _slope19[i];
    }
    slope /= slopeden;
    update_dpreds(dpreds,slope);
	return(slope);
}

double _smooth_slopeB(float *buf,int32_t ind)
{
	static double slopedenB=131072, _slope19B[19] = { 1,16,119,544,1700,3808,6188,7072,4862,0,-4862,-7072,-6188,-3808,-1700,-544,-119,-16,-1 };
	double slope = 0.;
	int32_t i;
	for (i=0; i<19; i++)
		slope += buf[ind - i] * _slope19B[i];
	return(slope / slopedenB);
}

struct madata *init_madata(struct madata **ptrp,int32_t numitems,int32_t islogprice)
{
	static int32_t numactive_mas;
	int32_t size;
	struct madata *mp;
	//printf("init madata(%d) numactive_mas.%d\n",numitems,numactive_mas);
	numactive_mas++;
	size = (int32_t)(sizeof(struct madata) + (sizeof(mp->rotbuf[0])*numitems));
	if ( numitems == 0 )
	{
		printf("init_madata(%d) = %d\n",numitems,size);
		numitems = 1;
	}
	mp = malloc(size);//alloc_aligned_buffer(size);
	memset(mp,0,size);
	mp->islogprice = islogprice;
	mp->maxitems = numitems;
	mp->stored = ptrp;
	if ( ptrp != 0 )
		(*ptrp) = mp;
	return(mp);
}

int32_t free_madata(struct madata *mp)
{
	if ( mp == 0 )
	{
		printf("Warning: trying to free_madata(NULL)\n");
		return(-1);
	}
	if( mp->stored != 0 )
		(*mp->stored) = 0;
	free(mp);
	return(0);
}

int32_t clear_madata(struct madata *mp)
{
	//memset(&mp->RAW,0,sizeof(mp->RAW));
	mp->slope = mp->diff = mp->pastanswer = mp->sum = mp->ave = mp->oldest = mp->signchanges = 0;//mp->slopechanges = 0.;//mp->accelchanges = mp->accel2changes = 0.;
	mp->next = mp->numitems = 0;
	mp->signchange = mp->dirchange = mp->answerchange = 0;
	memset(mp->rotbuf,0,sizeof(*mp->rotbuf)*mp->maxitems);
	return(0);
}

double _update_madata(struct madata *ma,double val)	// stripped down version
{
	double lastave;
	if ( val != 0. )
	{
		ma->sum += val;
		ma->numitems++;
		ma->oldest = ma->rotbuf[ma->next];
		lastave = ma->ave;
		if ( ma->numitems >= ma->maxitems )
		{
			ma->sum -= ma->oldest;
			ma->ave = ma->sum / ma->maxitems;
			ma->pastanswer = (ma->oldest != 0 ) ? (val - ma->oldest) : 0;
		}
		else ma->ave = ma->sum / ma->numitems;
		ma->diff = (val - ma->ave);
		ma->lastval = val;
		ma->rotbuf[ma->next++] = val;
		if ( ma->next >= ma->maxitems )
			ma->next = 0;
	}
	return(ma->ave);
}

double balanced_ave(double buf[],int32_t i,int32_t width)
{
	int32_t nonz,j;
	double sum,price;
	nonz = 0;
	sum = 0.0;
	for (j=-width; j<=width; j++)
	{
		price = buf[i + j];
		if ( price != 0.0 )
		{
			sum += price;
			nonz++;
		}
	}
	if ( nonz != 0 )
		sum /= nonz;
	return(sum);
}

void buf_trioave(double dest[],double src[],int32_t n)
{
	int32_t i,j,width = 3;
	for (i=0; i<128; i++)
		src[i] = 0;
	//for (i=n-width-1; i>width; i--)
	//	dest[i] = balanced_ave(src,i,width);
	//for (i=width; i>0; i--)
	//	dest[i] = balanced_ave(src,i,i);
	for (i=1; i<width; i++)
		dest[i] = balanced_ave(src,i,i);
	for (i=width; i<1024-width; i++)
		dest[i] = balanced_ave(src,i,width);
	dest[0] = _pairaved(dest[0],dest[1] - _pairdiff(dest[2],dest[1]));
	j = width-1;
	for (i=1024-width; i<1023; i++,j--)
		dest[i] = balanced_ave(src,i,j);
	if ( dest[1021] != 0. && dest[1021] != 0. )
		dest[1023] = ((2.0 * dest[1022]) - dest[1021]);
	else dest[1023] = 0.;
}

void smooth1024(double dest[],double src[],int32_t smoothiters)
{
	double smoothbufA[1024],smoothbufB[1024];
	int32_t i;
	buf_trioave(smoothbufA,src,1024);
	for (i=0; i<smoothiters; i++)
	{
		buf_trioave(smoothbufB,smoothbufA,1024);
		buf_trioave(smoothbufA,smoothbufB,1024);
	}
	buf_trioave(dest,smoothbufA,1024);
}

double project_derivs(int32_t len,double slope,double accel,double accel2,double accel3,double accel4,double accel5)
{
	int32_t i;
	double factor = 0.99;
	for (i=0; i<len; i++)
	{
		accel *= factor; accel2 *= factor; accel3 *= factor; accel4 *= factor; accel5 *= factor;
		slope += accel; accel += accel2; accel2 += accel3; accel3 += accel4; accel4 += accel5;
	}
	return(slope);
}

double copy_float1024(double *dbuf,float *src)
{
	int32_t i;
	double nonz = 0.;
	for (i=0; i<1024; i++)
		if ( (dbuf[i] = src[i]) != 0. )
			nonz = dbuf[i];
	return(nonz);
}

double extract_float1024(float *dest,double *dsrc,int32_t firstweekind,int32_t weekind)
{
	int32_t i;
	double nonz = 0.;
	for (i=0; i<1024; i++)
	{
		if ( weekind-1024+i >= firstweekind )
		{
			if ( (dest[i]= dsrc[i]) != 0. )
				nonz = dsrc[i];
		}
		else dest[i] = 0.;
	}
	return(nonz);
}

double extract1024(double *dest,double *dsrc,int32_t firstweekind,int32_t weekind)
{
	int32_t i;
	double nonz = 0.;
	for (i=0; i<1024; i++)
	{
		if ( weekind-1024+i >= firstweekind )
		{
			if ( (dest[i]= dsrc[i]) != 0. )
				nonz = dsrc[i];
		}
		else dest[i] = 0.;
	}
	return(nonz);
}

double calc_slopes1024(double *slopes,double *dsrc)
{
	int32_t i;
	double nonz = 0.;
	for (i=1; i<1024; i++)
		if ( (slopes[i]= _pairdiff(dsrc[i],dsrc[i-1])) != 0. )
			nonz = dsrc[i];
	slopes[0] = slopes[1] - (_pairdiff(slopes[2],slopes[1]));
	return(nonz);
}


//////////////// splines


void pingpong_smoother(int32_t numprimes,double *dest,double *src,int32_t maxwidth)
{
	double *tmp,*origdest;
	
	int32_t p,x,n,addlimit,sublimit;
	double price,prev,next,wt,sum;
	origdest = dest;
	p = 0;
	while ( p <= numprimes )
	{
		if ( p == numprimes )
		{
			if ( src != origdest )
				n = 1;
			else break;
		}
		else n = smallprimes[p++];
		addlimit = maxwidth - n - 1;
		sublimit = n + 1;
		for (x=0; x<maxwidth; x++)
		{
			sum = wt = 0.;
			if ( x < addlimit )
			{
				next = src[x+n];
				if ( fabs(next) > SMALLVAL )
					wt += 1., sum += next;
			}
			if ( x >= sublimit )
			{
				prev = src[x-n];
				if ( fabs(prev) > SMALLVAL )
					wt += 1., sum += prev;
			}
			price = src[x];
			if ( fabs(price) > SMALLVAL )
				wt += 1., sum += price;
			dest[x] = (wt > 0.) ? (sum / wt) : 0.;
			//if ( dest[x] != 0. && x >= firstx )
			//	printf("(%d %.20f %f %f %f %f %f) ",x,dest[x],next,prev,price,sum,wt);
		}
		tmp = src;
		src = dest;
		dest = tmp;
	}
}

int32_t calc_spline(double *aveabsp,double *outputs,double *slopes,double dSplines[][4],int32_t firstx,double *weekinds,double *splinevals,int32_t clumpsize,int32_t len)
{
	double c[NUM_REQFUNC_SPLINES],f[NUM_REQFUNC_SPLINES],dd[NUM_REQFUNC_SPLINES],dl[NUM_REQFUNC_SPLINES],du[NUM_REQFUNC_SPLINES],gaps[NUM_REQFUNC_SPLINES];
	int32_t n,i,lasti,x,numsplines,nonz;
	double gap,sum,dweekind,yval,abssum,lastval,lastweekind,incr = clumpsize;
	double vx,vy,vw,vz,slope = 0;
	double dSpline[4];
	n = lasti = nonz = 0;
	sum = 0.;
	lastweekind = 0.;
	for (i=0; i<NUM_REQFUNC_SPLINES; i++)
	{
		if ( (f[n]= splinevals[i]) != 0. && weekinds[i] != 0 )
		{
			//printf("i%d.(%f %f) ",i,weekinds[i],splinevals[i]);
			if ( n > 0 )
			{
				if ( (gaps[n-1] = weekinds[i] - lastweekind) < 0 )
				{
					printf("illegal gap w%f to w%f\n",lastweekind,weekinds[i]);
					return(0);
				}
			}
			lastweekind = weekinds[i];
			weekinds[n] = weekinds[i];
			n++;
		}
	}
	//printf("numsplines.%d\n",n);
	numsplines = n;
	if ( n < 4 )
		return(0);
	//startweekind = endweekind - len*clumpsize;
	//for (i=0; i<n; i++) printf("%f ",f[i]);
	//printf("F[%d]\n",n);
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
	//x = MIN(len-1,(int)calc_predisplinex(startweekind,clumpsize,weekinds[0]));
	//for (i=0; i<=x; i++)
	//	outputs[i] = slopes[i] = 0.;
	abssum = 0.;
	nonz = 0;
	lastval = 0;
	for (i=0; i<n-1; i++)
	{
		gap = gaps[i];
		vx = f[i];
		vz = c[i];
		vw = (c[i+1] - c[i]) / (3. * gap);
		vy = ((f[i+1] - f[i]) / gap) - (gap * (c[i+1] + 2.*c[i]) / 3.);
		//vSplines[i] = (float4){ (float)vx, (float)vy, (float)vz, (float)vw };
		dSpline[0] = vx, dSpline[1] = vy, dSpline[2] = vz, dSpline[3] = vw; //= (double4){ vx, vy, vz, vw };
		//printf("%3d: w%8.1f [%9.6f %9.6f %9.6f %9.6f]\n",i,weekinds[i],(vx),vy*1000,vz*1000*1000,vw*1000*1000*1000);
		memcpy(dSplines[i],dSpline,sizeof(dSpline));
		gap = clumpsize;//MAX(1.,(incr / 3.));
		dweekind = weekinds[i] + gap;
		lastval = dSpline[0];
		while ( 1 )
		{
			x = firstx + ((dweekind - weekinds[0]) / clumpsize);
			//x = (int)calc_predisplinex(startweekind,clumpsize,dweekind);
			//x = len-1 + ((dweekind - endweekind) / clumpsize);
			if ( x > len-1 ) x = len-1;
			if ( x < 0 ) x = 0;
			if ( (i < n-2 && gap > gaps[i]+clumpsize) || (i == n-2 && dweekind > weekinds[n-1]+MAX_LOOKAHEAD*clumpsize) )
				break;
			if ( x >= 0 )
			{
				yval = _extrapolate_Spline(dSpline,gap);
				if ( yval > 300. ) yval = 300.;
				else if ( yval < -300. ) yval = -300.;
				if ( isnan(yval) == 0 )	// not a nan
				{
					if ( 1 && fabs(lastval) > SMALLVAL )
					{
						if ( lastval != 0 && yval != 0 )
						{
							slope = (yval - lastval);
							abssum += fabs(slope);
							nonz++;
						}
					}
				}
				if ( outputs != 0 )
					outputs[x] = yval;
				if ( slopes != 0 )
					slopes[x] = slope;//(gap <= clumpsize) ? 0 : slope;
				//printf("x.%-4d i%-4d: gap %9.6f %9.6f last %9.6f slope %9.6f | %9.1f [%9.1f %9.6f %9.6f %9.6f %9.6f]\n",x,i,gap,yval,lastval,slopes[x],dweekind,weekinds[i+1],dSpline.x,dSpline.y*derivscales[0],dSpline.z*derivscales[1],dSpline.w*derivscales[2]);
				lastval = yval;
            }
			gap += incr;
			dweekind += incr;
		}
		double pred = (i>0) ? _extrapolate_Spline(dSplines[i-1],gaps[i-1]) : 0.;
		printf("%2d: w%8.1f [gap %f -> %9.6f | %9.6f %9.6f %9.6f %9.6f %9.6f]\n",i,weekinds[i],gap,pred,f[i],dSplines[i][0],1000000*dSplines[i][1],1000000*1000000*dSplines[i][2],1000000*1000000*1000*dSplines[i][3]);
	}
	if ( 0 && slopes != 0 )
	{
		for (x=firstx; x<len-1; x++)
		{
			dxblend(&slopes[x],(outputs[x+1] - outputs[x]),.9);
			abssum += fabs(slopes[x]);
			nonz++;
		}
	}
	if ( nonz != 0 )
		abssum /= nonz;
	*aveabsp = abssum;
	return(numsplines);
}

int32_t calc_splinevals(int32_t numprimes,float *projbuf,float *srcbuf,int32_t *RTip,int32_t *firstxp,int32_t lastweekind,double splinevals[NUM_REQFUNC_SPLINES],double weekinds[NUM_REQFUNC_SPLINES],int32_t clumpsize,double halfterminator,double terminator)
{
	double src[MAX_SCREENWIDTH],dest[MAX_SCREENWIDTH];
	double val;
	int32_t i,w,x,n,firstx,incr,firstweekind;
	//printf("calc_splinevals %d %p %p\n",firstweekind,projbuf,srcbuf);
	if ( projbuf == 0 || srcbuf == 0 )
		return(0);
	*firstxp = *RTip = -1;
	firstweekind = lastweekind - NUM_ACTIVE_PIXELS*clumpsize;
	if ( firstweekind < 0 )
	{
		printf("calc_splinevals: negative index.%d\n",firstweekind);
		return(0);
	}
	memset(src,0,sizeof(src));
	for (i=0; i<NUM_ACTIVE_PIXELS; i++)
	{
		if ( lastweekind < i*clumpsize )
			break;
		if ( (val = _bufaved(&srcbuf[lastweekind - i*clumpsize],clumpsize)) == 0 )
			val = _bufaved(&projbuf[lastweekind - i*clumpsize],clumpsize);
		src[NUM_ACTIVE_PIXELS - i] = val;
	}
	memcpy(dest,src,sizeof(dest));
	pingpong_smoother(numprimes,dest,src,NUM_ACTIVE_PIXELS);
	memset(splinevals,0,sizeof(*splinevals) * NUM_REQFUNC_SPLINES);
	memset(weekinds,0,sizeof(*weekinds) * NUM_REQFUNC_SPLINES);
	incr = (NUM_ACTIVE_PIXELS / (NUM_REQFUNC_SPLINES-1));
	firstx = -1;
	for (i=n=0; i<NUM_REQFUNC_SPLINES; i++)
	{
		x = (i * incr) + (incr / 2);
		w = firstweekind + (x * clumpsize) - clumpsize/2;
		if ( w < 0 )
			continue;
		if ( firstweekind < 0 )
			firstweekind = w;
		if ( (splinevals[n] = _dbufave(&dest[i*incr],incr)) != 0 )
		{
			//printf("i%d.(x%d w%d %f) ",i,x,w,exp(splinevals[n]));
			weekinds[n++] = w;
			dxblend(&terminator,splinevals[n],.9);
			if ( firstx < 0 )
				firstx = x;
		}//else printf("%f ",splinevals[n]);
	}
	*firstxp = firstx;
	if ( n != 0 )
	{
        *RTip = n-1;
		/*splinevals[n] = terminator;
         weekinds[n++] = w + SVMcontamination*10;
         splinevals[n] = terminator;
         weekinds[n++] = w + SVMcontamination*20;
         splinevals[n] = terminator;
         weekinds[n++] = w + SVMcontamination*30;*/
	}
	if ( *RTip > 0 )
	{
		//printf("i%d.(x%d w%d %f) ",i,x,(int)(weekinds[n-1] + NNcontamination),splinevals[n-1]);
		//weekinds[n] = weekinds[n-1] + NNcontamination; splinevals[n] = (terminator!=0)?terminator:splinevals[n-1]; n++;
	}
	//printf("n.%d\n",n);
	/*if ( terminator != 0. )
	 weekinds[n] = firstweekind + NUM_ACTIVE_PIXELS*clumpsize - NNcontamination/2, splinevals[n] = terminator, n++;
	 else
	 {
	 terminator = _bufaved(&srcbuf[firstweekind + NUM_ACTIVE_PIXELS*clumpsize - NNcontamination-2],3);
	 weekinds[n] = firstweekind + NUM_ACTIVE_PIXELS*clumpsize, splinevals[n] = terminator, n++;
	 for (i=2; i<8; i++)
	 {
	 terminator *= .9;
	 weekinds[n] = weekinds[n-1] + CONTAMINATION, splinevals[n] = terminator, n++;
	 }
	 }*/
	//printf("n.%d [%f %f %f] firstx.%d\n",n,svm->prevpastanswer,halfterminator,terminator,firstx);
	//for (i=0; i<n; i++)
	//	splinevals[i] *= 10000.;
	return(n);
}

int32_t disp_RTspline(int32_t numprimes,int32_t numslopeprimes,float *srcbuf,double *lastvalp,double outputs[MAX_SCREENWIDTH],double slopes[MAX_SCREENWIDTH],double accels[MAX_SCREENWIDTH],int32_t lastweekind,int32_t clumpsize)
{
	int32_t yoffset = 0,rowwidth = 0,height = 0;
	uint32_t *bitmap = 0;
	float *projbuf = srcbuf;
	double scale=1.,halfterminator=0;
	int32_t RTi=-1,firstx = -1,outcolor=0;
	double slopeabs,weekinds[NUM_REQFUNC_SPLINES],splinevals[NUM_REQFUNC_SPLINES];
	double dSplines[NUM_REQFUNC_SPLINES][4];
	double lastslope,accel,lastaccel,val;//,dispweekave = Dispweekaves[svm->refc];
	int32_t i,n=-1,y,color,colorscale,firstweekind;
	firstweekind = lastweekind - NUM_ACTIVE_PIXELS*clumpsize;
	*lastvalp = 0.;
	//printf("disp spline %d to %d %p %p\n",firstweekind,lastweekind,srcbuf,projbuf);
	if ( srcbuf != 0 && projbuf != 0 )
	{
		if ( outputs != 0 )
			memset(outputs,0,sizeof(*outputs) * Screenwidth);
		if ( slopes != 0 )
			memset(slopes,0,sizeof(*slopes) * Screenwidth);
		if ( accels != 0 )
			memset(accels,0,sizeof(*accels) * Screenwidth);
		if ( (n= calc_splinevals(numprimes,projbuf,srcbuf,&RTi,&firstx,lastweekind,splinevals,weekinds,clumpsize,halfterminator,*lastvalp)) > 0 )
		{
			//printf("calc_splinevals.n%d\n",n);
			firstx = (weekinds[0] - firstweekind) / clumpsize;
			color = 0xffff00;
			y = yoffset;
			if ( bitmap != 0 && outcolor == 0x0088ff )
			{
				colorscale = 50;
				for (i=0; i<n; i++)
				{
					//if ( weekinds[i] > svm->pastweekind )
					color = (splinevals[i] > 0) ? 0x00ffff : 0xff2200;
					val = splinevals[i];//2*_calc_pricey(splinevals[i],dispweekave);
					//printf("(dot%d %f/%f %f) ",i,splinevals[i],dispweekave,val);
					if ( (weekinds[i]-firstweekind)/clumpsize <= NUM_ACTIVE_PIXELS )
						disp_dot(4,color,y/Display_scale + scale*val,bitmap,(weekinds[i]-firstweekind)/clumpsize,rowwidth,height);
				}
			} else colorscale = 100;
			if ( n >= 4 && (n= calc_spline(&slopeabs,outputs,slopes,dSplines,firstx,weekinds,splinevals,clumpsize,NUM_ACTIVE_PIXELS)) >= 4 )
			{
				*lastvalp = outputs[NUM_ACTIVE_PIXELS-1];
				if ( slopes != 0 )
				{
					double tmpbuf[MAX_ACTIVE_WIDTH];
					memcpy(tmpbuf,slopes,MAX_ACTIVE_WIDTH*sizeof(*slopes));
					pingpong_smoother(numslopeprimes,slopes,tmpbuf,MAX_ACTIVE_WIDTH);
				}
				if ( accels != 0 || bitmap != 0 )
				{
					if ( slopeabs == 0 )
						slopeabs = 1.;
					lastslope = lastaccel = accel = 0.;
					for (i=firstx; i<=NUM_ACTIVE_PIXELS; i++)
					{
						if ( bitmap != 0 && outputs != 0 && outputs[i] != 0 )
						{
							disp_yval(scale_color(outcolor==0x0088ff?((outputs[i] > 0) ? 0x00ffff : 0xff2200):outcolor,100),y/Display_scale + scale*outputs[i],bitmap,i,rowwidth,height);
							if (  outcolor == 0x0088ff )
								disp_yval(scale_color(outcolor==0x0088ff?((outputs[i] > 0) ? 0x00ffff : 0xff2200):outcolor,100),y/Display_scale + scale*outputs[i],bitmap,i,rowwidth,height);
						}
						if ( slopes != 0 )
						{
							val = (50. * slopes[i]) / slopeabs;
							//if ( bitmap != 0 )
							//	disp_yval(scale_color(outcolor == 0x0088ff?0xff2200:scale_color(outcolor,50),colorscale),y/Display_scale + scale*val,bitmap,i,rowwidth,height);
							if ( slopes[i] != 0 )
							{
								if ( lastslope != 0 )
								{
									accel = (slopes[i] - lastslope);
									//if ( bitmap != 0 && outcolor == 0x0088ff )
									//	disp_yval(scale_color(0xff00ff,colorscale),y/Display_scale + scale*accel * (1000. / slopeabs),bitmap,i,rowwidth,height);
									if ( accels != 0 )
										accels[i] = accel;
									lastaccel = accel;
								}
								lastslope = slopes[i];
							}
							//else lastslope = lastaccel = 0.;
						}
					}
				}
			}
		}
	} else printf("no buffers for spline\n");
	return(RTi);
}

//////////////// filtered bufs

double init_emawts(double *emawts,int32_t len)
{
	int32_t j;
	double smoothfactor,remainder,sum,sum2,val;
	remainder = 1.;
	smoothfactor = exp(1.) / (.5 * ((double)len-1.));
	sum = 0.;
	for (j=0; j<len; j++)
	{
		val = remainder * smoothfactor;
		sum += val;
		//printf("%9.6f ",val);
		remainder *= (1. - smoothfactor);
		//emawts[len-1-j] = val;
		emawts[j] = val;
	}
	//printf("-> emasum %f\n",sum);
	if ( fabs(1. - sum) > .00000000000000001 )
	{
		sum2 = 0.;
		for (j=0; j<len; j++)
		{
			emawts[j] /= sum;
			//printf("%11.7f ",emaWts[j].x);
			sum2 += emawts[j];
		}
		sum = 0.;
		for (j=0; j<len; j++)
		{
			emawts[j] /= sum2;
			//printf("%11.7f ",emaWts[j].x);
			sum += emawts[j];
		}
		//printf("-> emasum2.%d %.40f %.40f\n",len,sum2,sum);
	} else sum2 = sum;
	return(sum2);
}

int32_t calc_smooth_code2(double _coeffs[5000],double middle,double *wts,int32_t count,int32_t smoothwidth,int32_t _maxprimes)
{
	static double coeffs[60][10000],smoothbuf[10000];
	int32_t i,x,p,prime,numprimes,middlei = -1;
	double sum;
	memset(coeffs,0,sizeof(coeffs));
	memset(smoothbuf,0,sizeof(smoothbuf));
	_maxprimes = MIN((int)(sizeof(coeffs)/(sizeof(double)*10000))-1,_maxprimes);
	smoothwidth = MIN(5000,smoothwidth);
	x = 5000;
	coeffs[0][x] = middle;
	for (i=0; i<count; i++)
	{
		coeffs[0][x + i + 1] = wts[i];
		coeffs[0][x - i - 1] = wts[i];
		printf("%f ",wts[i]);
	}
	printf("wts | middle %f\n",middle);
	for (numprimes=_maxprimes; numprimes>=3; numprimes--)
	{
		for (p=1; p<numprimes; p++)
		{
			memcpy(coeffs[p],coeffs[p-1],sizeof(coeffs[p]));
			prime = smallprimes[p-1];
			for (x=0; x<10000; x++)
			{
				coeffs[p][x] += (coeffs[p-1][x] * middle);
				for (i=0; i<count; i++)
				{
					coeffs[p][x] += (coeffs[p-1][x + (prime * (i + 1))] * wts[i]);
					coeffs[p][x] += (coeffs[p-1][x - (prime * (i + 1))] * wts[i]);
				}
			}
		}
		memcpy(smoothbuf,coeffs[numprimes-1],sizeof(smoothbuf));
		memset(coeffs,0,sizeof(coeffs));
		sum = 0.;
		for (x=0; x<10000; x++)
		{
			if ( smoothbuf[x] != 0. )
			{
				sum += smoothbuf[x];
				//printf("(%d %f) ",x-5000,smoothbuf[x]);
			}
		}
		//printf("maxprimes.%d\n",maxprimes);
		for (x=0; x<10000; x++)
			coeffs[0][x] = (smoothbuf[x] / sum);
	}
	sum = 0.;
	for (x=0; x<10000; x++)
		sum += smoothbuf[x];
	memset(_coeffs,0,sizeof(*_coeffs)*5000);
	if ( sum != 0. )
	{
		int32_t last=0,first = -1;
		printf("double Smooth_coeffs[%d] =	// numprimes.%d\n{\n",smoothwidth,_maxprimes);
		for (x=0; x<10000; x++)
		{
			if ( smoothbuf[x] != 0. )
			{
				if ( first < 0 )
					first = x;
				smoothbuf[x] = (1000000. * 1000000. * smoothbuf[x]) / sum;
				_coeffs[x-first] = smoothbuf[x];
				if ( x == 5000 )
				{
					middlei = x-first;
				}
				last = x-first;
				//printf("(%d %f) ",x-5000,smoothbuf[x]);
			}
		}
		printf("middle at %d, last %d\n",middlei,last);
		/*_coeffs[0] = smoothbuf[5000];
		 for (x=1; x<smoothwidth; x++)
		 {
		 if ( fabs(smoothbuf[5000 - x] - smoothbuf[5000 + x]) > SMALLVAL )
		 printf("x.%d error %.20f != %.20f [%.20f]\n",x,smoothbuf[5000 - x],smoothbuf[5000 + x],smoothbuf[5000 - x] - smoothbuf[5000 + x]);
		 _coeffs[x] = (smoothbuf[5000 - x] + smoothbuf[5000 + x]) / 2.;
		 }*/
		sum = 0;
		for (x=0; x<smoothwidth; x++)
			sum += _coeffs[x];
		if ( sum != 0. )
		{
			double newsum;
			while ( fabs(sum - 1.) > .0000000000000000000000000001 )
			{
				newsum = 0;
				for (x=0; x<smoothwidth; x++)
				{
					_coeffs[x] /= sum;
					newsum += _coeffs[x];
				}
				sum = 0.;
				for (x=0; x<smoothwidth; x++)
				{
					_coeffs[x] /= newsum;
					sum += _coeffs[x];
				}
				printf("sum %.40f newsum %.40f\n",sum,newsum);
			}
			
			for (x=0; x<smoothwidth; x++)
			{break;
				printf("%.20f, ",_coeffs[x]);
				if ( (x%9) == 8 )
					printf("// x.%d\n",x);
			}
		}
	}
	printf("\n};	// sum %.40f\n",sum);
	return(middlei);
	//printf("_Constants size %d\n",(int)__constant_size);
}

double dSum16(double *dTmp)
{
    int32_t i;
    double sum = 0.;
    for (i=0; i<16; i++)
        sum += dTmp[i];
    return(sum);
}

void init_filtered_bufs()
{
	int32_t _maxprimes = 5;
	int32_t i,j,lasti,smoothwidth = 512;
	double den,ave,wts[7];//,_coeffs[5000],_projden[5000];
	double dTmp[16] = { 0.2905619972,1.539912853,0.3033266023,1.466074838,1.146540574,0.5808175934,0.9764347770,1.392661532, 0.9764347770,0.5808175934,1.146540574,1.466074838,0.3033266023,1.539912853,0.2905619972,0. };
	for (j=0; j<4; j++)
    {
        ave = dSum16(dTmp);
        for (i=0; i<16; i++)
        {
            if ( j == 0 )
                dTmp[i] /= ave;
            else dTmp[i] = (.5 * dTmp[i] + .5 * dTmp[i]/ave);
        }
        printf("dTmp sum %.20f -> %.20f\n",ave,dSum16(dTmp));
    }
	wts[0] = dTmp[8]; wts[1] = dTmp[9]; wts[2] = dTmp[0xa]; wts[3] = dTmp[0xb]; wts[4] = dTmp[0xc]; wts[5] = dTmp[0xd]; wts[6] = dTmp[0xe];
	Filter_middlei = calc_smooth_code2(Filtercoeffs,dTmp[7],wts,7,smoothwidth,_maxprimes);
	memset(Filterprojden,0,sizeof(Filterprojden));
	den = 0;
	lasti = Filter_middlei*2;
	Filterlen = Filter_middlei*2+1;
	for (i=lasti; i>=0; i--)
	{
		den += Filtercoeffs[i];
		Filterprojden[i] = 1. / den;
	}
	if ( 0 )
	{
		printf("double Filtercoeffs[] = {\n");
		for (i=0; i<=lasti; i++)
			printf("%.20f, ",Filtercoeffs[i]);
		printf("\n};\n");
		printf("double Filterprojden[] = {\n");
		for (i=0; i<=lasti; i++)
			printf("%.20f, ",Filterprojden[i]);
		printf("\n};	// lasti.%d\n",lasti);
	}
    init_emawts(EMAWTS15,15);
}

void _init_filtered_buf(struct filtered_buf *fb,int32_t middlei,int32_t len,double *coeffs,double *projden)
{
	memset(fb,0,sizeof(*fb));
	fb->middlei = middlei;
	fb->len = len;
	memcpy(fb->coeffs,coeffs,sizeof(*fb->coeffs)*len);
	memcpy(fb->projden,projden,sizeof(*fb->projden)*len);
	init_emawts(fb->emawts,middlei+1);
}

void init_filtered_buf(struct filtered_buf *fb)
{
	_init_filtered_buf(fb,Filter_middlei,Filterlen,Filtercoeffs,Filterprojden);
}

double _calc_fb_widthsums(double *RTbuf,struct filtered_buf *fb,int32_t *widths)
{
	int32_t i,RTnonzs;
	double *ptr,RTsums;
	ptr = fb->projbuf + 256 + fb->middlei;
	RTsums = 0;
	RTnonzs = 0;
	for (i=0; i<19; i++)
	{
		ptr -= widths[i];
		if ( (RTbuf[i] = _dbufave(ptr,widths[i])) != 0 )
		{
			RTsums += RTbuf[i];
			RTnonzs++;
		}
	}
	if ( RTnonzs != 0 )
		RTsums /= RTnonzs;
	return(RTsums);
}

double _update_filtered_buf(struct filtered_buf *fb,double newval,int32_t *widths)
{
	//static int32_t widths[19] = { 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1 };
	//static int32_t widths[19] = { 4, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1 };
	//static int32_t widths[19] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 6, 5, 4, 3, 2, 1 };
	static double slopeden=393216, _slope19[19] = { -23,-264,-1281,-3152,-2700,7056,28028,45552,36894,0,-36894,-45552,-28028,-7056,2700,3152,1281,264,23 };
	static double slopedenB=131072, _slope19B[19] = { 1,16,119,544,1700,3808,6188,7072,4862,0,-4862,-7072,-6188,-3808,-1700,-544,-119,-16,-1 };
	static double RTslopeden=16384, _RTslope[16] = { 1,13,77,273,637,1001,1001,429,-429,-1001,-1001,-637,-273,-77,-13,-1 };
	static double RTslopedenB=2856, _RTslopeB[16] = { 322,217,110,35,-42,-87,-134,-149,-166,-151,-138,-93,-50,25,98,203 };
	int32_t dispflag = 0;
	double RTsums,sum,sum2,ave,den,den2,wt,diffsum,proj,refdiffsum,refval,val,slope19,slope19B,RTslope,RTslopeB,RTbuf[19];
	int32_t i,nonz,n,refn,aven,firsti = 255 - fb->middlei;
	nonz = n = 0;
	sum = sum2 = den = den2 = diffsum = refdiffsum = ave = slope19 = slope19B = RTslope = RTslopeB = 0.;
	refval = fb->buf[256];
	for (i=aven=refn=0; i<256+MIN(fb->middlei+1,255); i++)
	{
		fb->buf[i] = fb->buf[i+1];
		fb->prevprojbuf[i] = fb->projbuf[i];
		if ( i >= firsti && i < firsti+fb->len )
		{
			fb->buf[i] += fb->coeffs[i-firsti] * newval;
			if ( (proj = fb->buf[i] * fb->projden[i-firsti]) != 0 )
			{
				if ( i-firsti >= fb->middlei )
				{
					wt = 1./(i-firsti-fb->middlei+1);
					nonz++, den += wt, sum += wt * proj;
					if ( proj != 0. && fb->projbuf[i] != 0. )
					{
						n++, diffsum += (proj - fb->projbuf[i]);
						wt = fb->emawts[(i-firsti) - fb->middlei];
						den2 += wt, sum2 += wt * (proj - fb->projbuf[i]);
					}
				}
				if ( refval != 0 )
				{
					refn++;
					if ( i > 256 )
						refdiffsum += (proj - refval);
					else refdiffsum -= (proj - refval);
				}
				fb->projbuf[i] = proj;
			}
		}
		else fb->projbuf[i] = fb->buf[i];
		if ( fb->projbuf[i] != 0. )
			aven++, ave += fb->projbuf[i];
		if ( (i & 0xf) == 0xf )
		{
			if ( aven != 0 )
				ave /= aven;
			fb->avebuf[i >> 4] = ave;
			aven = 0;
			ave = 0.;
		}
	}
	RTsums = _calc_fb_widthsums(RTbuf,fb,widths);
	for (i=0; i<19; i++)
	{
		val = RTbuf[i];
		slope19 += _slope19[i] * val;
		slope19B += _slope19B[i] * val;
		if ( i < 16 )
		{
			RTslope += _RTslope[i] * val;
			RTslopeB += _RTslopeB[i] * val;
		}
	}
	if ( aven != 0 )
	{
		ave /= aven;
		fb->avebuf[i >> 4] = ave;
	}
	if ( n != 0 )
		diffsum /= n;
	if ( den2 != 0 )
		sum2 /= den2;
	if ( nonz != 0 )
		sum /= den;
	if ( refn != 0 )
		refdiffsum /= refn;
	fb->refdiffsum = refdiffsum;
	fb->diffsum = diffsum;
	fb->emadiffsum = sum2;
	fb->Idiffsum += diffsum;
	fb->lastave = sum;
	if ( slopeden != 0 )
		slope19 /= slopeden;
	if ( slopedenB != 0 )
		slope19B /= slopedenB;
	if ( RTslopeden != 0 )
		RTslope /= RTslopeden;
	if ( RTslopedenB != 0 )
		RTslopeB /= RTslopedenB;
	fb->slopes[0] = slope19; fb->slopes[1] = slope19B; fb->slopes[2] = RTslope; fb->slopes[3] = RTslopeB;
	if ( dispflag != 0 || (rand() % 10000) == 0 )
	{
		for (i=0; i<(256+256); i+=16)
		{
			if ( dispflag > 1 )
			{
				if ( i == 256 )
					printf("| ");
				if ( i >= 256-16 )
					printf("%9.6f ",fb->avebuf[i/16]);
			}
		}
		if ( dispflag > 1 )
			printf("emasum %f diffsum %f Idiffsum %f refdiffsum %f\n",sum,diffsum,fb->Idiffsum,refdiffsum);
	}
	fb->lastval = newval;
	return(RTsums);
}

double update_filtered_buf(struct filtered_buf *fb,double newval,int32_t *widths)
{
	int32_t i;
	if ( newval != 0. )
	{
		if ( fb->lastval == 0. )
		{
			for (i=0; i<255; i++)
				_update_filtered_buf(fb,newval,widths);
		}
        fb->RTsum = _update_filtered_buf(fb,newval,widths);
	}
    return(fb->RTsum);
}

#endif
