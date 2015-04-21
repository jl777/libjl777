
#include <windows.h>
#include <wincrypt.h>

int32_t OS_getppid() { int32_t parent_pid; return(parent_pid); }

int32_t OS_waitpid(int32_t childpid,int32_t *statusp,int32_t flags) { return(waitpid(childpid,statusp,flags)); }

int32_t OS_launch_process(char *args[])
{
    pid_t child_pid;
    if ( (child_pid= fork()) >= 0 )
    {
        if ( child_pid == 0 )
        {
            printf("plugin PID =  %d, parent pid = %d (%s, %s, %s, %s, %s)\n",getpid(),getppid(),args[0],args[1],args[2],args[3],args[4]);
            return(execv(args[0],args));
        }
        else
        {
            printf("parent PID =  %d, child pid = %d\n",getpid(),child_pid);
            return(child_pid);
        }
    }
    else return(-1);
}

// from tweetnacl
void randombytes(unsigned char *x,long xlen)
{
    HCRYPTPROV prov = 0;
    CryptAcquireContextW(&prov, NULL, NULL,PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);
    CryptGenRandom(prov, xlen, x);
    CryptReleaseContext(prov, 0);
}




