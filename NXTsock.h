
//  Created by jl777
//  MIT License
//

#ifndef NXTAPI_NXTsock_h
#define NXTAPI_NXTsock_h

typedef int32_t (*handler)();
//struct server_request_header { int32_t retsize,argsize,variant,funcid; };
struct handler_info { handler variant_handler; int32_t variant,funcid; long argsize,retsize; char **whitelist; };
struct server_request
{
	struct server_request_header H;
    int32_t timestamp,coinid,srcgateway,destgateway,numinputs,isforging;
    char NXTaddr[MAX_NXTADDR_LEN];
};

struct server_response
{
	struct server_request_header H;
    int32_t retsize,numips,numnxtaccts,coinid;
    int64_t nodeshares,current_nodecoins,nodecoins,nodecoins_sent;
};

static int32_t Numhandlers;
static struct handler_info Handlers[100];
//struct server_request WINFO[NUM_GATEWAYS];

int32_t register_variant_handler(int32_t variant,handler variant_handler,int32_t funcid,long argsize,long retsize,char **whitelist)
{
    int32_t i;
    if ( Numhandlers > (int)(sizeof(Handlers)/sizeof(*Handlers)) )
    {
        printf("Out of space: Numhandlers %d\n",Numhandlers);
        return(-1);
    }
    for (i=0; i<Numhandlers; i++)
    {
        if ( Handlers[i].variant == variant && Handlers[i].funcid == funcid )
        {
            printf("Overwriting handler for variant.%d funcid.%d\n",variant,funcid);
            Handlers[i].variant_handler = variant_handler;
            return(i);
        }
    }
    printf("Setting handler.%d for variant.%d funcid.%d\n",i,variant,funcid);
    Handlers[i].variant_handler = variant_handler;
    Handlers[i].funcid = funcid;
    Handlers[i].variant = variant;
    Handlers[i].argsize = argsize;
    Handlers[i].retsize = retsize;
    Handlers[i].whitelist = whitelist;
    return(Numhandlers++);
}

int32_t find_handler(int32_t variant,int32_t funcid)
{
    int32_t i;
    for (i=0; i<Numhandlers; i++)
    {
        if ( Handlers[i].variant == variant && Handlers[i].funcid == funcid )
            return(i);
    }
    return(-1);
}

int32_t check_whitelist(char **whitelist,char *ipaddr)
{
    int32_t i;
    //printf("check_whitelist.(%s) %p\n",ipaddr,whitelist);
    if ( whitelist == 0 )
        return(0);
    //if ( listcmp(whitelist,ipaddr) == 0 )
    //    return(0);
    for (i=0; whitelist[i]!=0&&whitelist[i][0]!=0; i++)
    {
        //printf("%s vs %s\n",whitelist[i],ipaddr);
        if ( strcmp(whitelist[i],ipaddr) == 0 )
            return(i);
    }
    printf("%s not in whitelist (%s) %d %d\n",ipaddr,whitelist[0],strcmp(whitelist[0],ipaddr),listcmp(whitelist,ipaddr));
    return(-1);
}

int32_t process_client_packet(int32_t variant,struct server_request_header *req,int32_t len,char *clientip)
{
    int32_t ind;
    char **whitelist;
    //printf("process_client_packet variant.%d func.%d len.%d\n",variant,req->funcid,len);
    ind = find_handler(variant,req->funcid);
    //printf("process_client_packet ind.%d got %d vs %d, ip.(%s) variant.%d func.%d\n",ind,len,req->argsize,clientip,req->variant,req->funcid);
    if ( ind >= 0 )
    {
        whitelist = Handlers[ind].whitelist;
        if ( check_whitelist(whitelist,clientip) >= 0 )
        {
            //printf("call handler.%p\n",Handlers[ind].variant_handler);
            return((*Handlers[ind].variant_handler)((void *)req,clientip));
        }
        printf("nonwhitelist attempt (%s)\n",clientip);
    }
    else
    {
        printf("No handler for variant.%d funcid.%d (%s)\n",variant,req->funcid,clientip);
        // whitelist = Server_names;
        // if ( check_whitelist(whitelist,clientip) >= 0 )
        //     WINFO[req->srcgateway % NUM_GATEWAYS] = *req;
        req->argsize = 0;
    }
    req->retsize = 0;
    return(0);
}

int32_t wait_for_serverdata(int32_t *sdp,unsigned char *buffer,int32_t len)
{
    struct server_request_header *H;
	int32_t total,rc;
    //printf("wait for %d\n",len);
	total = 0;
    H = (struct server_request_header *)buffer;
	while ( total < len )
	{
		rc = (int)recv(*sdp,&buffer[total],len - total, 0);
		if ( rc <= 0 )
		{
			if ( rc < 0 )
				printf("recv() failed\n");
			else printf("The server closed the connection\n");
			close(*sdp);
			*sdp = -1;
			return(-1);
		}
		total += rc;
        if ( total >= (int32_t)sizeof(struct server_request_header) && H->retsize != len )
        {
            //printf("expected len %d -> %d\n",len,H->retsize);
            len = H->retsize;
        }
	}
	return(total);
}

