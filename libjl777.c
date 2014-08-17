/*
 * libwebsockets-test-server - libwebsockets test implementation
 *
 * Copyright (C) 2010-2011 Andy Green <andy@warmcat.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation:
 *  version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */
#ifdef CMAKE_BUILD
#include "lws_config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdint.h>

#ifdef _WIN32
#include <io.h>
#ifdef EXTERNAL_POLL
#define poll WSAPoll
#endif
#else
#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#define LIBWEBSOCKETS_MILLIS 250
#define LIBWEBSOCKETS_PORT 7777
extern int testimagelen;
extern char testforms[1024*1024];
unsigned char NXTprotocol_parms[4096],testimage[1024*1024*sizeof(int32_t)];


#include "includes/libwebsockets.h"

#include "libwebsocketsglue.h"
#define INSIDE_CCODE
#include "jl777.cpp"
#undef INSIDE_CCODE



static int close_testing;
int max_poll_elements;

struct pollfd *pollfds;
int *fd_lookup;
int count_pollfds;
static volatile int force_exit = 0;
static struct libwebsocket_context *context;

/*
 * This demo server shows how to use libwebsockets for one or more
 * websocket protocols in the same server
 *
 * It defines the following websocket protocols:
 *
 *  dumb-increment-protocol:  once the socket is opened, an incrementing
 *				ascii string is sent down it every 50ms.
 *				If you send "reset\n" on the websocket, then
 *				the incrementing number is reset to 0.
 *
 *  lws-mirror-protocol: copies any received packet to every connection also
 *				using this protocol, including the sender
 */

enum demo_protocols {
	/* always first */
	PROTOCOL_HTTP = 0,

	PROTOCOL_DUMB_INCREMENT,
	PROTOCOL_LWS_MIRROR,

	/* always last */
	DEMO_PROTOCOL_COUNT
};

#ifndef INSTALL_DATADIR
#define INSTALL_DATADIR "~"
#endif

#define LOCAL_RESOURCE_PATH INSTALL_DATADIR
char *resource_path = LOCAL_RESOURCE_PATH;

/*
 * We take a strict whitelist approach to stop ../ attacks
 */

struct serveable {
	const char *urlpath;
	const char *mimetype;
}; 

struct per_session_data__http {
	int fd;
};

/*
 * this is just an example of parsing handshake headers, you don't need this
 * in your code unless you will filter allowing connections by the header
 * content
 */

static void
dump_handshake_info(struct libwebsocket *wsi)
{
	int n;
	static const char *token_names[] = {
		/*[WSI_TOKEN_GET_URI]		=*/ "GET URI",
		/*[WSI_TOKEN_POST_URI]		=*/ "POST URI",
		/*[WSI_TOKEN_HOST]		=*/ "Host",
		/*[WSI_TOKEN_CONNECTION]	=*/ "Connection",
		/*[WSI_TOKEN_KEY1]		=*/ "key 1",
		/*[WSI_TOKEN_KEY2]		=*/ "key 2",
		/*[WSI_TOKEN_PROTOCOL]		=*/ "Protocol",
		/*[WSI_TOKEN_UPGRADE]		=*/ "Upgrade",
		/*[WSI_TOKEN_ORIGIN]		=*/ "Origin",
		/*[WSI_TOKEN_DRAFT]		=*/ "Draft",
		/*[WSI_TOKEN_CHALLENGE]		=*/ "Challenge",

		/* new for 04 */
		/*[WSI_TOKEN_KEY]		=*/ "Key",
		/*[WSI_TOKEN_VERSION]		=*/ "Version",
		/*[WSI_TOKEN_SWORIGIN]		=*/ "Sworigin",

		/* new for 05 */
		/*[WSI_TOKEN_EXTENSIONS]	=*/ "Extensions",

		/* client receives these */
		/*[WSI_TOKEN_ACCEPT]		=*/ "Accept",
		/*[WSI_TOKEN_NONCE]		=*/ "Nonce",
		/*[WSI_TOKEN_HTTP]		=*/ "Http",

		"Accept:",
		"If-Modified-Since:",
		"Accept-Encoding:",
		"Accept-Language:",
		"Pragma:",
		"Cache-Control:",
		"Authorization:",
		"Cookie:",
		"Content-Length:",
		"Content-Type:",
		"Date:",
		"Range:",
		"Referer:",
		"Uri-Args:",

		/*[WSI_TOKEN_MUXURL]	=*/ "MuxURL",
	};
	char buf[4096];

	for (n = 0; n < sizeof(token_names) / sizeof(token_names[0]); n++) {
		if (!lws_hdr_total_length(wsi, n))
			continue;

		lws_hdr_copy(wsi, buf, sizeof buf, n);
        if ( strcmp(token_names[n],"Uri-Args:") == 0 )
            strcpy((char *)NXTprotocol_parms,buf);
		//fprintf(stderr, "    %s = %s n.%d\n", token_names[n], buf,n);
	}
}

