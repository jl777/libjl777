//
//  sorts.h
//  xcode
//
//  Created by jl777 on 7/25/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_jdatetime_h
#define xcode_jdatetime_h

//#include <sys/time.h>
#ifndef BUFSIZE
#define BUFSIZE 512
#endif
#define FIRST_YEAR 2013

#define FALSE 0
#define TRUE 1
#define JDATE_SUNDAY 1
#define SECONDS_IN_MINUTE (60)
#define SECONDS_IN_DAY (24*60*60)
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_WEEK (24*3600*7)
int32_t START_DST[30],END_DST[30],TIMEZONE_ADJUST,ACTUAL_GMT_JDATETIME;
int32_t ENDJDATETIME,FIRSTJDATETIME;
char *dayofweek[7] = { "Sat", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri" };	// jdate 0 == 1/1/2000
char *months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

char *jdate_fname(int32_t jdate);
char *jdatetime_str(int32_t jdatetime);
char *jdatetime_str2(int32_t jdatetime);

//*
//* Mark R. Showalter, PDS Rings Node, December 1995
//* Revised by MRS 6/98 to conform to RingLib naming conventions.

#define  JD_OF_J2000	2451545		    /* Julian Date of noon, J2000 */
typedef int		RL_INT4;
typedef int		RL_BOOL;
static RL_INT4 gregorian_dutc  = -152384;   /* Default is October 15, 1582 */
static RL_INT4 gregorian_year  = 1582;
static RL_INT4 gregorian_month = 10;
static RL_INT4 gregorian_day   = 15;

void Jul_FixYM(year,month)
RL_INT4	*year, *month;
{
	RL_INT4 y,m,dyear;
	y = *year;
	m = *month;
	/* Shift month into range 1-12 */
	dyear = (m-1) / 12;
	m -= 12 * dyear;
	y += dyear;
	if (m < 1) {
		m += 12;
		y -= 1;
	}
	*year  = y;
	*month = m;
}

RL_INT4 Jul_JDNofGregYMD(year,month,day)
RL_INT4 year, month, day;
{
	RL_INT4 temp;
	/*	temp = (month - 14)/12; */
	temp = (month <= 2 ? -1:0);
	return   (1461*(year + 4800 + temp))/4
	+ (367*(month - 2 - 12*temp))/12
	- (3*((year + 4900 + temp)/100))/4 + day - 32075;
}

RL_INT4 Jul_JDNofJulYMD(year,month,day)
RL_INT4 year,month,day;
{
	return(367*year
	       - (7*(year + 5001 + (month-9)/7))/4
	       + (275*month)/9
	       + day + 1729777);
}

void Jul_GregYMDofJDN(jd,year,month,day)
RL_INT4 jd,*year,*month,*day;
{
	RL_INT4 l,n,i,j,d,m,y;
	l = jd + 68569;
	n = (4*l) / 146097;
	l = l - (146097*n + 3)/4;
	i = (4000*(l+1))/1461001;
	l = l - (1461*i)/4 + 31;
	j = (80*l)/2447;
	d = l - (2447*j)/80;
	l = j/11;
	m = j + 2 - 12*l;
	y = 100*(n-49) + i + l;
	
	*year  = y;
	*month = m;
	*day   = d;
}

void Jul_JulYMDofJDN(jd,year,month,day)
RL_INT4 jd,*year,*month,*day;
{
	RL_INT4 j,k,l,n,d,i,m,y;
	
	j = jd + 1402;
	k = (j-1)/1461;
	l = j - 1461*k;
	n = (l-1)/365 - l/1461;
	i = l - 365*n + 30;
	j = (80*i)/2447;
	d = i - (2447*j)/80;
	i = j/11;
	m = j + 2 - 12*i;
	y = 4*k + n + i - 4716;
	
	*year  = y;
	*month = m;
	*day   = d;
}

RL_INT4 Jul_DUTCofYMD(year,month,day)
RL_INT4 year,month,day;
{
	RL_BOOL isgreg;
	/* Correct year & month ranges */
	Jul_FixYM(&year,&month);
	/* Determine which calendar to use */
	if	  (year > gregorian_year)   isgreg = TRUE;
		else if   (year < gregorian_year)   isgreg = FALSE;
			else {
				if      (month > gregorian_month) isgreg = TRUE;
					else if (month < gregorian_month) isgreg = FALSE;
						else                              isgreg = (day >= gregorian_day);
							}
	/* Calculate and return date */
	if (isgreg) return Jul_JDNofGregYMD(year,month,day) - JD_OF_J2000;
	else        return Jul_JDNofJulYMD (year,month,day) - JD_OF_J2000;
}

void Jul_YMDofDUTC(dutc,year,month,day)
RL_INT4 dutc,*year,*month,*day;
{
	if (dutc >= gregorian_dutc)
		Jul_GregYMDofJDN(dutc + JD_OF_J2000, year, month, day);
		else
			Jul_JulYMDofJDN (dutc + JD_OF_J2000, year, month, day);
			}
// end of  Mark R. Showalter code


int32_t mdy_to_julian(int32_t month,int32_t day,int32_t year)
{
	int32_t val;
	val = (int32_t)Jul_DUTCofYMD(year,month,day);
	if ( val < 0 || val > 36500 )
	{
		fprintf(stderr,"%d/%d/%d for before 1/1/2000 or after 12/31/2100\n",month,day,year);
		return(-1);
	}
	return(val);
}

int32_t calc_daylight_savings(int32_t *startp,int32_t *endp,int32_t year)
{
	int32_t day,month,n,jdate;
	month = 3;
	n = 0;
	for (day=1; day<=31; day++)
	{
		jdate = mdy_to_julian(month,day,year);
		//printf("jdate %d\n",jdate);
		if ( (jdate%7) == JDATE_SUNDAY )
		{
			if ( ++n == 2 )
			{
				*startp = jdate;
				break;
			}
		}
	}
	for (day=1; day<=31; day++)
	{
		jdate = mdy_to_julian(11,day,year);
		//printf("jdate %d\n",jdate);
		if ( (jdate%7) == JDATE_SUNDAY )
		{
			*endp = jdate;
			break;
		}
	}
	return(0);
}

char *tick_str(int32_t tick)
{
	static char buf[BUFSIZE];
	char *ampm_str;
	tick %= SECONDS_IN_DAY;
	//tick += 3 * SECONDS_IN_HOUR;
	if ( tick/SECONDS_IN_HOUR >= 12 )
		ampm_str = "PM";
	else
		ampm_str = "AM";
	if ( tick/SECONDS_IN_HOUR > 12 )
	{
		tick -= 12 * SECONDS_IN_HOUR;
	}
	sprintf(buf,"%02d:%02d:%02d %s",tick/SECONDS_IN_HOUR,(tick%SECONDS_IN_HOUR)/SECONDS_IN_MINUTE,tick%SECONDS_IN_MINUTE,ampm_str);
	return(buf);
}

int32_t julian_to_day(int32_t jdate)
{
	int32_t year,month,day;
	Jul_YMDofDUTC(jdate,&year,&month,&day);
	return((int32_t)day);
}

int32_t julian_to_month(int32_t jdate)
{
	int32_t year,month,day;
	Jul_YMDofDUTC(jdate,&year,&month,&day);
	return((int32_t)month);
}

int32_t julian_to_year(int32_t jdate)
{
	int32_t year,month,day;
	Jul_YMDofDUTC(jdate,&year,&month,&day);
	return((int32_t)year);
}

char *jdate_fname(int32_t jdate)
{
	static char buf[100];
	int32_t month,day,year;
	month = julian_to_month(jdate);
	day = julian_to_day(jdate);
	year = julian_to_year(jdate);
	sprintf(buf,"%s_%02d_%02d",months[month-1],day,year%100);
	return(buf);
}

int32_t jdatetime_to_year(int32_t jdatetime)
{
	return(julian_to_year(jdatetime/SECONDS_IN_DAY));
}

int32_t jdatetime_to_month(int32_t jdatetime)
{
	return(julian_to_month(jdatetime/SECONDS_IN_DAY));
}

int32_t jdatetime_to_day(int32_t jdatetime)
{
	return(julian_to_day(jdatetime/SECONDS_IN_DAY));
}

int32_t dst_adjust(int32_t jdatetime)
{
	int32_t adjust,year,jdate,y;
	if ( jdatetime == 0 )
		return(0);
	jdate = jdatetime / SECONDS_IN_DAY;
	adjust = 0;
	year = jdatetime_to_year(jdatetime);
	y = year - FIRST_YEAR;
	if ( y >= (int32_t)(sizeof(START_DST)/sizeof(*START_DST)) )
	{
		if ( jdatetime != 0 )
			printf("dst_adjust: unsupported year %d, %d, need to add it\n",year,jdatetime);
		return(0);
	}
	if ( jdate >= START_DST[y] && jdate < END_DST[y] )
		adjust = SECONDS_IN_HOUR;
	return(adjust);
}

int32_t conv_to_gmt(int32_t jdatetime,int32_t tzone)
{
	jdatetime -= tzone;
	jdatetime -= dst_adjust(jdatetime);
	return(jdatetime);
}

int32_t mdy_to_jdatetime(int32_t month,int32_t day,int32_t year)
{
	int32_t val,jdatetime;
	val = (int32_t)Jul_DUTCofYMD(year,month,day);
	if ( val < 0 || val > 36500 )
	{
		fprintf(stderr,"%d/%d/%d for before 1/1/2000 or after 12/31/2100\n",month,day,year);
		return(-1);
	}
	jdatetime = conv_to_gmt(val * SECONDS_IN_DAY,TIMEZONE_ADJUST);
	return(jdatetime);
}

int32_t year_to_jdatetime(int32_t year)
{
	return(mdy_to_jdatetime(1,1,year));// + TIMEZONE_ADJUST);
}

int32_t gmt_to_local(int32_t jdatetime,int32_t tzone)
{
	int32_t ltime;
	ltime = jdatetime + tzone;
    //printf("ltime.%d j.%d tzon.%d\n",ltime,jdatetime,tzone);
	ltime += dst_adjust(jdatetime);
	return(ltime);
}

int32_t time_t_to_jdate(time_t t)
{
	int32_t jdate;
	struct tm *tp;
	tp = gmtime(&t);//localtime(&t);
	//printf("mon %d, day %d, year %d\n",tp->tm_mon,tp->tm_mday,tp->tm_year);
	jdate = mdy_to_julian(tp->tm_mon+1,tp->tm_mday,tp->tm_year+1900);
	//printf("jdate %d, %s\n",jdate,jdate_fname(jdate));
	return(jdate);
}

int32_t time_t_to_jtime(time_t t)
{
	int32_t jdate;
	struct tm *tp;
	tp = gmtime(&t);//localtime(&t);
	jdate = time_t_to_jdate(t);
	return((jdate*SECONDS_IN_DAY) + (tp->tm_hour*SECONDS_IN_HOUR) + (tp->tm_min*SECONDS_IN_MINUTE) + tp->tm_sec);
}

int32_t current_jtime()
{
	time_t T;
	time(&T);
    //printf("%d --> jtime %d\n",(int32_t)T,time_t_to_jtime(T));
	return(time_t_to_jtime(T));
}

int32_t actual_gmt_jdatetime()
{
	int32_t jdatetime;
	jdatetime = current_jtime();
	ACTUAL_GMT_JDATETIME = jdatetime;
    //printf("current local time %s, timezone %d, DST %d\n",jdatetime_str(jdatetime),TIMEZONE_ADJUST,dst_adjust(jdatetime));
	return(jdatetime);
}

int32_t gmt_jdatetime()
{
	static int32_t counter;
	if ( counter++ > 1000 || ACTUAL_GMT_JDATETIME == 0 )
	{
		counter = 0;
		return(actual_gmt_jdatetime());
	}
	return(ACTUAL_GMT_JDATETIME);
}

char *jdatetime_str(int32_t jdatetime)
{
	static char buf[BUFSIZE];
	jdatetime = gmt_to_local((int32_t)jdatetime,TIMEZONE_ADJUST);
	sprintf(buf,"%s %s",jdate_fname((int32_t)jdatetime/SECONDS_IN_DAY),tick_str((int32_t)jdatetime%SECONDS_IN_DAY));
	return(buf);
}

char *jdatetime_str2(int32_t jdatetime)
{
	static char buf[BUFSIZE];
	jdatetime = gmt_to_local((int32_t)jdatetime,TIMEZONE_ADJUST);
	sprintf(buf,"%s %s",jdate_fname((int32_t)jdatetime/SECONDS_IN_DAY),tick_str((int32_t)jdatetime%SECONDS_IN_DAY));
	return(buf);
}

char *jdatetime_str3(int32_t jdatetime)
{
	static char buf[BUFSIZE];
	jdatetime = gmt_to_local((int32_t)jdatetime,TIMEZONE_ADJUST);
	sprintf(buf,"%s %s",jdate_fname((int32_t)jdatetime/SECONDS_IN_DAY),tick_str((int32_t)jdatetime%SECONDS_IN_DAY));
	return(buf);
}

int32_t timezone_init(int32_t timezone)
{
	int32_t y;
	TIMEZONE_ADJUST = timezone;
	for (y=0; y<(int32_t)(sizeof(START_DST)/sizeof(*START_DST)); y++)
	{
		calc_daylight_savings(&START_DST[y],&END_DST[y],FIRST_YEAR + y);
		static unsigned long long debugmsg;
		if ( debugmsg++ < 3 )
		{
			printf("For %d DST starts %s, ends ",FIRST_YEAR+y,jdate_fname(START_DST[y]));
			printf("%s\n",jdate_fname(END_DST[y]));
		}
	}
	return(0);
}

uint32_t conv_unixtime(uint32_t unixtime)
{
    uint32_t zerotime = 946684800;
	return(unixtime - zerotime);
}

void init_jdatetime(int32_t firstunixtime,int32_t timezone)
{
	timezone_init(timezone);
	FIRSTJDATETIME = conv_unixtime(firstunixtime);
    printf("FIRSTJDATETIME %s timezone.%d\n",jdatetime_str(FIRSTJDATETIME),timezone);
}
#endif