int32_t server_request(int32_t *sdp,char *destserver,struct server_request_header *req,int32_t variant,int32_t funcid)
{
	int32_t rc,retsize,ind;
	char server[128],servport[16];
	struct in6_addr serveraddr;
	struct addrinfo hints, *res=NULL;
    strcpy(servport,SERVER_PORTSTR);
    req->variant = variant;
    req->funcid = funcid;
    ind = find_handler(variant,funcid);
    if ( ind >= 0 )
    {
        if ( req->argsize == 0 )
            req->argsize = (int)Handlers[ind].argsize;
        if ( req->retsize == 0 )
            req->retsize = (int)Handlers[ind].retsize;
    }
    if ( req->argsize == 0 )
        req->argsize = sizeof(struct server_request);
    if ( 0 && req->retsize == 0 )
        req->retsize = sizeof(struct server_response);
    retsize = req->retsize;
    printf("server requests variant.%d funcid.%d ind.%d argsize.%d retsize.%d\n",variant,funcid,ind,req->argsize,req->retsize);
    usleep(1000);
 	//static pthread_mutex_t mutex;
 	//portable_mutex_lock(&mutex);
    if ( *sdp < 0 )
    {
        sprintf(servport,"%d",SERVER_PORT + variant);
        safecopy(server, destserver,sizeof(server));
        memset(&hints, 0x00, sizeof(hints));
        hints.ai_flags    = AI_NUMERICSERV;
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        rc = inet_pton(AF_INET, server, &serveraddr);
        if ( rc == 1 )    // valid IPv4 text address?
        {
            hints.ai_family = AF_INET;
            hints.ai_flags |= AI_NUMERICHOST;
        }
        else
        {
            rc = inet_pton(AF_INET6, server, &serveraddr);
            if ( rc == 1 ) // valid IPv6 text address?
            {
                hints.ai_family = AF_INET6;
                hints.ai_flags |= AI_NUMERICHOST;
            }
        }
        rc = getaddrinfo(server, servport, &hints, &res);
        if ( rc != 0 )
        {
            printf("Host not found --> %s\n", gai_strerror((int)rc));
            //if (rc == EAI_SYSTEM)
                printf("getaddrinfo() failed\n");
            *sdp = -1;
            //portable_mutex_unlock(&mutex);
            return(-1);
        }
        *sdp = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if ( *sdp < 0 )
        {
            printf("socket() failed\n");
            *sdp = -1;
            //portable_mutex_unlock(&mutex);
            return(-1);
        }
        //fprintf(stderr,"start connect %s:%s\n",server,servport);
        rc = connect(*sdp,res->ai_addr,res->ai_addrlen);
        //fprintf(stderr,"back from connect rc.%d\n",rc);
        if ( rc < 0 )
        {
            printf("connection to %s variant.%d failure port.(%s) ",server,variant,servport);
            perror("connect() failed");
            close(*sdp);
            *sdp = -1;
            sleep(1);
            //portable_mutex_unlock(&mutex);
            return(-1);
        }
        //printf("connection to %s variant.%d port.(%s)\n",server,variant,servport);
        if ( res != NULL )
            freeaddrinfo(res);
    }
    //printf("send %d req %d bytes from variant.%d\n",*sdp,req->argsize,variant);
    if ( (rc = (int)send(*sdp,(char *)req,req->argsize,0)) < 0 )
    {
        printf("send(%d) request failed\n",variant);
        close(*sdp);
        *sdp = -1;
        //portable_mutex_unlock(&mutex);
        return(-1);
    }
    //usleep(1);
    rc = 0;
  //  printf("ind.%d retsize %d req->retsize.%d (variant.%d funcid.%d)\n",ind,retsize,req->retsize,variant,funcid);
    if ( retsize > 0 )
    {
        if ( (rc= wait_for_serverdata(sdp,(unsigned char *)req,retsize)) != retsize )
            printf("GATEWAY_RETSIZE error retsize.%d rc.%d\n",retsize,rc);
        //else usleep(1000);
    }
   // printf("finished ind.%d retsize %d req->retsize.%d (variant.%d funcid.%d)\n",ind,retsize,req->retsize,variant,funcid);
    close(*sdp);
    *sdp = -1;
    //portable_mutex_unlock(&mutex);
    return(rc);
}