const char * get_mimetype(const char *file)
{
	int n = (int32_t)strlen(file);

	if (n < 5)
		return NULL;

	if (!strcmp(&file[n - 4], ".ico"))
		return "image/x-icon";

	if (!strcmp(&file[n - 4], ".png"))
		return "image/png";

	if (!strcmp(&file[n - 5], ".html"))
		return "text/html";

	return NULL;
}

/* this protocol server (always the first one) just knows how to do HTTP */

static int callback_http(struct libwebsocket_context *context,struct libwebsocket *wsi,enum libwebsocket_callback_reasons reason, void *user,void *in, size_t len)
{
#if 0
	char client_name[128];
	char client_ip[128];
#endif
	char buf[256];
	char leaf_path[1024];
	char b64[64];
	struct timeval tv;
	//int n, m;
    int64_t mylen;
	//unsigned char *p;
    struct NXT_protocol *nxtprotocol;
	char *other_headers;
	static unsigned char buffer[4096];
	//struct stat stat_buf;
	struct per_session_data__http *pss = (struct per_session_data__http *)user;
	const char *mimetype;
#ifdef EXTERNAL_POLL
	struct libwebsocket_pollargs *pa = (struct libwebsocket_pollargs *)in;
#endif

	switch (reason)
    {
        case LWS_CALLBACK_HTTP:
            dump_handshake_info(wsi);
            if ( len < 1 )
            {
                libwebsockets_return_http_status(context, wsi,HTTP_STATUS_BAD_REQUEST, NULL);
                return -1;
            }
            if ( strchr((const char *)in + 1, '/') != 0 ) // this server has no concept of directories
            {
                libwebsockets_return_http_status(context, wsi,HTTP_STATUS_FORBIDDEN, NULL);
                return -1;
            }
            // if a legal POST URL, let it continue and accept data
            if ( lws_hdr_total_length(wsi,WSI_TOKEN_POST_URI) != 0 )
                return 0;
#ifdef GRAPHICS_SUPPORT
            if ( (testimagelen= search_images(testimage,(char *)in+1)) != 0 )
            {
                static int counter;
                static uint64_t lastmicro;
                if ( testimagelen != 0 )
                {
                    counter++;
                    //printf("%d IN.(%s) testimagelen.%d\n",counter,in,testimagelen);
                    unsigned char *space;
                    space = malloc(testimagelen+512);
                    sprintf((char *)space,
                            "HTTP/1.0 200 OK\x0d\x0a"
                            "Server: libwebsockets\x0d\x0a"
                            "Content-Type: image/jpeg\x0d\x0a"
                            "Content-Length: %u\x0d\x0a\x0d\x0a",
                            (unsigned int)testimagelen);
                    memcpy(space+strlen((char *)space),testimage,testimagelen);
                    //printf("buffer.(%s)\n",buffer);
                    libwebsocket_write(wsi,space,strlen((char *)buffer)+testimagelen,LWS_WRITE_HTTP);
                    free(space);
                    lastmicro = microseconds();
                }
                return(-1);
            }
#endif
            if ( (nxtprotocol= get_NXTprotocol((char *)in)) != 0 )
            {
                char *retstr;
                retstr = NXTprotocol_json_handler(nxtprotocol,(char *)NXTprotocol_parms);
                //printf("GOT.(%s) for (%s)\n",retstr,(char *)in);
                if ( retstr != 0 )
                {
                    len = strlen(retstr);
                    sprintf((char *)buffer,
                            "HTTP/1.0 200 OK\x0d\x0a"
                            "Server: NXTprotocol.jl777\x0d\x0a"
                            "Content-Type: text/html\x0d\x0a"
                            "Access-Control-Allow-Origin: *\x0d\x0a"
                            "Content-Length: %u\x0d\x0a\x0d\x0a",
                            (unsigned int)len);
                    //printf("html hdr.(%s)\n",buffer);
                    libwebsocket_write(wsi,buffer,strlen((char *)buffer),LWS_WRITE_HTTP);
                    libwebsocket_write(wsi,(unsigned char *)retstr,len,LWS_WRITE_HTTP);
                }
                return(-1);
            }
            else
            {
                /*static char *fileptr; static int64_t allocsize;
                if ( 1 && load_file(NXTPROTOCOL_HTMLFILE,&fileptr,&mylen,&allocsize) != 0 )
                {
                    printf("loaded NXTprotocol, len.%ld + forms.%ld\n",(long)mylen,strlen(testforms));
                    if ( URL_changed == 0 )
                        len = mylen + strlen(testforms);
                    else len = mylen;
                    sprintf((char *)buffer,
                            "HTTP/1.0 200 OK\x0d\x0a"
                            "Server: NXTprotocol.jl777\x0d\x0a"
                            "Content-Type: text/html\x0d\x0a"
                            "Access-Control-Allow-Origin: *\x0d\x0a"
                            "Content-Length: %u\x0d\x0a\x0d\x0a",
                            (unsigned int)len);
                    printf("html hdr.(%s)\n",buffer);
                    libwebsocket_write(wsi,buffer,strlen((char *)buffer),LWS_WRITE_HTTP);
                    if ( URL_changed == 0 && testforms[0] != 0 )
                       libwebsocket_write(wsi,(unsigned char *)testforms,strlen(testforms),LWS_WRITE_HTTP);
                    libwebsocket_write(wsi,(unsigned char *)fileptr,mylen,LWS_WRITE_HTTP);
                    return(-1);
                }
                else*/
                    if ( URL_changed == 0 && testforms[0] != 0 )
                {
                    mylen = strlen(testforms);
                    //printf("testforms len %ld\n",(long)mylen);
                    if ( mylen != 0 )
                    {
                        sprintf((char *)buffer,
                                "HTTP/1.0 200 OK\x0d\x0a"
                                "Server: NXTprotocol.jl777\x0d\x0a"
                                "Content-Type: text/html\x0d\x0a"
                                "Access-Control-Allow-Origin: *\x0d\x0a"
                                "Content-Length: %u\x0d\x0a\x0d\x0a",
                                (unsigned int)mylen);
                        libwebsocket_write(wsi,buffer,strlen((char *)buffer),LWS_WRITE_HTTP);
                        //printf("html hdr.(%s)\n",buffer);
                        libwebsocket_write(wsi,(unsigned char *)testforms,strlen(testforms),LWS_WRITE_HTTP);
                        return(-1);
                    }
                }
            }

		// if not, send a file the easy way
		strcpy(buf, resource_path);
		if ( strcmp(in, "/") != 0)
        {
			if ( *((const char *)in) != '/' )
				strcat(buf, "/");
			strncat(buf,in,sizeof(buf) - strlen(resource_path));
		} else // default file to serve 
			strcat(buf, "/NXTprotocol.html");
		buf[sizeof(buf) - 1] = '\0';

		// refuse to serve files we don't understand 
		mimetype = get_mimetype(buf);
		if ( mimetype == 0 )
        {
			lwsl_err("Unknown mimetype for %s\n", buf);
			libwebsockets_return_http_status(context, wsi,HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, NULL);
			return -1;
		}
            printf("mimetype.%s\n",mimetype);
		other_headers = NULL;
		if ( strcmp((const char *)in, "/") == 0 && lws_hdr_total_length(wsi,WSI_TOKEN_HTTP_COOKIE) == 0 )
        {
			// this isn't very unguessable but it'll do for us 
			gettimeofday(&tv, NULL);
			sprintf(b64, "LWS_%u_%u_COOKIE",(unsigned int)tv.tv_sec,(unsigned int)tv.tv_usec);
			sprintf(leaf_path,"Set-Cookie: test=LWS_%u_%u_COOKIE;Max-Age=360000\x0d\x0a",(unsigned int)tv.tv_sec,(unsigned int)tv.tv_usec);
			other_headers = leaf_path;
			lwsl_err(other_headers);
		}
		if ( libwebsockets_serve_http_file(context,wsi,buf,mimetype,other_headers) != 0 )
			return -1; // through completion or error, close the socket 
//notice that the sending of the file completes async, we'll get a LWS_CALLBACK_HTTP_FILE_COMPLETION callback when it's done
		break;
	case LWS_CALLBACK_HTTP_BODY:
		strncpy(buf,in,20);
		buf[20] = '\0';
		if ( len < 20 )
			buf[len] = '\0';
		lwsl_notice("LWS_CALLBACK_HTTP_BODY: %s... len %d\n",(const char *)buf,(int32_t)len);
		break;
        case LWS_CALLBACK_HTTP_BODY_COMPLETION: // the whole sent body arried, close the connection

		lwsl_notice("LWS_CALLBACK_HTTP_BODY_COMPLETION\n");
		libwebsockets_return_http_status(context, wsi,HTTP_STATUS_OK, NULL);
		return -1;
	case LWS_CALLBACK_HTTP_FILE_COMPLETION:     // kill the connection after we sent one file
//		lwsl_info("LWS_CALLBACK_HTTP_FILE_COMPLETION seen\n");
		 
		return -1;
	case LWS_CALLBACK_HTTP_WRITEABLE:           // we can send more of whatever it is we were sending
		/*do
        {
			n = (int32_t)read(pss->fd,buffer,sizeof buffer);
			if ( n < 0 ) // problem reading, close conn
				goto bail;
			if ( n == 0 ) // sent it all, close conn
				goto flush_bail;
			// because it's HTTP and not websocket, don't need to take care about pre and postamble
			m = libwebsocket_write(wsi,buffer,n,LWS_WRITE_HTTP);
			if ( m < 0 ) // write failed, close conn
				goto bail;
			if ( m != n ) // partial write, adjust
				lseek(pss->fd,m - n,SEEK_CUR);
		} while ( lws_send_pipe_choked(wsi) == 0 );*/
        //if ( testimagelen != 0 )
        //    libwebsocket_write(wsi,(unsigned char *)testimage,testimagelen,LWS_WRITE_HTTP);

		libwebsocket_callback_on_writable(context,wsi);
		break;
flush_bail:
		if ( lws_send_pipe_choked(wsi) == 0 )   // true if still partial pending
        {
			libwebsocket_callback_on_writable(context, wsi);
			break;
		}
bail:
		close(pss->fd);
		return -1;
	/*
	 * callback for confirming to continue with client IP appear in
	 * protocol 0 callback since no websocket protocol has been agreed
	 * yet.  You can just ignore this if you won't filter on client IP
	 * since the default uhandled callback return is 0 meaning let the
	 * connection continue.
	 */
	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
#if 0
		libwebsockets_get_peer_addresses(context, wsi, (int32_t)(long)in, client_name,sizeof(client_name), client_ip, sizeof(client_ip));
		fprintf(stderr, "Received network connect from %s (%s)\n",client_name, client_ip);
#endif
		// if we returned non-zero from here, we kill the connection 
		break;
#ifdef EXTERNAL_POLL
	//callbacks for managing the external poll() array appear in protocol 0 callback
	case LWS_CALLBACK_LOCK_POLL:
		// lock mutex to protect pollfd state called before any other POLL related callback
		break;
	case LWS_CALLBACK_UNLOCK_POLL:
		// unlock mutex to protect pollfd state when called after any other POLL related callback
		break;
	case LWS_CALLBACK_ADD_POLL_FD:
		if ( count_pollfds >= max_poll_elements )
        {
			lwsl_err("LWS_CALLBACK_ADD_POLL_FD: too many sockets to track\n");
			return 1;
		}
		fd_lookup[pa->fd] = count_pollfds;
		pollfds[count_pollfds].fd = pa->fd;
		pollfds[count_pollfds].events = pa->events;
		pollfds[count_pollfds++].revents = 0;
		break;
	case LWS_CALLBACK_DEL_POLL_FD:
		if ( --count_pollfds <= 0 )
			break;
		m = fd_lookup[pa->fd];
		// have the last guy take up the vacant slot 
		pollfds[m] = pollfds[count_pollfds];
		fd_lookup[pollfds[count_pollfds].fd] = m;
		break;
	case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
	        pollfds[fd_lookup[pa->fd]].events = pa->events;
		break;
#endif
	case LWS_CALLBACK_GET_THREAD_ID:
		/*
		 * if you will call "libwebsocket_callback_on_writable"
		 * from a different thread, return the caller thread ID
		 * here so lws can use this information to work out if it
		 * should signal the poll() loop to exit and restart early
		 */
		/* return pthread_getthreadid_np(); */
		break;
	default:
		break;
	}
	return 0;
}


