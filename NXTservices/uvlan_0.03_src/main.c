#include "uvlan.h"

#ifdef WIN32
#else
 #include <arpa/inet.h>
 #include <sys/types.h>
#endif

#include <sys/stat.h>
#include <unistd.h>

static const char *uvlan_cvsid = "$id$";

prng_state uvlan_prng;
static ecc_key uvlan_prikey, tmppub;
static pcap_t             *pcap_handle;
static int                 net_handle;

static uvlan_port          ports[UVLAN_MAX];
static int                 port_no;

static volatile ulong64    stats_in_eth[2],
                           stats_in_udp[2],
                           stats_out_udp[2];
volatile int               nagle_delay=0, stats_mode=0,debug_level=0,heartbeat_delay=0,bound_sock;
static char *stats_file=NULL;

static const unsigned char bc[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  
static const unsigned char noaddr[4] = {0,0,0,0};
static const unsigned char ports6867[] = {00, 68, 00, 67};

static int isIpPacket(unsigned char *buf) {
  // if ethertype == 0800 and if there is actually an ip addr
  return buf[12]==0x08 && buf[13]==0x00 && (memcmp(buf+26, noaddr, 4) != 0);
}

/* handle getting local traffic */
void *pcap_thread(void *d)
{
   struct pcap_pkthdr *header;                        /* The header that pcap gives us */
   u_char             *packet;                        /* The actual packet */
   int                 src_port, dst_port, x, y;
   uvlan_host_entry    ent;   
   uvlan_tree         *src_o, *dst_o;
   int                 broad;

   for (;;) {
      // read packet
      pcap_next_ex(pcap_handle, &header, (const u_char **)&packet);
      broad = 0;
      
      stats_in_eth[0] += header->len;

      // find port for src
      src_o = tree_find(packet+6);

      // if no port, add to local side
      if (src_o == NULL) { //UVLAN_TREE_NOT_FOUND) {
         memcpy(ent.addr, packet+6, 6);
         ent.port = UVLAN_PORT_LOCAL;
         memset(ent.bytes_in, 0, sizeof(ent.bytes_in));
         memset(ent.bytes_out, 0, sizeof(ent.bytes_out));
         if (isIpPacket(packet)) memcpy(ent.ipaddr, packet+26, 4);
         else                    memset(ent.ipaddr, 0, 4);
         tree_add(&ent);

         src_port = UVLAN_PORT_LOCAL;
      } else {
         src_port = src_o->ent.port;
         src_o->ent.bytes_in[0] += header->len;
      }

      // if broadcast, reject if source is not local
      if (memcmp(packet, bc, 6) == 0) {
         broad = 1;
      }
      

      // DHCP request: broadcast MAC, broadcast IP, ports 68&67
      if (broad
        &&memcmp(packet+30, bc, 4) == 0
        &&memcmp(packet+34, ports6867, 4) == 0) {
        if (debug_level>9) fprintf(stderr, "Remote DHCP broadcast rejected\n");
        continue;
      }

      if (broad && src_port != UVLAN_PORT_LOCAL) {
         if (debug_level>9) fprintf(stderr, "Remote broadcast rejected\n");
         continue;
      }
         
      // if destine to a local port, reject
      dst_o = tree_find(packet);
      dst_port = dst_o==NULL ? UVLAN_TREE_NOT_FOUND : dst_o->ent.port;
      if (broad == 0 && dst_port == UVLAN_PORT_LOCAL) {
         if (debug_level>9) fprintf(stderr, "Local packet rejected\n");
         continue;
      }
         
      // send out to port(s) that is(are) ready
      if (broad) {
         // if broadcast or unknown send to all
         if (debug_level) { 
            fprintf(stderr, "(out) Routing to %02x:%02x:%02x:%02x:%02x:%02x ", 
                   (unsigned)(packet[0]),(unsigned)(packet[1]), (unsigned)(packet[2]),
                   (unsigned)(packet[3]),(unsigned)(packet[4]),(unsigned)(packet[5])); 
            fprintf(stderr, "from %02x:%02x:%02x:%02x:%02x:%02x\n", 
                   (unsigned)(packet[6]),(unsigned)(packet[7]), (unsigned)(packet[8]),
                   (unsigned)(packet[9]),(unsigned)(packet[10]),(unsigned)(packet[11])); 
         }   
         
         for (x = 0; x < port_no; x++) {
            pthread_mutex_lock(&ports[x].lock);
            if (ports[x].cur_mode == UVLAN_MODE_SEND_RECV) {
               if (dst_o != NULL)
                  dst_o->ent.bytes_out[0] += header->len;
               stats_out_udp[0] += header->len;
               if ((y = crypto_send(&ports[x], packet, header->len)) != UVLAN_READ_OK) {
                  // failed to send
               }
            }
            pthread_mutex_unlock(&ports[x].lock);
         }
      } else if (dst_port != UVLAN_TREE_NOT_FOUND  &&  dst_port != UVLAN_PORT_LOCAL) {
         pthread_mutex_lock(&ports[dst_port].lock);
         if (debug_level) { 
            fprintf(stderr, "(out) Routing to %02x:%02x:%02x:%02x:%02x:%02x ", 
                   (unsigned)(packet[0]),(unsigned)(packet[1]), (unsigned)(packet[2]),
                   (unsigned)(packet[3]),(unsigned)(packet[4]),(unsigned)(packet[5])); 
            fprintf(stderr, "from %02x:%02x:%02x:%02x:%02x:%02x\n", 
                   (unsigned)(packet[6]),(unsigned)(packet[7]), (unsigned)(packet[8]),
                   (unsigned)(packet[9]),(unsigned)(packet[10]),(unsigned)(packet[11])); 
         }   
         
         if (ports[dst_port].cur_mode == UVLAN_MODE_SEND_RECV) {
            if (dst_o != NULL)
               dst_o->ent.bytes_out[0] += header->len;
            stats_out_udp[0] += header->len;
            if ((y = crypto_send(&ports[dst_port], packet, header->len)) != UVLAN_READ_OK) {
               // failed to send
            }
         }
         pthread_mutex_unlock(&ports[dst_port].lock);
      } else {
         // no route
         if (debug_level) { 
            fprintf(stderr, "No route for %02x:%02x:%02x:%02x:%02x:%02x\n", 
                   (unsigned)(packet[6]),(unsigned)(packet[7]), (unsigned)(packet[8]),
                   (unsigned)(packet[9]),(unsigned)(packet[10]),(unsigned)(packet[11])); 
         }   
      }
   }
   return NULL;
}

// http://www.networksorcery.com/enp/protocol/ip.htm#Protocol
const char* protos(const unsigned char *buf) {
  if (buf[6+6] == 0x08  && buf[6+6+1] == 0x00) {
    return "IP-...";
  }
  return "UNK-UNK";
}

void route_frame(int port, unsigned char *buf, int buflen)
{
   uvlan_host_entry ent;
   uvlan_tree *o;
   int y;
   
   memcpy(ent.addr, buf+6, 6);
   if (isIpPacket(buf)) memcpy(ent.ipaddr, buf+26, 4);
   else                 memset(ent.ipaddr, 0, 4);
   ent.port = port;
   memset(ent.bytes_in,0, sizeof(ent.bytes_in));
   memset(ent.bytes_out,0, sizeof(ent.bytes_out));
   o = tree_add(&ent);
   o->ent.bytes_in[0] += buflen;
   o->ent.portptr = o->ent.port==UVLAN_PORT_LOCAL ? NULL : &ports[port];

   /* only write to broadcast and local hosts */ 
   o = tree_find(buf);
   if (!memcmp(buf, bc, 6) || o->ent.port == UVLAN_PORT_LOCAL) {  
      #ifdef WIN32
      y = pcap_sendpacket(pcap_handle, buf, buflen);
      #else
      y = write(net_handle, buf, buflen);
      #endif
      if (debug_level) fprintf(stderr, "Wrote %d(%d) bytes to line h=%.8x %p\n", y, buflen, net_handle, pcap_handle);
      if (debug_level) { 
         fprintf(stderr, "(in)Routing to %02x:%02x:%02x:%02x:%02x:%02x ", 
                (unsigned)(buf[0]),(unsigned)(buf[1]), (unsigned)(buf[2]),
                (unsigned)(buf[3]),(unsigned)(buf[4]),(unsigned)(buf[5])); 
         fprintf(stderr, "from %02x:%02x:%02x:%02x:%02x:%02x\n", 
                (unsigned)(buf[6]),(unsigned)(buf[7]), (unsigned)(buf[8]),
                (unsigned)(buf[9]),(unsigned)(buf[10]),(unsigned)(buf[11])); 
      }   
   } else {
      if (debug_level) fprintf(stderr, "Rejecting remote packet that doesn't appear to be for us.\n");
   }
}

void route_frames(int port, unsigned char *buf, int buflen)
{
   int x, y;
   for (x = 0; x < buflen; ) {
      y = ((unsigned)buf[x+0]<<8)+(unsigned)buf[x+1];
      if (y > LISTEN_SIZE) {
         if (debug_level) fprintf(stderr, "Dumping huge frame (this indicates a software error btw)...\n");
         break;
      }
      if (debug_level) fprintf(stderr, "Routing frame of %d bytes\n", y);
      route_frame(port, buf+x+2, y);
      x += 2 + y;
   }
}

/* handle receiving UDP traffic */
void *udp_thread(void *d)
{
   unsigned char buf[4096], outbuf[4096];
   int           x, y, len, outlen;
   struct sockaddr_in sin;
   socklen_t          sinlen;
    
   for (;;) {
      // read packet
      sinlen = sizeof(sin);
      len = x = recvfrom(bound_sock, buf, sizeof(buf), 0, (struct sockaddr *)&sin, &sinlen);
      if (x < 0) {
         perror("recvfrom");
         usleep(500);
      }
         
      stats_in_udp[0] += len;
          
      // find out which port this belongs to
      for (y=0; y<port_no; y++) {
         if (ports[y].dest_sin.sin_addr.s_addr == sin.sin_addr.s_addr) {
            break;
         }
      }
         
      // reject if not found for a port 
      if (y == port_no) continue;
         
      if (debug_level)  fprintf(stderr, "Read in %d byte UDP packet on port %d, %d\n", len, y, ports[y].cur_mode);
         
      // switch on state
      pthread_mutex_lock(&ports[y].lock);
      switch (ports[y].cur_mode) {
         case UVLAN_MODE_SEND_RECV:
         // state 0: if valid, write to local (add source to tree for given port)
         //          if CTR failed reject
         //          if MAC failed makesendseed and goto state 3
         //          if type is SEED and MAC is valid then makesendseed, make key and goto state 0
            if (buf[0] == UVLAN_TYPE_PKT) {
               outlen = sizeof(outbuf);
               x = crypto_read(&ports[y], buf, len, outbuf, &outlen);
               if (x == UVLAN_FAIL_CTR || x == UVLAN_FAIL_SIZE) {
                  pthread_mutex_unlock(&ports[y].lock);
                  continue;
               } else if (x == UVLAN_FAIL_MAC) {
                  if ((x = crypto_make_send_seed(&ports[y])) != UVLAN_READ_OK) {
                     // error sending seed
                  }
                  ports[y].cur_mode = UVLAN_MODE_MAKE_SEED_RECV_SEED;
                  ports[y].ticker   = time(NULL) + 5; 
                  ports[y].buf_len  = 0;
               } else {
                  // valid so lets route it 
                  route_frames(y, outbuf, outlen);
               }
            } else if (buf[0] == UVLAN_TYPE_SEED) {
               // it's a seed
               if ((x = crypto_read_seed(&ports[y], buf, len)) != UVLAN_READ_OK) {
                  // these are using master keys so if they fail it's cuz of a transmission fault, just reject
                  pthread_mutex_unlock(&ports[y].lock);
                  continue;
               } else {
                  // send new seed
                  if ((x = crypto_make_send_seed(&ports[y])) != UVLAN_READ_OK) {
                     // error sending seed
                  } else {                  
                     // make key 
                     crypto_make_key(&ports[y]);
                     ports[y].cur_mode = UVLAN_MODE_SEND_RECV;
                  }
               }   
            }
            break;

            
         case UVLAN_MODE_MAKE_SEED_RECV_SEED:
            if (buf[0] == UVLAN_TYPE_PKT || buf[0] == UVLAN_TYPE_HB) {
               // reject packets 
               pthread_mutex_unlock(&ports[y].lock);
               continue;
            } else if (buf[0] == UVLAN_TYPE_SEED) {
               // it's a seed
               if ((x = crypto_read_seed(&ports[y], buf, len)) != UVLAN_READ_OK) {
                  pthread_mutex_unlock(&ports[y].lock);
                  continue;
               } else {
                  // make key 
                  crypto_make_key(&ports[y]);
                  ports[y].cur_mode = UVLAN_MODE_SEND_RECV;
               }   
            }
            break;
      }
      pthread_mutex_unlock(&ports[y].lock);
   }
   return NULL;
}


void print_devs() {
   pcap_if_t *alldevs;
   pcap_if_t *d;
   int i=0;
   char errbuf[PCAP_ERRBUF_SIZE];

   /* Retrieve the device list */
   if (pcap_findalldevs(&alldevs, errbuf) == -1) {
     fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
     exit(1);
   }

   fprintf(stderr, "ETH Devs:\n");
   /* Print the list */
   for(d=alldevs; d; d=d->next) {
     fprintf(stderr, "%d. %s", ++i, d->name);
     if (d->description) fprintf(stderr, " (%s)\n", d->description);
     else                fprintf(stderr, " (No description available)\n");
   }
}

void read_conf(const char *fname)
{
   FILE           *in, *pubkey_out;
   char           *conf, buf[128], b64[1024], tmp[3], errbuf[PCAP_ERRBUF_SIZE], *nick, *dev, *protocol;
   xmlp            xml, *X=&xml, *peerxml;
   unsigned char   key[MAXBLOCKSIZE], *ptr;
   struct hostent *he;
   unsigned        x, ret;
   unsigned long   b64len, buflen, z;
   ulong32         addr;
   struct sockaddr_in sin;
   
   crypto_init();
   tree_init();
   print_devs();
   
   in = fopen(fname, "r");
   if (in == NULL) {
      snprintf(buf, sizeof(buf), "fopen:%s:", fname);
      perror(buf);
      exit(EXIT_FAILURE);
   }

   fseek(in, 1, SEEK_END);
   buflen = ftell(in);
   fseek(in, 0, SEEK_SET);
   
   conf = (char*)malloc(buflen+1);
   ret = fread(conf, 1, buflen, in);
   conf[ret] = '\0';
   fclose(in); in=NULL;

   xmlp_init(X);
   if ((ret=xmlp_read(X, conf)) != XMLP_OK) {
     fprintf(stderr, "Couldn't parse config file: %s.\n", xmlp_error_to_string(ret));
     exit(EXIT_FAILURE);
   }

   /* get device name */
   dev = X->g(X->g(X->g(X,"uvlanConf")->down,"local")->down,"dev")->data;
   
   /* Open the session in promiscuous mode */
#ifdef WIN32
   pcap_handle = pcap_open_live(dev, LISTEN_SIZE, 1, 1, errbuf);
#else
   pcap_handle = pcap_open_live(dev, LISTEN_SIZE, 1, 0, errbuf);
#endif
   if (pcap_handle == NULL) {
      fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
      exit(EXIT_FAILURE);
   }
   net_handle = pcap_fileno(pcap_handle);
   
   /* get local bindings */
   memset(&sin, 0, sizeof(sin));
   sin.sin_family = AF_INET;
   /* get address */
   sin.sin_addr.s_addr = inet_addr( X->g(X->g(X->g(X,"uvlanConf")->down,"local")->down,"bindipaddr")->data );
   /* get port */
   sin.sin_port        = htons(atoi( X->g(X->g(X->g(X,"uvlanConf")->down,"local")->down,"bindport")->data ));
   protocol = X->g(X->g(X->g(X,"uvlanConf")->down,"local")->down,"protocol")->data;
   if (strcmp(protocol, "UDP") != 0) {
      fprintf(stderr, "Only supported protocol is UDP, %s invalid\n", protocol);
      exit(EXIT_FAILURE);
   }
   
   /* make socket and bind it */
   bound_sock = socket(AF_INET, SOCK_DGRAM, 0);  
   if (bound_sock < 0) {
      perror("socket");
      exit(EXIT_FAILURE);
   }
   if (bind(bound_sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      perror("bind");
      exit(EXIT_FAILURE);
   }

   /* read pri-key */
   ptr = X->g(X->g(X->g(X,"uvlanConf")->down,"local")->down,"privateKey")->data;
   b64len = sizeof(b64);
   if ((ret=base64_decode(ptr, strlen(ptr), b64, &b64len)) != CRYPT_OK) {
      fprintf(stderr, "Error decoding private key, ret=%s!\n", error_to_string(ret));
      exit(EXIT_FAILURE);
   }

   if ((ret=ecc_import(b64, b64len, &uvlan_prikey)) != CRYPT_OK) {
      fprintf(stderr, "Error importing private key, ret=%s!\n", error_to_string(ret));
      exit(EXIT_FAILURE);
   }

   /* export pubkey every start-up */
   if ((pubkey_out=fopen("uvlan_pub.txt", "wb")) == NULL) {
      fprintf(stderr, "Error opening public-key file: %s\n", error_to_string(ret));
      exit(EXIT_FAILURE);
   }

   buflen = sizeof(buf);
   if ((ret=ecc_export(buf, &buflen, PK_PUBLIC, &uvlan_prikey)) != CRYPT_OK) {
      fprintf(stderr, "Error exporting public-key: %s\n", error_to_string(ret));
      exit(EXIT_FAILURE);
   }

   b64len = sizeof(b64);
   if ((ret=base64_encode(buf, buflen, b64, &b64len)) != CRYPT_OK) {
      fprintf(stderr, "Error encoding public-key: %s\n", error_to_string(ret));
      exit(EXIT_FAILURE);
   }
   fwrite(b64, 1, b64len, pubkey_out);
   fclose(pubkey_out);

      
   /* now read the hosts */
   for (x=0; x<UVLAN_MAX; x++) {
      /* get the nickname for this peer */
      peerxml = X->gn(X->g(X,"uvlanConf")->down,"peer",x);
      if (peerxml->notfound)
        break;

      nick = X->g(peerxml->down,"nickname")->data;

      /* get the host/ip for this peer */
      if ((addr = inet_addr( ptr=X->g(peerxml->down,"address")->data )) == 0xFFFFFFFF) {
         /* host name? */
         he = gethostbyname(ptr);
         if (he == NULL) {
            perror("gethostbyname");
            ret = CRYPT_OK+1;
            goto END_OF_FUNC;
         }
         LOAD32L(addr, he->h_addr);
      }

      memset(&ports[x].dest_sin, 0, sizeof(sin));
      ports[x].dest_sin.sin_addr.s_addr = addr;
      ports[x].dest_sin.sin_family      = AF_INET;
      /* get the port for this peer */
      ports[x].dest_sin.sin_port        = htons(atoi( X->g(peerxml->down,"port")->data ));
      protocol = X->g(peerxml->down,"protocol")->data;

      if (strcmp(protocol, "UDP") != 0) {
         fprintf(stderr, "Only supported protocol is UDP, %s invalid\n", protocol);
         exit(EXIT_FAILURE);
      }
         
      /* read pubkey for this peer */
      ptr = X->g(peerxml->down,"publicKey")->data;
      /* check if we're using ECC pubkey exchange */
      b64len = sizeof(b64);
      if ((ret=base64_decode(ptr, strlen(ptr), b64, &b64len)) != CRYPT_OK) {
         fprintf(stderr, "Error decoding public-key #%u, ret=%s!\n", x, error_to_string(ret));
         goto END_OF_FUNC;
      }
   
      if ((ret=ecc_import(b64, b64len, &tmppub)) != CRYPT_OK) {
         fprintf(stderr, "Error importing public-key #%u, ret=%s!\n", x, error_to_string(ret));
         goto END_OF_FUNC;
      }
   
      z = sizeof(key);
      if ((ret=ecc_shared_secret(&uvlan_prikey, &tmppub, key, &z)) != CRYPT_OK) {
         fprintf(stderr, "Error generating shared-secret #%u, ret=%s!\n", x, error_to_string(ret));
         goto END_OF_FUNC;
      }

      /* setup master key */
      crypto_init_port(&ports[x], key, 16);
      ports[x].buf_len = 0;
      ports[x].ticker  = 0;
      ports[x].nick = strdup(nick);
   }
   port_no = x;
   
END_OF_FUNC:
   xmlp_free(&xml);
   memset(key, 0, sizeof(key));
   memset(buf, 0, sizeof(buf));
   memset(tmp, 0, sizeof(tmp));
   if (ret != CRYPT_OK)
     exit(EXIT_FAILURE);
}

void *stats_thread(void *d)
{
   time_t now, last;
   double eth_rate, udpr_rate, udpw_rate;
   char *drawn_tree = NULL, total_stats[1024];
   FILE *fout = NULL;

   if (stats_file != NULL) {
     drawn_tree = (char*) malloc(32*1024);
   }

   last = 0;
   for (;;) {
      sleep(2);
      now = time(NULL);

      eth_rate = (double)(stats_in_eth[0] - stats_in_eth[1]) / (now - last);
      udpr_rate = (double)(stats_in_udp[0] - stats_in_udp[1]) / (now - last);
      udpw_rate = (double)(stats_out_udp[0] - stats_out_udp[1]) / (now - last);

      fprintf(stderr, "\033[2J\033[H");
      fprintf(stderr, "ETH Read:  (%9.2fKB) [%5.2f KB/s]\n", (double)(stats_in_eth[0])/1024, eth_rate/1024);
      fprintf(stderr, "UDP Read:  (%9.2fKB) [%5.2f KB/s]\n", (double)(stats_in_udp[0])/1024, udpr_rate/1024);
      fprintf(stderr, "UDP Write: (%9.2fKB) [%5.2f KB/s]\n", (double)(stats_out_udp[0])/1024, udpw_rate/1024);

      if (drawn_tree != NULL) {
         if (fout == NULL  &&  (fout=fopen(stats_file, "wb")) == NULL) {
           fprintf(stderr, "Can't open output file '%s'!\n", stats_file);
           exit(EXIT_FAILURE);
         }
         // reset drawn_tree to empty string
         drawn_tree[0] = '\0';
         sprintf(total_stats,
                  "<tr><th>Local Ethernet</td><td>%.2f KB</td><td>%.2f KB/sec</td></tr>\n"
                  "<tr><th>UVLan Read</td><td>%.2f KB</td><td>%.2f KB/sec</td></tr>\n"
                  "<tr><th>UVLan Write</td><td>%.2f KB</td><td>%.2f KB/sec</td></tr>\n",
                  (double)(stats_in_eth[0])/1024, eth_rate/1024,
                  (double)(stats_in_udp[0])/1024, udpr_rate/1024,
                  (double)(stats_out_udp[0])/1024, udpw_rate/1024);
      }

      stats_in_eth[1] = stats_in_eth[0];
      stats_in_udp[1] = stats_in_udp[0];
      stats_out_udp[1] = stats_out_udp[0];
      last = now;

      // build data table
      tree_draw(drawn_tree);

      fwrite(jshtml_part1, 1, strlen(jshtml_part1), fout);
      fwrite(drawn_tree, 1, strlen(drawn_tree), fout);
      fwrite(jshtml_part2, 1, strlen(jshtml_part2), fout);
      fwrite(total_stats, 1, strlen(total_stats), fout);
      fwrite(jshtml_part3, 1, strlen(jshtml_part3), fout);
      fclose(fout); fout=NULL;
   }
   return NULL;
}

void *heart_thread(void *d)
{
   int x;
   char buf[1];
   buf[0] = UVLAN_TYPE_HB;
   for (;;) {
      sleep(heartbeat_delay);
      for (x = 0; x < port_no; x++) {
         sendto(bound_sock, buf, 1, 0, (struct sockaddr *)&ports[x].dest_sin, sizeof(ports[x].dest_sin));
      }
   }
   return NULL;
}

void *nagle_thread(void *d)
{
   int x;
   for (;;) {
      usleep(nagle_delay);
      for (x = 0; x < port_no; x++) {
         pthread_mutex_lock(&ports[x].lock);
         if (ports[x].cur_mode == UVLAN_MODE_SEND_RECV && ports[x].buf_len > 0) {
            crypto_write_buf(&ports[x], ports[x].buf, ports[x].buf_len);
         }
         ports[x].buf_len = 0;
         pthread_mutex_unlock(&ports[x].lock);
      }
   }
   return NULL;
}

void *timer_thread(void *d)
{
   int x;
   unsigned long ti;
   for (;;) {
      sleep(1);
      ti = time(NULL);
      for (x = 0; x < port_no; x++) {
         pthread_mutex_lock(&ports[x].lock);
         if (ports[x].cur_mode != UVLAN_MODE_SEND_RECV && ports[x].ticker < ti) {
            ports[x].cur_mode = UVLAN_MODE_SEND_RECV;
            ports[x].buf_len  = 0;
            ports[x].ticker   = 0;
         }
         pthread_mutex_unlock(&ports[x].lock);
      }
   }
   return NULL;
}
         
         
      
      

void usage(void)
{
   printf(
"Usage: [-D] [-c filename] [-d level] [-h delay] [-s] [-S file] [-n]\n"
"   -G,--genkey    Generate new keys and exit\n"
"   -D,--daemon    Run the program in the background\n"
"   -c,--conf      filename,  Specify an alternate configuration file (default: /etc/uvlan.conf)\n"
"   -d,--debug     level, specify a debug level (1-3)\n"
"   -h,--heartbeat delay, use a UDP heartbeat delay in seconds, (default: 0 == no heartbeat)\n"
"   -n,--nagle     delay, nagle delay frames upto delay 1/8 milliseconds (default: off, max: 100ms)\n"
"   -s,--stats     Turn on stat display\n"
"   -S,--htmlstats Turn on HTML stat dump\n\n");
}

void check_perms(char *conf_name)
{
#ifndef WIN32
    struct stat fs;
    
    /* are we root? */
    if (getuid() != 0) {
       fprintf(stderr, "UVLAN must run as root.\n");
       exit(EXIT_FAILURE);
    }
    
    /* is the mode bits of the conf 400? */
    if (stat(conf_name, &fs) < 0) {
       perror("stat");
       exit(EXIT_FAILURE);
    }
    
    if (fs.st_uid || fs.st_gid) {
       fprintf(stderr, "%s must belong to root and be in the root group\n", conf_name);
       exit(EXIT_FAILURE);
    }
    
    fs.st_mode &= 0777;
    if (fs.st_mode != 0400) {
       fprintf(stderr, "%s must have mode bits 400 not %o\n", conf_name, fs.st_mode);
       exit(EXIT_FAILURE);
    }       
#endif
}

void genKeys() {
   unsigned int    ret;
   unsigned long   buflen, b64len;
   unsigned char   buf[512], b64[1024];
   FILE *prikey_out, *pubkey_out;
   memset(buf, 0, sizeof(buf));
   memset(b64, 0, sizeof(b64));

   if ((prikey_out=fopen("uvlan_pri.txt", "wb")) == NULL) {
       fprintf(stderr, "Error opening private-key output file.\n");
       exit(EXIT_FAILURE);
   }
   chmod("uvlan_pri.txt", 0400);

   if ((pubkey_out=fopen("uvlan_pub.txt", "wb")) == NULL) {
       fprintf(stderr, "Error opening public-key output file.\n");
       exit(EXIT_FAILURE);
   }

   if ((ret=ecc_make_key(&uvlan_prng, find_prng("yarrow"), 32, &uvlan_prikey)) != CRYPT_OK) {
       fprintf(stderr, "Error generating new keys: %s\n", error_to_string(ret));
       exit(EXIT_FAILURE);
   }

   buflen = sizeof(buf);
   if ((ret=ecc_export(buf, &buflen, PK_PRIVATE, &uvlan_prikey)) != CRYPT_OK) {
       fprintf(stderr, "Error exporting private-key: %s\n", error_to_string(ret));
       exit(EXIT_FAILURE);
   }

   b64len = sizeof(b64);
   if ((ret=base64_encode(buf, buflen, b64, &b64len)) != CRYPT_OK) {
       fprintf(stderr, "Error encoding public-key: %s\n", error_to_string(ret));
       exit(EXIT_FAILURE);
   }
   fwrite(b64, 1, b64len, prikey_out);
   fclose(prikey_out);

   buflen = sizeof(buf);
   if ((ret=ecc_export(buf, &buflen, PK_PUBLIC, &uvlan_prikey)) != CRYPT_OK) {
       fprintf(stderr, "Error exporting private-key: %s\n", error_to_string(ret));
       exit(EXIT_FAILURE);
   }

   b64len = sizeof(b64);
   if ((ret=base64_encode(buf, buflen, b64, &b64len)) != CRYPT_OK) {
       fprintf(stderr, "Error encoding public-key: %s\n", error_to_string(ret));
       exit(EXIT_FAILURE);
   }
   fwrite(b64, 1, b64len, pubkey_out);
   fclose(pubkey_out);
}

int main(int argc, char **argv)
{
   pthread_t nt, pt, ut, st, ht, tt;
   int       daemon_mode=0, x;
   char      *conf_name = "/etc/uvlan.conf";
   unsigned ret;
   
   srand(time(NULL));
   
   fprintf(stderr, "UVLAN: Tom and J-L's Userspace Virtual LAN\n"
                   "Public Domain\n"
                   "Website: http://libtom.org\n"
                   "CVS ID %s\n\n"
                   "Built on %s\n\n", uvlan_cvsid, __DATE__);

   crypto_init();
   ltc_mp = ltm_desc;
   if ((ret=rng_make_prng(128, find_prng("yarrow"), &uvlan_prng, NULL)) != CRYPT_OK) {
      fprintf(stderr, "Error creating PRNG '%s', err=%s\n", "yarrow", error_to_string(ret));
      usage();
      return EXIT_FAILURE;
   }

   for (x=1; x<argc; x++) {
      if (!strcmp(argv[x], "--help")) {
         usage();
         return EXIT_SUCCESS;
      } else if (!strcmp(argv[x], "-G") || !strcmp(argv[x], "--genkey")) {
        genKeys();
        return 0;
      } else if (!strcmp(argv[x], "-D") || !strcmp(argv[x], "--daemon")) {
         daemon_mode = 1;
      } else if (!strcmp(argv[x], "-c") || !strcmp(argv[x], "--conf")) {
         if (x + 1 >= argc) { usage(); return EXIT_FAILURE; }
         conf_name = argv[x+1];
         ++x;
      } else if (!strcmp(argv[x], "-d") || !strcmp(argv[x], "--debug")) {
         if (x + 1 >= argc) { usage(); return EXIT_FAILURE; }
         if (sscanf(argv[x+1], "%d", &debug_level) != 1) {
            fprintf(stderr, "Invalid debug mode: %s\n", argv[x+1]);
            usage();
            return EXIT_FAILURE;
         }
         ++x;
      } else if (!strcmp(argv[x], "-n") || !strcmp(argv[x], "--nagle")) {
         if (x + 1 >= argc) { usage(); return EXIT_FAILURE; }
         if (sscanf(argv[x+1], "%d", &nagle_delay) != 1) {
            fprintf(stderr, "Invalid naggle delay: %s\n", argv[x+1]);
            usage();
            return EXIT_FAILURE;
         }
         ++x;
         
         if (nagle_delay < 0 || nagle_delay > 800) {
            nagle_delay = 1;
         }
         nagle_delay *= 125; // convert 1/4 ms to us 
      } else if (!strcmp(argv[x], "-h") || !strcmp(argv[x], "--heartbeat")) {
         if (x + 1 >= argc) { usage(); return EXIT_FAILURE; }
         if (sscanf(argv[x+1], "%d", &heartbeat_delay) != 1) {
            fprintf(stderr, "Invalid heartbeat delay: %s\n", argv[x+1]);
            usage();
            return EXIT_FAILURE;
         }
         ++x;
      } else if (!strcmp(argv[x], "-s") || !strcmp(argv[x], "--stats")) {
         stats_mode = 1;
      } else if (!strcmp(argv[x], "-S") || !strcmp(argv[x], "--htmlstats")) {
         stats_mode = 1;
         if (x + 1 >= argc) { usage(); return EXIT_FAILURE; }
         stats_file = argv[x+1];
         ++x;
      } else {
         usage();
         return EXIT_FAILURE;
      }
   }  

   if (daemon_mode) {
      daemon(0, debug_level>0?1:0);
   }
   
   check_perms(conf_name);
  
   read_conf(conf_name);
   pthread_create(&pt, NULL, pcap_thread, NULL);
   pthread_create(&ut, NULL, udp_thread,  NULL);
   pthread_create(&tt, NULL, timer_thread,  NULL);

   if (stats_mode) {
      pthread_create(&st, NULL, stats_thread, NULL);
   }

   if (heartbeat_delay) {
      pthread_create(&ht, NULL, heart_thread,  NULL);
   }
   
   if (nagle_delay) {
      pthread_create(&nt, NULL, nagle_thread, NULL);
   }
   
   pthread_join(pt, NULL);
   pthread_join(ut, NULL);
   return 0;
}
   
   

