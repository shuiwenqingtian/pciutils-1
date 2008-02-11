/*
 *	The PCI Library -- Resolving ID's via DNS
 *
 *	Copyright (c) 2007--2008 Martin Mares <mj@ucw.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include <string.h>
#include <stdlib.h>

#include "internal.h"
#include "names.h"

#ifdef PCI_USE_DNS

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

char
*pci_id_net_lookup(struct pci_access *a, int cat, int id1, int id2, int id3, int id4)
{
  char name[256], dnsname[256], txt[256];
  byte answer[4096];
  const byte *data;
  int res, i, j, dlen;
  ns_msg m;
  ns_rr rr;

  switch (cat)
    {
    case ID_VENDOR:
      sprintf(name, "%04x", id1);
      break;
    case ID_DEVICE:
      sprintf(name, "%04x.%04x", id2, id1);
      break;
    case ID_SUBSYSTEM:
      sprintf(name, "%04x.%04x.%04x.%04x", id4, id3, id2, id1);
      break;
    case ID_GEN_SUBSYSTEM:
      sprintf(name, "%04x.%04x.s", id2, id1);
      break;
    case ID_CLASS:
      sprintf(name, "%02x.c", id1);
      break;
    case ID_SUBCLASS:
      sprintf(name, "%02x.%02x.c", id2, id1);
      break;
    case ID_PROGIF:
      sprintf(name, "%02x.%02x.%02x.c", id3, id2, id1);
      break;
    default:
      return NULL;
    }
  sprintf(dnsname, "%s.%s", name, a->id_domain);

  a->debug("Resolving %s\n", dnsname);
  res_init();
  res = res_query(dnsname, ns_c_in, ns_t_txt, answer, sizeof(answer));
  if (res < 0)
    {
      a->debug("\tfailed, h_errno=%d\n", _res.res_h_errno);
      return NULL;
    }
  if (ns_initparse(answer, res, &m) < 0)
    {
      a->debug("\tinitparse failed\n");
      return NULL;
    }
  for (i=0; ns_parserr(&m, ns_s_an, i, &rr) >= 0; i++)
    {
      a->debug("\tanswer %d (class %d, type %d)\n", i, ns_rr_class(rr), ns_rr_type(rr));
      if (ns_rr_class(rr) != ns_c_in || ns_rr_type(rr) != ns_t_txt)
	continue;
      data = ns_rr_rdata(rr);
      dlen = ns_rr_rdlen(rr);
      j = 0;
      while (j < dlen && j+1+data[j] <= dlen)
	{
	  memcpy(txt, &data[j+1], data[j]);
	  txt[data[j]] = 0;
	  j += 1+data[j];
	  a->debug("\t\t%s\n", txt);
	  if (txt[0] == 'i' && txt[1] == '=')
	    return strdup(txt+2);
	}
    }

  return NULL;
}

#else

char *pci_id_net_lookup(struct pci_access *a UNUSED, int cat UNUSED, int id1 UNUSED, int id2 UNUSED, int id3 UNUSED, int id4 UNUSED)
{
  return NULL;
}

#endif

void
pci_set_net_domain(struct pci_access *a, char *name, int to_be_freed)
{
  if (a->free_id_domain)
    free(a->id_domain);
  a->id_domain = name;
  a->free_id_domain = to_be_freed;
}