/* dumb_increment protocol */

/*
 * one of these is auto-created for each connection and a pointer to the
 * appropriate instance is passed to the callback in the user parameter
 *
 * for this example protocol we use it to individualize the count for each
 * connection.
 */

struct per_session_data__dumb_increment {
	int number;
};


static int
callback_dumb_increment(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
	int32_t n, m;
	struct per_session_data__dumb_increment *pss = (struct per_session_data__dumb_increment *)user;

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		lwsl_info("callback_dumb_increment: "
						 "LWS_CALLBACK_ESTABLISHED\n");
		pss->number = 0;
		break;
  
	case LWS_CALLBACK_SERVER_WRITEABLE:
            if ( 1 )//0 && testimagelen == 0 )
            {
                n = (int32_t)strlen((char *)dispstr);
                if ( n > 0 )
                {
                    m = libwebsocket_write(wsi, (unsigned char *)dispstr, n, LWS_WRITE_TEXT);
                    //printf("wrote (%s).%d to wsi, got %d\n",dispstr,n,m);
                    if (m < n) {
                        lwsl_err("ERROR %d writing to di socket\n", n);
                        return -1;
                    }
                }
            }
            else
            {
                unsigned char buffer[512];
                sprintf((char *)buffer,
                        "HTTP/1.0 200 OK\x0d\x0a"
                        "Server: libwebsockets\x0d\x0a"
                        "Content-Type: image/jpeg\x0d\x0a"
                        "Content-Length: %u\x0d\x0a\x0d\x0a",
                        (unsigned int)testimagelen);
                printf("buffer.(%s)\n",buffer);
                libwebsocket_write(wsi,buffer,strlen((char *)buffer),LWS_WRITE_HTTP);
                libwebsocket_write(wsi,(unsigned char *)testimage,testimagelen,LWS_WRITE_HTTP);
            }
            break;

	case LWS_CALLBACK_RECEIVE:
//		fprintf(stderr, "rx %d\n", (int32_t)len);
		//if (len < 6)
		//	break;
            printf("button.(%s)\n",(char *)in);
            if (strcmp((const char *)in, "reset\n") == 0)
                pss->number = 10000000;
            else if (strcmp((const char *)in, "big\n") == 0)
                pss->number = 0;
		break;
	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info(wsi);
		/* you could return non-zero here and kill the connection */
		break;

	default:
		break;
	}

	return 0;
}


