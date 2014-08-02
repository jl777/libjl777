#include "uvlan.h"

static pthread_mutex_t tree_lock = PTHREAD_MUTEX_INITIALIZER;
static uvlan_tree tree;

void tree_init(void)
{
   pthread_mutex_lock(&tree_lock);
   
   memset(&tree, 0, sizeof(tree));
   
   pthread_mutex_unlock(&tree_lock);
}

uvlan_tree *tree_find_ex(const unsigned char *addr)
{
   uvlan_tree *o;
   int n;
   
   pthread_mutex_lock(&tree_lock);
   
   /* binsearch the tree */
   o = &tree;
     
   while ((n = memcmp(addr, o->ent.addr, 6))) {
      if (n == -1) {
         if (o->left == NULL) {
            break;
         } else {
            o = o->left;
         }
      } else {
         if (o->right == NULL) {
            break;
         } else {
            o = o->right;
         }
      }
   }
     
   /* we're either at the node or where the node should be added */
  
   return o;
}

uvlan_tree*  tree_find(const unsigned char *addr)
{
   uvlan_tree *o;
   
   o = tree_find_ex(addr);
   
   if (debug_level>9) {
       fprintf(stderr, " Finding  %02x:%02x:%02x:%02x:%02x:%02x at %p\n", 
                   (unsigned)(addr[0]),(unsigned)(addr[1]), (unsigned)(addr[2]),
                   (unsigned)(addr[3]),(unsigned)(addr[4]),(unsigned)(addr[5]), o);
   }
   
   
   if (!memcmp(o->ent.addr, addr, 6)) {
      pthread_mutex_unlock(&tree_lock);
      return o;
   } else {
      pthread_mutex_unlock(&tree_lock);
      return NULL;
   }
}
   
static unsigned char noaddr[4] = {0,0,0,0};
uvlan_tree* tree_add(uvlan_host_entry *ent)
{
   uvlan_tree *o, *ret;;
   int n;
   
   o = tree_find_ex(ent->addr);
   
   if (debug_level>9) {
                  fprintf(stderr, " Adding  %02x:%02x:%02x:%02x:%02x:%02x\n", 
                   (unsigned)(ent->addr[0]),(unsigned)(ent->addr[1]), (unsigned)(ent->addr[2]),
                   (unsigned)(ent->addr[3]),(unsigned)(ent->addr[4]),(unsigned)(ent->addr[5]));
   }
   
   /* is it already present? */
   n = memcmp(ent->addr, o->ent.addr, 6);
   if (!n) {
      memcpy(o->ent.addr, ent->addr, 6);
      // overwrite current addy iff new addr is not empty
      if (memcmp(ent->ipaddr, noaddr, 4) != 0)
        memcpy(o->ent.ipaddr, ent->ipaddr, 4);
      o->ent.port = ent->port;
      ret = o;
   } else if (n == -1) {
      o->left = calloc(1, sizeof(*o));
      memcpy(o->left->ent.addr, ent->addr, 6);
      memcpy(o->left->ent.ipaddr, ent->ipaddr, 4);
      o->left->ent.port = ent->port;
      ret = o->left;
   } else {
      o->right = calloc(1, sizeof(*o));
      memcpy(o->right->ent.addr, ent->addr, 6);
      memcpy(o->right->ent.ipaddr, ent->ipaddr, 4);
      o->right->ent.port = ent->port;
      ret = o->right;
   }
   pthread_mutex_unlock(&tree_lock);

   return ret;
}

static unsigned char blank_addr[6] = {'\0', '\0', '\0', '\0', '\0', '\0'};
static void do_print_walk(uvlan_tree *o, char *drawn_tree)
{
   uvlan_host_entry *ent;
   double rate_in, rate_out;
   time_t now;
   now = time(NULL);

   ent = &o->ent;
   if (memcmp(ent->addr, blank_addr, 6) == 0) {
      fprintf(stderr, "Hardware Address  {     IP Address} =>   Nick (    inKB/   outKB)[Kps in/out]\n");
      goto SKIP_STATS;
   }   

   rate_in = (double)(ent->bytes_in[0] - ent->bytes_in[1]) / (now - ent->last);
   rate_out = (double)(ent->bytes_out[0] - ent->bytes_out[1]) / (now - ent->last);

   if (drawn_tree != NULL) {
      sprintf(&drawn_tree[strlen(drawn_tree)],
              "'%s'," /* nick */
              "'%02x:%02x:%02x:%02x:%02x:%02x'," /* mac */
              "%u," /* ip */
              "%llu,%llu," /* Bytes in/out */
              "%.2f,%.2f,\n", /* Byte/sec in/out */
              ent->portptr == NULL ? "local" : ent->portptr->nick,
              (unsigned)(ent->addr[0]),(unsigned)(ent->addr[1]),(unsigned)(ent->addr[2]),
              (unsigned)(ent->addr[3]),(unsigned)(ent->addr[4]),(unsigned)(ent->addr[5]),
              (ent->ipaddr[0]<<24) | (ent->ipaddr[1]<<16) | (ent->ipaddr[2]<<8) | (ent->ipaddr[3]),
              ent->bytes_in[0], ent->bytes_out[0],
              rate_in, rate_out);
   }

   fprintf(stderr, "%02x:%02x:%02x:%02x:%02x:%02x {%3u.%3u.%3u.%3u} =>%7s (%8.2f/%8.2f)[%5.2f/%5.2f]\n", 
                   (unsigned)(ent->addr[0]),(unsigned)(ent->addr[1]),(unsigned)(ent->addr[2]),
                   (unsigned)(ent->addr[3]),(unsigned)(ent->addr[4]),(unsigned)(ent->addr[5]),
                   (unsigned)(ent->ipaddr[0]), (unsigned)(ent->ipaddr[1]), (unsigned)(ent->ipaddr[2]), (unsigned)(ent->ipaddr[3]),
                   ent->portptr == NULL ? "local" : ent->portptr->nick,
                   (double)ent->bytes_in[0]/1024, (double)ent->bytes_out[0]/1024,
                   rate_in/1024, rate_out/1024);

   ent->bytes_in[1] = ent->bytes_in[0];
   ent->bytes_out[1] = ent->bytes_out[0];
   ent->last = now;

SKIP_STATS:
   if (o->left) {
      do_print_walk(o->left, drawn_tree);
   }
   if (o->right) {
      do_print_walk(o->right, drawn_tree);
   }
}

void tree_draw(char *drawn_tree)
{
   do_print_walk(&tree, drawn_tree);
}   
