//
//  mofnfs.h
//  libjl777
//
//  Created by jl777 on 10/6/14.
//  Copyright (c) 2014 jl777. MIT license
//

#ifndef libjl777_cloudfs_h
#define libjl777_cloudfs_h



char *onion_sendfile(int32_t L,struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,char *dest,FILE *fp)
{
    return(0);
}

char *mofn_savefile(struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,FILE *fp,int32_t L,int32_t M,int32_t N,char *usbname,char *password)
{
    int32_t status = 0;
    char retstr[1024];
    sprintf(retstr,"{\"result\":\"status.%d\",\"filesize\":\"%ld\",\"descr\":\"mofn_savefile M.%d of N.%d sent with Lfactor.%d usbname.(%s) usedpassword.%d dont lose the password!\"}",status,ftell(fp),M,N,L,usbname,password!=0);
    return(clonestr(retstr));
}

char *mofn_restorefile(struct sockaddr *prevaddr,char *verifiedNXTaddr,char *NXTACCTSECRET,char *sender,FILE *fp,int32_t L,int32_t M,int32_t N,char *usbname,char *password)
{
    int32_t mcount=0,status = 0;
    char retstr[1024];
    sprintf(retstr,"{\"result\":\"status.%d\",\"filesize\":\"%ld\",\"descr\":\"mofn_restorefile M.%d of N.%d sent with Lfactor.%d usbname.(%s) usedpassword.%d reconstructed from %d parts\"}",status,ftell(fp),M,N,L,usbname,password!=0,mcount);
    return(clonestr(retstr));
}
#endif