/* lws-mirror_protocol */

#define MAX_MESSAGE_QUEUE 32

struct per_session_data__lws_mirror {
	struct libwebsocket *wsi;
	int ringbuffer_tail;
};

struct a_message {
	void *payload;
	size_t len;
};

static struct a_message ringbuffer[MAX_MESSAGE_QUEUE];
static int ringbuffer_head;

static int
callback_lws_mirror(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
	int n;
	struct per_session_data__lws_mirror *pss = (struct per_session_data__lws_mirror *)user;

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		lwsl_info("callback_lws_mirror: LWS_CALLBACK_ESTABLISHED\n");
		pss->ringbuffer_tail = ringbuffer_head;
		pss->wsi = wsi;
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		lwsl_notice("mirror protocol cleaning up\n");
		for (n = 0; n < sizeof ringbuffer / sizeof ringbuffer[0]; n++)
			if (ringbuffer[n].payload)
				free(ringbuffer[n].payload);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		if (close_testing)
			break;
		while (pss->ringbuffer_tail != ringbuffer_head) {

			n = libwebsocket_write(wsi, (unsigned char *)
				   ringbuffer[pss->ringbuffer_tail].payload +
				   LWS_SEND_BUFFER_PRE_PADDING,
				   ringbuffer[pss->ringbuffer_tail].len,
								LWS_WRITE_TEXT);
			if (n < 0) {
				lwsl_err("ERROR %d writing to mirror socket\n", n);
				return -1;
			}
			if (n < ringbuffer[pss->ringbuffer_tail].len)
				lwsl_err("mirror partial write %d vs %d\n",
				       n, ringbuffer[pss->ringbuffer_tail].len);

			if (pss->ringbuffer_tail == (MAX_MESSAGE_QUEUE - 1))
				pss->ringbuffer_tail = 0;
			else
				pss->ringbuffer_tail++;

			if (((ringbuffer_head - pss->ringbuffer_tail) &
				  (MAX_MESSAGE_QUEUE - 1)) == (MAX_MESSAGE_QUEUE - 15))
				libwebsocket_rx_flow_allow_all_protocol(
					       libwebsockets_get_protocol(wsi));

			// lwsl_debug("tx fifo %d\n", (ringbuffer_head - pss->ringbuffer_tail) & (MAX_MESSAGE_QUEUE - 1));

			if (lws_send_pipe_choked(wsi)) {
				libwebsocket_callback_on_writable(context, wsi);
				break;
			}
			/*
			 * for tests with chrome on same machine as client and
			 * server, this is needed to stop chrome choking
			 */
#ifdef _WIN32
			Sleep(1);
#else
			usleep(1);
#endif
		}
		break;

	case LWS_CALLBACK_RECEIVE:

		if (((ringbuffer_head - pss->ringbuffer_tail) &
				  (MAX_MESSAGE_QUEUE - 1)) == (MAX_MESSAGE_QUEUE - 1)) {
			lwsl_err("dropping!\n");
			goto choke;
		}

		if (ringbuffer[ringbuffer_head].payload)
			free(ringbuffer[ringbuffer_head].payload);

		ringbuffer[ringbuffer_head].payload =
				malloc(LWS_SEND_BUFFER_PRE_PADDING + len +
						  LWS_SEND_BUFFER_POST_PADDING);
		ringbuffer[ringbuffer_head].len = len;
		memcpy((char *)ringbuffer[ringbuffer_head].payload +
					  LWS_SEND_BUFFER_PRE_PADDING, in, len);
		if (ringbuffer_head == (MAX_MESSAGE_QUEUE - 1))
			ringbuffer_head = 0;
		else
			ringbuffer_head++;

		if (((ringbuffer_head - pss->ringbuffer_tail) &
				  (MAX_MESSAGE_QUEUE - 1)) != (MAX_MESSAGE_QUEUE - 2))
			goto done;

choke:
		lwsl_debug("LWS_CALLBACK_RECEIVE: throttling %p\n", wsi);
		libwebsocket_rx_flow_control(wsi, 0);

//		lwsl_debug("rx fifo %d\n", (ringbuffer_head - pss->ringbuffer_tail) & (MAX_MESSAGE_QUEUE - 1));
done:
		libwebsocket_callback_on_writable_all_protocol(
					       libwebsockets_get_protocol(wsi));
		break;

	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info(wsi);
		/* you could return non-zero here and kill the connection */
		break;

	default:
		break;
	}

	return 0;
}


