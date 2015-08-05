//
//  child.h
//
//  Created by jl777 on 16/4/15.
//  Copyright (c) 2015 jl777. All rights reserved.
//

#ifndef xcode_child_h
#define xcode_child_h

int32_t child_diedflag; void sigchld_handler(int a) { child_diedflag = 1; }
static void wait_child()
{
    sigset_t mask,oldmask;
    int32_t status;
    // Set mask to temporarily block SIGCHLD
    sigemptyset(&mask);
    sigaddset(&mask,SIGCHLD);
    sigprocmask(SIG_BLOCK,&mask,&oldmask);
    while ( child_diedflag == 0 )
        sigsuspend(&oldmask);
    sigprocmask(SIG_UNBLOCK,&mask,NULL);
    wait(&status);
}

int main(int argc,const char *argv[])
{
    if ( fork() != 0 )
        exit(0);
    setsid();
    (void)signal(SIGCHLD,sigchld_handler);
    while ( 1 )
    {
        child_diedflag = 0;
        if ( fork() != 0 )
        {
            wait_child();       // parent
            fprintf(stderr,"child process died, restarting in %d seconds\n",30);
        }
        else
        {
            main(argc,argv);
            break;
        }
    }
    return(-1);
}
#endif
