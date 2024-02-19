/* $Id: listdevices.c,v 1.6 2015/07/23 20:40:08 nanard Exp $ */
/* Project : miniupnp
 * Author : Thomas Bernard
 * Copyright (c) 2013-2015 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution. */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <winsock2.h>
#endif /* _WIN32 */
#include "miniupnpc.h"

struct upnp_dev_list {
	struct upnp_dev_list * next;
	char * descURL;
	struct UPNPDev * * array;
	size_t count;
	size_t allocated_count;
};

#define ADD_DEVICE_COUNT_STEP 16

void add_device(struct upnp_dev_list * * list_head, struct UPNPDev * dev)
{
	struct upnp_dev_list * elt;
	size_t i;

	if(dev == NULL)
		return;
	for(elt = *list_head; elt != NULL; elt = elt->next) {
		if(strcmp(elt->descURL, dev->descURL) == 0) {
			for(i = 0; i < elt->count; i++) {
				if (strcmp(elt->array[i]->st, dev->st) == 0 && strcmp(elt->array[i]->usn, dev->usn) == 0) {
					return;	/* already found */
				}
			}
			if(elt->count >= elt->allocated_count) {
				struct UPNPDev * * tmp;
				elt->allocated_count += ADD_DEVICE_COUNT_STEP;
				tmp = realloc(elt->array, elt->allocated_count * sizeof(struct UPNPDev *));
				if(tmp == NULL) {
					fprintf(stderr, "Failed to realloc(%p, %lu)\n", elt->array, (unsigned long)(elt->allocated_count * sizeof(struct UPNPDev *)));
					return;
				}
				elt->array = tmp;
			}
			elt->array[elt->count++] = dev;
			return;
		}
	}
	elt = malloc(sizeof(struct upnp_dev_list));
	if(elt == NULL) {
		fprintf(stderr, "Failed to malloc(%lu)\n", (unsigned long)sizeof(struct upnp_dev_list));
		return;
	}
	elt->next = *list_head;
	elt->descURL = strdup(dev->descURL);
	if(elt->descURL == NULL) {
		fprintf(stderr, "Failed to strdup(%s)\n", dev->descURL);
		free(elt);
		return;
	}
	elt->allocated_count = ADD_DEVICE_COUNT_STEP;
	elt->array = malloc(ADD_DEVICE_COUNT_STEP * sizeof(struct UPNPDev *));
	if(elt->array == NULL) {
		fprintf(stderr, "Failed to malloc(%lu)\n", (unsigned long)(ADD_DEVICE_COUNT_STEP * sizeof(struct UPNPDev *)));
		free(elt->descURL);
		free(elt);
		return;
	}
	elt->array[0] = dev;
	elt->count = 1;
	*list_head = elt;
}

void free_device(struct upnp_dev_list * elt)
{
	free(elt->descURL);
	free(elt->array);
	free(elt);
}

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERR_GENERAL 1
#define ERR_SOCKET 2
#define ERR_GETADDRINFO 3
#define ERR_CONNECT 4
#define ERR_SEND 5
#define ERR_USAGE 6

#define NDEBUG

static int get_http_respcode(const char *inpbuf) {
  char proto[4096], descr[4096];
  int code;

  if (sscanf(inpbuf, "%s %d %s", proto, &code, descr) < 2)
    return -1;
  return code;
}

static int download_http(int writefd, char *ip, char *port, char *uri)
{
#ifndef NDEBUG
  fprintf(stderr,"ip:port/uri http://%s:%s%s\n", ip, port, uri);
#endif

  int ret = ERR_GENERAL;

  struct addrinfo hints, *res, *res0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  int err = getaddrinfo(ip, port, &hints, &res0);
  if (err) {
#ifndef NDEBUG
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
#endif
    return ERR_GETADDRINFO;
  }

  int s = -1;
  for (res = res0; res; res = res->ai_next) {
    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s < 0) {
      ret = ERR_SOCKET;
      continue;
    }

#ifndef NDEBUG
    char buf[256];
    inet_ntop(res->ai_family, &((struct sockaddr_in *)res->ai_addr)->sin_addr,
              buf, sizeof(buf));
    fprintf(stderr, "Connecting to %s...\n", buf);
#endif

    if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
      ret = ERR_CONNECT;
      close(s);
      s = -1;
      continue;
    }
    break; /* okay we got one */
  }
  freeaddrinfo(res0);

  if (s < 0) {
    return ret;
  }

  char buf[4096];
  // use the hack to save some space in .rodata
  strcpy(buf, "GET /");
  if (uri) {
    strncat(buf, uri, sizeof(buf) - strlen(buf) - 1);
  }
  strncat(buf, " HTTP/1.0\r\nHost: ", sizeof(buf) - strlen(buf) - 1);
  strncat(buf, ip, sizeof(buf) - strlen(buf) - 1);
  strncat(buf, "\r\n\r\n", sizeof(buf) - strlen(buf) - 1);
  int tosent = strlen(buf);
  int nsent = send(s, buf, tosent, 0);
  if (nsent != tosent)
    return ERR_SEND;

  int header = 1;
  int nrecvd;
  while ((nrecvd = recv(s, buf, sizeof(buf), 0))) {
    char *ptr = buf;
    if (header) {
      ptr = strstr(buf, "\r\n\r\n");
      if (!ptr)
        continue;

      int rcode = get_http_respcode(buf);
      if (rcode / 100 != 2)
        return rcode / 100 * 10 + rcode % 10;

      header = 0;
      ptr += 4;
      nrecvd -= ptr - buf;
    }
    write(writefd, ptr, nrecvd);
  }

  return 0;
}