/* list of supported protocols and callbacks */

static struct libwebsocket_protocols protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",		/* name */
		callback_http,		/* callback */
		sizeof (struct per_session_data__http),	/* per_session_data_size */
		0,			/* max frame size / rx buffer */
	},
	{
		"dumb-increment-protocol",
		callback_dumb_increment,
		sizeof(struct per_session_data__dumb_increment),
		65536,
	},
	{
		"lws-mirror-protocol",
		callback_lws_mirror,
		sizeof(struct per_session_data__lws_mirror),
		128,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

void sighandler(int sig)
{
	force_exit = 1;
	libwebsocket_cancel_service(context);
}

struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "debug",	required_argument,	NULL, 'd' },
	{ "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "allow-non-ssl",	no_argument,		NULL, 'a' },
	{ "interface",  required_argument,	NULL, 'i' },
	{ "closetest",  no_argument,		NULL, 'c' },
#ifndef LWS_NO_DAEMONIZE
	{ "daemonize", 	no_argument,		NULL, 'D' },
#endif
	{ "resource_path", required_argument,		NULL, 'r' },
	{ NULL, 0, 0, 0 }
};

char *libjl777_JSON(char *JSONstr)
{
    cJSON *json;
    char *retstr = 0;
    if ( (json= cJSON_Parse(JSONstr)) != 0 )
    {
        retstr = pNXT_jsonhandler(&json,JSONstr,0);
        free_json(json);
    }
    if ( retstr == 0 )
        retstr = clonestr("{\"result\":null}");
    return(retstr);
}