int32_t wait_for_client(int32_t *sdp,char str[INET6_ADDRSTRLEN],int32_t variant)
{
	struct sockaddr_in serveraddr, clientaddr;
	socklen_t addrlen = sizeof(clientaddr);
	int32_t sdconn = -1;
    str[0] = 0;
	//get_lockid(0);
	while ( *sdp < 0 )
	{
		if ((*sdp = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("socket() failed");
			break;
		}
		/*if ( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,(char *)&on,sizeof(on)) < 0)
         {
         perror("setsockopt(SO_REUSEADDR) failed");
         close(sd);
         sd = -1;in
         break;
         }*/
		memset(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_port   = htons(SERVER_PORT+variant);
		//serveraddr.sin_addr   = in6addr_any;
		if ( bind(*sdp,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0 )
		{
			printf("failed to bind %s variant.%d\n",(char *)&serveraddr.sin_addr,variant);
			perror("variant bind() failed");
            exit(1);
			close(*sdp);
			*sdp = -1;
			sleep(30);
			continue;
		}
		if ( listen(*sdp, 3000) < 0 )
		{
			perror("listen() failed");
			close(*sdp);
			*sdp = -1;
			break;
		}
	}
	//release_lockid(0);
	
	if ( *sdp < 0 )
		return(-1);
	else
	{
		if ((sdconn = accept(*sdp, NULL, NULL)) < 0)	// non blocking would be nice
		{
			perror("accept() failed");
			return(-1);
		}
		else
		{
			getpeername(sdconn, (struct sockaddr *)&clientaddr,&addrlen);
			if ( inet_ntop(AF_INET, &clientaddr.sin_addr, str, INET_ADDRSTRLEN) != 0 )
              //  if ( inet_ntop(AF_INET6, &clientaddr.sin6_addr, str, INET6_ADDRSTRLEN) != 0 )
			{
                printf("variant.%d [Client address is %20s | Client port is %6d] sdconn.%d\n",variant,str,ntohs(clientaddr.sin_port),sdconn);
			} else printf("Error getting client str\n");
		}
	}
	return(sdconn);
}

void *_server_loop(void *_args)
{
    int64_t variant;
	struct server_request_header *req;
	int32_t sd,sdconn,expected,rc,bytesReceived,numreqs = 0;
	long xferred = 0;
    char clientip[INET6_ADDRSTRLEN],*ip;
	variant = *(int64_t *)_args;
	req = malloc(65536);
    sd = -1;
	printf("Start server_loop.%ld on port.%d\n",(long)variant,SERVER_PORT+(int)variant);
	while ( 1 )
	{
		//usleep(10000);
		if ( (sdconn= wait_for_client(&sd,clientip,(int)variant)) >= 0 )
		{
			//printf("wait for req %d bytes from variant.%d\n",expected,(int)variant);
			while ( 1 )
			{
                expected = (int)sizeof(*req);//sizeof(*req);// - sizeof(req->space));
				bytesReceived = 0;
				while ( bytesReceived < expected )
				{
                    // printf("bytesReceived %d < expected %d\n",bytesReceived,expected);
					rc = (int)recv(sdconn,&((unsigned char *)req)[bytesReceived],expected - bytesReceived, 0);
					if ( rc <= 0 )
					{
						if ( rc < 0 )
							printf("recv() failed\n");
						else printf("The client closed the connection\n");
						break;
					}
					bytesReceived += rc;
                }
                if ( 1 && req->argsize < 65534 && expected != req->argsize )
                {
                    //printf("req from clientip.%s variant %d, funcid.%d expected %d -> %d\n",clientip,(int)variant,req->funcid,expected,req->argsize);
                    expected = req->argsize;
                }
				while ( bytesReceived < expected )
				{
                    //printf("B bytesReceived %d < expected %d\n",bytesReceived,expected);
					rc = (int)recv(sdconn,&((unsigned char *)req)[bytesReceived],expected - bytesReceived, 0);
					if ( rc <= 0 )
					{
						if ( rc < 0 )
							printf("recv() failed\n");
						else printf("The client closed the connection\n");
						break;
					}
					bytesReceived += rc;
                }
				if ( bytesReceived < expected )
				{
					printf("The client.%ld closed the connection before all of the data was sent, got %d of %d\n",(long)variant,bytesReceived,expected);
					break;
				}
                ip = clientip;
                //printf("clientip.(%s)\n",clientip);
                if ( strncmp(clientip,"::ffff:",strlen("::ffff:")) == 0 )
                    ip += strlen("::ffff:");
                req->retsize = process_client_packet((int)variant,req,bytesReceived,ip);
                numreqs++;
                if ( req->retsize > 0 )
                {
                    //printf("return %d to %s for variant.%d funcid.%d\n",req->retsize,clientip,(int)variant,req->funcid);
                    if ( (rc = (int)send(sdconn,(char *)req,req->retsize,0)) < req->retsize )
                    {
                        printf("send() failed? rc.%d instead of %d\n",rc,req->retsize);
                        break;
                    }
                    xferred += rc;
                    //printf("done return %d to %s for variant.%d funcid.%d\n",req->retsize,clientip,(int)variant,req->funcid);
                }
                //if ( req->H.argsize == 0 )
                {
                    //printf("break variant.%d funcid.%d\n",variant,req->H.funcid);
                    //break;
                }
                //if ( variant == 0 ) // hacky special case!
                break;
			}
           // printf("Server.%ld loop xferred %ld bytes, in %d REQ's ave %ld bytes\n",(long)variant,xferred,numreqs,xferred/numreqs);
			close(sdconn);
			//sdconn = -1;
		}
	}
}




#endif
