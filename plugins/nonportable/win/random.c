/******************************************************************************
 * Copyright © 2014-2015 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#include <inttypes.h>
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <process.h>
#include <tlhelp32.h>
#include <time.h>

uint32_t OS_conv_datenum(int32_t datenum,int32_t hour,int32_t minute,int32_t second) // datenum+H:M:S -> unix time
{
    struct tm t;
    memset(&t,0,sizeof(t));
    t.tm_year = (datenum / 10000) - 1900, t.tm_mon = ((datenum / 100) % 100) - 1, t.tm_mday = (datenum % 100);
    t.tm_hour = hour, t.tm_min = minute, t.tm_sec = second;
    return(time(NULL));
    //return((uint32_t)_mkgmtime(&t));
}

int32_t OS_conv_unixtime(int32_t *secondsp,time_t timestamp) // gmtime -> datenum + number of seconds
{
    struct tm t; int32_t datenum; uint32_t checktime; char buf[64];
    t = *gmtime(&timestamp);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ",&t); //printf("%s\n",buf);
    datenum = conv_date(secondsp,buf);
    if ( (checktime= OS_conv_datenum(datenum,*secondsp/3600,(*secondsp%3600)/60,*secondsp%60)) != timestamp )
    {
        printf("error: timestamp.%lu -> (%d + %d) -> %u\n",timestamp,datenum,*secondsp,checktime);
        return(-1);
    }
    return(datenum);
}

int32_t OS_getpid() { return(GetCurrentProcessId()); }

int32_t OS_getppid()
{
    int pid, ppid = -1;
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { 0 };
    pe.dwSize = sizeof(PROCESSENTRY32);
    pid = GetCurrentProcessId();
    if( Process32First(h, &pe)) {
    	do {
    		if (pe.th32ProcessID == pid) {
    			ppid = pe.th32ParentProcessID;
                break;
    		}
    	} while( Process32Next(h, &pe));
    }

    CloseHandle(h);
    return ppid;
}

int32_t OS_waitpid(int32_t pid, int32_t *statusp, int32_t flags) {
    _cwait (statusp, pid, WAIT_CHILD);
}

int32_t  OS_launch_process(char *args[])
{
    int32_t pid;
    pid = _spawnl( _P_NOWAIT, args[0], args[0],  NULL, NULL );
    return pid;
}

// from tweetnacl
void randombytes(unsigned char *x,long xlen)
{
    HCRYPTPROV prov = 0;
    CryptAcquireContextW(&prov, NULL, NULL,PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
    CryptGenRandom(prov, xlen, x);
    CryptReleaseContext(prov, 0);
}

int32_t OS_init()
{
    // Initialize Windows Sockets
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != NO_ERROR)
    {
        printf("Error: TCP/IP socket library failed to start (WSAStartup returned error %d)\n", ret);
        //printf("%s\n", strError.c_str());
        return -1;
    }
    return(0);
}

struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
	struct tm *p = gmtime(timep);
	memset(result, 0, sizeof(*result));
	if (p) {
        *result = *p;
        p = result;
	}
	return p;
}