uint64_t call_libjl777_broadcast(char *msg,int32_t duration)
{
    int32_t libjl777_broadcast(char *msg,int32_t duration);
    unsigned char hash[256>>3];
    uint64_t txid;
    calc_sha256(0,hash,(uint8_t *)msg,(int32_t)strlen(msg));
    txid = calc_txid(hash,sizeof(hash));
    if ( libjl777_broadcast(msg,duration) == 0 )
        return(txid);
    else return(0);
}

uint64_t broadcast_publishpacket(struct NXT_acct *np,char *NXTACCTSECRET,char *srvNXTaddr,char *srvipaddr,uint16_t srvport)
{
    struct coin_info *cp;
    char cmd[1024],packet[2048],hexstr[512];
    int32_t len;
    init_hexbytes(hexstr,np->pubkey,sizeof(np->pubkey));
    if ( (cp= get_coin_info("BTCD")) != 0 && cp->pubnxt64bits != 0 )
    {
        sprintf(cmd,"{\"requestType\":\"publishaddrs\",\"srvipaddr\":\"%s\",\"srvport\":\"%d\",\"srvNXTaddr\":\"%s\",\"pubkey\":\"%s\",\"pubBTCD\":\"%s\",\"pubNXT\":\"%s\",\"pubBTC\":\"%s\"}",srvipaddr,srvport,srvNXTaddr,hexstr,np->BTCDaddr,np->H.NXTaddr,np->BTCaddr);
        len = construct_tokenized_req(packet,cmd,NXTACCTSECRET);
        return(call_libjl777_broadcast(packet,PUBADDRS_MSGDURATION));
    } else printf("broadcast_publishpacket error: no public nxt addr\n");
    return(-1);
}

