#include "uvlan.h"

const char* xmlp_error_to_string_[XMLP_RESERVED+1] = {
"OK",
"No opening tag found",
"No closing tag found",
"Invalid error code"
};

const char* xmlp_error_to_string(int code) {
  if (XMLP_OK <= code  &&  code <= (XMLP_RESERVED-1)) {
    return xmlp_error_to_string_[code];
  }
  return xmlp_error_to_string_[XMLP_RESERVED];
}

xmlp* xmlp_notfound = NULL;

void xmlp_free(xmlp *x);
void xmlp_free_next(xmlp *x) {
  if (x->next) {
    xmlp_free(x->next);
    free(x->next);
    x->next = NULL;
  }
}
void xmlp_free_down(xmlp *x) {
  if (x->down) {
    xmlp_free(x->down);
    free(x->down);
    x->down = NULL;
  }
}
void xmlp_free(xmlp *x) {
  memset(x->tag, 0, strlen(x->tag));
  if (x->data) {
    memset(x->data, 0, strlen(x->data));
    free(x->data);
    x->data = NULL;
  }
  xmlp_free_next(x);
  xmlp_free_down(x);
}

void xmlp_print_(xmlp *x, FILE *fout, char* indent) {
  char indent2[32];
  while (x != NULL) {
    fprintf(fout, "%s{%s,", indent, x->tag);
    if (x->down) {
      fprintf(fout, "\n");
      strcpy(indent2, indent);
      strcat(indent2, " ");
      xmlp_print_(x->down, fout, indent2);
      fprintf(fout, "%s}\n", indent);
    } else {
      fprintf(fout, " %s}\n", x->data);
    }
    x = x->next;
  }
}
void xmlp_print(xmlp *x, FILE *fout) {
  char indent[32];
  strcpy(indent, "");
  xmlp_print_(x, fout, indent);
}

xmlp* xmlp_find_elem_n(xmlp *x, const char *tag, int idx) {
  while (x != NULL) {
    if (strcmp(x->tag, tag) == 0) {
      if (idx-- == 0)
        return x;
    }
    x = x->next;
  }

  return xmlp_notfound;
}
xmlp* xmlp_find_elem(xmlp *x, const char *tag) {
  return xmlp_find_elem_n(x, tag, 0);
}

void xmlp_init(xmlp *x) {
  if (xmlp_notfound == NULL) {
    xmlp_notfound = (xmlp*) malloc(sizeof(*xmlp_notfound));
    xmlp_init(xmlp_notfound);
    xmlp_notfound->tag = "<NOT_FOUND>";
    xmlp_notfound->data = "<NOT_FOUND>";
    xmlp_notfound->down = xmlp_notfound;
    xmlp_notfound->notfound = 1;
  }
  memset(x, 0, sizeof(*x));
  x->g = xmlp_find_elem;
  x->gn = xmlp_find_elem_n;
}

int xmlp_read(xmlp *x, const char *xmldata) {
  char *start, *end, *space;
  char endtag[64];
  xmlp *last=NULL;
  int ret;

  while (strspn(xmldata, " \t\r\n") != strlen(xmldata)) {
    if ((start=strchr(xmldata, '<')) == NULL)
      return XMLP_NO_OPEN_TAG;
    if ((space=strchr(start, ' ')) == NULL)
      return XMLP_NO_OPEN_TAG;
    if ((end=strchr(xmldata, '>')) == NULL)
      return XMLP_NO_OPEN_TAG;
    if (space < end) // <tag attr=blah>...
      end = space;

    x->tag = (char*)malloc(end-start);
    memcpy(x->tag, start+1, end-start-1);
    x->tag[end-start-1] = '\0';
//fprintf(stderr,"x->tag=%s %u\n", x->tag, end-start);

    strcpy(endtag, "</");
    strcpy(endtag+2, x->tag);
    strcpy(endtag+2+strlen(x->tag), ">");
//fprintf(stderr,"endtag=%s\n", endtag);
  
    start = end+1;
  
    if ((end=strstr(xmldata, endtag)) == NULL)
      return XMLP_NO_CLOSE_TAG;

    x->data = (char*)malloc(end-start+1);
    memcpy(x->data, start, end-start);
    x->data[end-start] = '\0';

    if (strchr(x->data, '<') != NULL) {
//fprintf(stderr,"iter\n");
      xmlp *l = x;
      while (l->down != NULL)
        l = l->down;
      l->down = (xmlp*) malloc(sizeof(*x));

      xmlp_init(l->down);
      ret = xmlp_read(l->down, x->data);
      free(x->data);
      x->data = NULL;

      if (ret != XMLP_OK) {
        xmlp_free(x);
        return ret;
      }
    }

    last = x;
    x->next = (xmlp*) malloc(sizeof(*x));
    xmlp_init(x->next);
    x = x->next;

    xmldata = end + strlen(endtag);
//fprintf(stderr,"xml='%u'\n'%s'\n", strspn(xmldata, " \t\r\n"), xmldata);
  } /* while */

  if (last != NULL  &&  x->data == NULL  &&  x->down == NULL) { /* both null */
    free(last->next);
    last->next = NULL;
  }

  return XMLP_OK;
}

#if 0
int main(int argL, char *arg[]) {
  xmlp xml, *x=&xml;
  FILE *fin = fopen("uvlan.xml", "rb");
  char *buf=(char*)malloc(32*1024);
  int readlen, ret;

  xmlp_init(&xml);

  readlen = fread(buf, 1, 32*1024, fin);
  buf[readlen] = '\0';

  if ((ret=xmlp_read(&xml, buf)) != XMLP_OK) {
    fprintf(stderr,"error = %u, %s\n", ret, xmlp_error_to_string(ret));
    return -1;
  }

  xmlp_print(&xml, stdout);

  printf("simp = %s\n", x->g(x->g(x->g(x,"uvlanConf")->down,"local")->down,"bindipaddr") ->tag);
  printf("test = %s\n", xmlp_find_elem_n(x->down, "peer", 0)->tag);
  xmlp_free(&xml);
  
  return 0;
}
#endif
