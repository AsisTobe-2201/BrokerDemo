/*
 * @file
 */
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>

#include "hexdump.h"
#include "cache.h"
#include "hGw.h"
#include "gw.h"

int OnMessage(HGW h, char* buf, size_t sz)
{
    fprintf(stdout, "OnMessage\n");
    return 0;
}


int mkRegReq(
    char* url,
    struct message** out,
    size_t *sz)
{
    size_t szURL = 0;
    uint8_t* buf = NULL;
    struct message* msg = NULL;

    szURL = strlen(url) + 1;
    *sz = szURL + sizeof(struct message);
    buf = nng_alloc(*sz);
    if (NULL == buf)
    {
        errno = ENOMEM;
        return ~0;
    }

    bzero(buf, *sz); /* ensures uuid is nil */
    msg = (struct message*)buf;
    msg->hdr.msgid = GWREGREQ;
    uuid_generate(msg->hdr.dest);
    memcpy(msg->bytes, url, szURL);
    *out = (struct message*)buf;
    return 0;
}


int OnConnect(HGW h, void* args)
{
    int rv = ~0;
    struct message* msg;
    size_t szMsg = 0;
    char* name;

    fprintf(stdout, "Enter OnConnect\n");
    int len = asprintf(&name, "///tmp.client%d", getpid());
    (void) len;
    if (NULL == name)
    {
        fprintf(stderr, "asprintf NULL\n");
        return ~0;
    }

    fprintf(stdout, "Client URL %s\n",name);
    hgw_listen(h, name);

    if (0 != mkRegReq(name, &msg, &szMsg))
    {
        fprintf(stderr, "mkRegReq error\n");
        return ~0;
    }
fprintf(stdout, "szMsg is %ld\n", szMsg);
    DumpHex(msg, szMsg);

    if (0 != (rv = hgw_send(h, msg, szMsg, 0)))
    {
        fprintf(stderr, "%s\n", nng_strerror(rv));
    }

    nng_free(msg, szMsg);
    free(name);
    fprintf(stdout, "Exit OnConnect\n");
    return 0;
}


int main(
    int argc,
    char* argv[])
{
    HGW server = hgw_create("ipc:///tmp/gw", 0, OnMessage, OnConnect, 0);

    if (NULL == server)
    {
        perror("Hgw_create failed.");
    }

	int flg = 1;
    nng_msleep(1000);
    while (0 == flg)
    {
		char* data;
		size_t sz;

		flg = hgw_recv(server, &data, &sz, NNG_FLAG_ALLOC);
        fprintf(stderr, "%s\n", data);
        nng_strfree(data);
    };
    hgw_free(server);
    return 0;
}