char *libjl777_gotpacket(char *msg,int32_t duration)
{
    cJSON *json;
    uint64_t txid;
    int32_t len,createdflag;
    unsigned char packet[4096],hash[256>>3];
    char txidstr[64];
    char retjsonstr[4096],*retstr;
    uint64_t obookid;
    //display_orderbook_tx((struct orderbook_tx *)packet);
    //for (i=0; i<len; i++)
    //    printf("%02x ",packet[i]);
    len = (int32_t)strlen(msg);
    strcpy(retjsonstr,"{\"result\":null}");
    if ( is_hexstr(msg) != 0 )
    {
        len >>= 1;
        decode_hex(packet,len,msg);
        calc_sha256(0,hash,packet,len);
        txid = calc_txid(hash,sizeof(hash));
        sprintf(txidstr,"%llu",(long long)txid);
        MTadd_hashtable(&createdflag,&Global_pNXT->msg_txids,txidstr);
        if ( createdflag == 0 )
            return(clonestr("{\"error\":\"duplicate msg\"}"));
        printf("C libjl777_gotpacket.%s size.%d hexencoded txid.%llu\n",msg,len<<1,(long long)txid);
        if ( is_encrypted_packet(packet,len) != 0 )
            process_packet(retjsonstr,0,0,packet,len,0,0,0,0,0);
        else if ( (obookid= is_orderbook_tx(packet,len)) != 0 )
        {
            if ( update_orderbook_tx(1,obookid,(struct orderbook_tx *)packet,txid) == 0 )
            {
                ((struct orderbook_tx *)packet)->txid = txid;
                sprintf(retjsonstr,"{\"result\":\"libjl777_gotpacket got obbokid.%llu packet txid.%llu\"}",(long long)obookid,(long long)txid);
            } else sprintf(retjsonstr,"{\"result\":\"libjl777_gotpacket error updating obookid.%llu\"}",(long long)obookid);
        } else sprintf(retjsonstr,"{\"error\":\"libjl777_gotpacket cant find obookid\"}");
    }
    else
    {
        calc_sha256(0,hash,(uint8_t *)msg,len);
        txid = calc_txid(hash,sizeof(hash));
        sprintf(txidstr,"%llu",(long long)txid);
        MTadd_hashtable(&createdflag,&Global_pNXT->msg_txids,txidstr);
        if ( createdflag == 0 )
            return(clonestr("{\"error\":\"duplicate msg\"}"));
        printf("C libjl777_gotpacket.%s size.%d ascii txid.%llu\n",msg,len,(long long)txid);
        if ( (json= cJSON_Parse((char *)packet)) != 0 )
        {
            retstr = pNXT_jsonhandler(&json,(char *)packet,0);
            free_json(json);
            if ( retstr != 0 )
                return(retstr);
        }
    }
    return(clonestr(retjsonstr));
}

void *libjl777_threads(void *arg)
{
    char *JSON_or_fname = arg;
	char cert_path[1024];
	char key_path[1024];
	int n = 0;
	int use_ssl = 0;
	int opts = 0;
	const char *iface = NULL;
#ifndef WIN32
	int syslog_options = LOG_PID | LOG_PERROR;
#endif
	unsigned int oldus = 0;
	struct lws_context_creation_info info;

	int debug_level = 7;
#ifndef LWS_NO_DAEMONIZE
	int daemonize = 0;
#endif
    init_NXTservices(JSON_or_fname);
	memset(&info, 0, sizeof info);
	info.port = LIBWEBSOCKETS_PORT;

	/*while (n >= 0) {
		n = getopt_long(argc, argv, "ci:hsap:d:Dr:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
#ifndef LWS_NO_DAEMONIZE
		case 'D':
			daemonize = 1;
			#ifndef WIN32
			syslog_options &= ~LOG_PERROR;
			#endif
			break;
#endif
		case 'd':
			debug_level = atoi(optarg);
			break;
		case 's':
			use_ssl = 1;
			break;
		case 'a':
			opts |= LWS_SERVER_OPTION_ALLOW_NON_SSL_ON_SSL_PORT;
			break;
		case 'p':
			info.port = atoi(optarg);
			break;
		case 'i':
			strncpy(interface_name, optarg, sizeof interface_name);
			interface_name[(sizeof interface_name) - 1] = '\0';
			iface = interface_name;
			break;
		case 'c':
			close_testing = 1;
			fprintf(stderr, " Close testing mode -- closes on "
					   "client after 50 dumb increments"
					   "and suppresses lws_mirror spam\n");
			break;
		case 'r':
			resource_path = optarg;
			printf("Setting resource path to \"%s\"\n", resource_path);
			break;
		case 'h':
			fprintf(stderr, "Usage: test-server "
					"[--port=<p>] [--ssl] "
					"[-d <log bitfield>] "
					"[--resource_path <path>]\n");
			exit(1);
		}
	}*/

#if !defined(LWS_NO_DAEMONIZE) && !defined(WIN32)
	/* 
	 * normally lock path would be /var/lock/lwsts or similar, to
	 * simplify getting started without having to take care about
	 * permissions or running as root, set to /tmp/.lwsts-lock
	 */
	if (daemonize && lws_daemonize("/tmp/.lwsts-lock")) {
		fprintf(stderr, "Failed to daemonize\n");
		return(0);
	}
#endif

	signal(SIGINT, sighandler);

#ifndef WIN32
	/* we will only try to log things according to our debug_level */
	setlogmask(LOG_UPTO (LOG_DEBUG));
	openlog("lwsts", syslog_options, LOG_DAEMON);
#endif
	/* tell the library what debug level to emit and to send it to syslog */
	lws_set_log_level(debug_level, lwsl_emit_syslog);
	lwsl_notice("libwebsockets test server - "
			"(C) Copyright 2010-2013 Andy Green <andy@warmcat.com> - "
						    "licensed under LGPL2.1\n");
#ifdef EXTERNAL_POLL
	max_poll_elements = getdtablesize();
	pollfds = malloc(max_poll_elements * sizeof (struct pollfd));
	fd_lookup = malloc(max_poll_elements * sizeof (int32_t));
	if (pollfds == NULL || fd_lookup == NULL) {
		lwsl_err("Out of memory pollfds=%d\n", max_poll_elements);
		return(0);
	}
#endif

	info.iface = iface;
	info.protocols = protocols;
#ifndef LWS_NO_EXTENSIONS
	info.extensions = libwebsocket_get_internal_extensions();
#endif
	if (!use_ssl) {
		info.ssl_cert_filepath = NULL;
		info.ssl_private_key_filepath = NULL;
	} else {
		if (strlen(resource_path) > sizeof(cert_path) - 32) {
			lwsl_err("resource path too long\n");
            return(0);
		}
		sprintf(cert_path, "%s/libwebsockets-test-server.pem",
								resource_path);
		if (strlen(resource_path) > sizeof(key_path) - 32) {
			lwsl_err("resource path too long\n");
            return(0);
		}
		sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
								resource_path);

		info.ssl_cert_filepath = cert_path;
		info.ssl_private_key_filepath = key_path;
	}
	info.gid = -1;
	info.uid = -1;
	info.options = opts;

	context = libwebsocket_create_context(&info);
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return(0);
	}

	n = 0;
	while (n >= 0 && !force_exit)
    {
        static int started_UV;
		struct timeval tv;
        usleep(5000);
        if ( Finished_loading != 0 && started_UV == 0 )
        {
            printf("run_UVloop\n");
            if ( portable_thread_create(run_UVloop,Global_mp) == 0 )
                printf("ERROR hist process_hashtablequeues\n");
            started_UV = 1;
        }
 
		gettimeofday(&tv, NULL);

		/*
		 * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
		 * live websocket connection using the DUMB_INCREMENT protocol,
		 * as soon as it can take more packets (usually immediately)
		 */

		if (((unsigned int)tv.tv_usec - oldus) > LIBWEBSOCKETS_MILLIS*1000) {
			libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_DUMB_INCREMENT]);
			oldus = tv.tv_usec;
		}