static int berxel_ssdp_print(char *oristr)
{
    int ret = 0;

    char *input = oristr;
    if (strncmp(input, "http://", strlen("http://")) == 0) {
      input += strlen("http://");
    }

    //printf("%s", input);
    char *p         = input;
    char ip[255]    ;
    char port[255]  ;
    char uri[255]   ;
    char input2[255]   ;
    memset(ip, 0, sizeof(ip));
    memset(port, 0, sizeof(port));
    memset(uri, 0,  sizeof(uri));
    memset(input2, 0,  sizeof(input2));


    char *tag= strstr(p, ":");
    if (tag == NULL) {
        p = strstr(p, "/");
        memcpy(uri, p,     strlen(p));
        memcpy(ip, input,  p-input);
        memcpy(port, "80", strlen("80"));
    } else {
        p = strstr(p, ":");
        memcpy(ip, input, p-input);
        p += 1;
        memcpy(input2, p,  strlen(p));

        p = strstr(p, "/");
        memcpy(uri, p,     strlen(p));

        memcpy(port, input2, strlen(input2)- strlen(uri));
    }

    ret = download_http(STDOUT_FILENO, ip, port, uri);

    return ret;
}


#ifdef BUILDING_UDB
int list_upnpc_main(int argc, char * * argv)
#else
int main(int argc, char * * argv)
#endif
{
	const char * searched_device = NULL;
	const char * * searched_devices = NULL;
	const char * multicastif = 0;
	const char * minissdpdpath = 0;
	int ipv6 = 0;
	unsigned char ttl = 2;
	int error = 0;
	struct UPNPDev * devlist = 0;
	struct UPNPDev * dev;
	struct upnp_dev_list * sorted_list = NULL;
	struct upnp_dev_list * dev_array;
	int i;

#ifdef _WIN32
	WSADATA wsaData;
	int nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if(nResult != NO_ERROR)
	{
		fprintf(stderr, "WSAStartup() failed.\n");
		return -1;
	}
#endif

	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-6") == 0)
			ipv6 = 1;
		else if(strcmp(argv[i], "-d") == 0) {
			if(++i >= argc) {
				fprintf(stderr, "%s option needs one argument\n", "-d");
				return 1;
			}
			searched_device = argv[i];
		} else if(strcmp(argv[i], "-t") == 0) {
			if(++i >= argc) {
				fprintf(stderr, "%s option needs one argument\n", "-t");
				return 1;
			}
			ttl = (unsigned char)atoi(argv[i]);
		} else if(strcmp(argv[i], "-l") == 0) {
			if(++i >= argc) {
				fprintf(stderr, "-l option needs at least one argument\n");
				return 1;
			}
			searched_devices = (const char * *)(argv + i);
			break;
		} else if(strcmp(argv[i], "-m") == 0) {
			if(++i >= argc) {
				fprintf(stderr, "-m option needs one argument\n");
				return 1;
			}
			multicastif = argv[i];
		} else {
			printf("usage : %s [options] [-l <device1> <device2> ...]\n", argv[0]);
			printf("options :\n");
			printf("   -6 : use IPv6\n");
			printf("   -m address/ifname : network interface to use for multicast\n");
			printf("   -d <device string> : search only for this type of device\n");
			printf("   -l <device1> <device2> ... : search only for theses types of device\n");
			printf("   -t ttl : set multicast TTL. Default value is 2.\n");
			printf("   -h : this help\n");
			return 1;
		}
	}

	if(searched_device) {
		printf("searching UPnP device type %s\n", searched_device);
		devlist = upnpDiscoverDevice(searched_device,
		                             2000, multicastif, minissdpdpath,
		                             0/*localport*/, ipv6, ttl, &error);
	} else if(searched_devices) {
		printf("searching UPnP device types :\n");
		for(i = 0; searched_devices[i]; i++)
			printf("\t%s\n", searched_devices[i]);
		devlist = upnpDiscoverDevices(searched_devices,
		                              2000, multicastif, minissdpdpath,
		                              0/*localport*/, ipv6, ttl, &error, 1);
	} else {
		printf("searching all UPnP devices\n");
		devlist = upnpDiscoverAll(2000, multicastif, minissdpdpath,
		                             0/*localport*/, ipv6, ttl, &error);
	}
	if(devlist) {
		for(dev = devlist, i = 1; dev != NULL; dev = dev->pNext, i++) {
#if 0
			printf("%3d: %-48s\n", i, dev->st);
			printf("     %s\n", dev->descURL);
			printf("     %s\n", dev->usn);
#endif
			add_device(&sorted_list, dev);
		}
		putchar('\n');
		for (dev_array = sorted_list; dev_array != NULL ; dev_array = dev_array->next) {
			printf("%s :\n", dev_array->descURL);
#if 0
			for(i = 0; (unsigned)i < dev_array->count; i++) {
				printf("%2d: %s\n", i+1, dev_array->array[i]->st);
				printf("    %s\n", dev_array->array[i]->usn);
			}
#else
            char *p = NULL;
            p = strstr(dev_array->descURL, "berxel");
            if (p != NULL) {
                berxel_ssdp_print(dev_array->descURL);
            } else {
                //berxel_ssdp_print();
            }
#endif
			putchar('\n');
		}
		freeUPNPDevlist(devlist);
		while(sorted_list != NULL) {
			dev_array = sorted_list;
			sorted_list = sorted_list->next;
			free_device(dev_array);
		}
	} else {
		printf("no device found.\n");
	}

	return 0;
}