#ifdef EXTERNAL_POLL

		/*
		 * this represents an existing server's single poll action
		 * which also includes libwebsocket sockets
		 */

		n = poll(pollfds, count_pollfds, LIBWEBSOCKETS_MILLIS);
		if (n < 0)
			continue;


		if (n)
			for (n = 0; n < count_pollfds; n++)
				if (pollfds[n].revents)
					/*
					* returns immediately if the fd does not
					* match anything under libwebsockets
					* control
					*/
					if (libwebsocket_service_fd(context,&pollfds[n]) < 0)
						goto done;
#else
		/*
		 * If libwebsockets sockets are all we care about,
		 * you can use this api which takes care of the poll()
		 * and looping through finding who needed service.
		 *
		 * If no socket needs service, it'll return anyway after
		 * the number of ms in the second argument.
		 */

		n = libwebsocket_service(context, LIBWEBSOCKETS_MILLIS);
#endif
	}
#ifdef EXTERNAL_POLL
done:
#endif
	libwebsocket_context_destroy(context);
	lwsl_notice("libwebsockets-test-server exited cleanly\n");
#ifndef WIN32
	closelog();
#endif
	return 0;
}

int libjl777_start(char *JSON_or_fname)
{
    struct NXT_str *tp = 0;
    Global_mp = calloc(1,sizeof(*Global_mp));
    printf("libjl777_start(%s)\n",JSON_or_fname);
    curl_global_init(CURL_GLOBAL_ALL); //init the curl session
    if ( Global_pNXT == 0 )
    {
        Global_pNXT = calloc(1,sizeof(*Global_pNXT));
        orderbook_txids = hashtable_create("orderbook_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->txid[0] - (long)tp),sizeof(tp->txid),((long)&tp->modified - (long)tp));
        Global_pNXT->orderbook_txidsp = &orderbook_txids;
        Global_pNXT->msg_txids = hashtable_create("msg_txids",HASHTABLES_STARTSIZE,sizeof(struct NXT_str),((long)&tp->txid[0] - (long)tp),sizeof(tp->txid),((long)&tp->modified - (long)tp));
        printf("SET ORDERBOOK HASHTABLE %p\n",orderbook_txids);
    }
    if ( portable_thread_create(libjl777_threads,JSON_or_fname) == 0 )
        printf("ERROR libjl777_threads\n");
        return(0);
}
