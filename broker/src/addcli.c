/*
 * @file
 */
#define _GNU_SOURCE

#include <errno.h>
#include <nng/nng.h>
#include <nng/supplemental/util/platform.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "addsvr.h"
#include "cache.h"
#include "gw.h"
#include "hGw.h"
#include "hexdump.h"

int addsvr(uint32_t a, uint32_t b, uint8_t** buf, size_t* sz);


/**
 * Called for every message.
 */
int OnMessage(HGW h, char* buf, size_t sz)
{
    struct message* msg = (struct message*)buf;
    struct addmsg* add_msg = (struct addmsg*)msg->bytes;

    fprintf(stdout, "OnMessage\n");
    switch (msg->hdr.msgid)
    {
        case ADDRSP:
            fprintf(stdout, "OnMessage\n");
            break;
        default:
            fprintf(stdout, "\tUnexpected.\n");
            break;
    }
    return 0;
}


/**
 * Creates a register request containing the URL that this service
 * listens on.
 */
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


/**
 * Called every tmo mS.
 */
int OnTimeOut(HGW h, void* args)
{
    return 0;
}


/**
 * Called when the dialer connects.
 */
int OnConnect(HGW h, void* args)
{
    int rv = ~0;
    struct message* msg;
    size_t szMsg = 0;
    char* name;

    int len = asprintf(&name, "ipc:///tmp/addcli/v1");
    (void) len;
    if (NULL == name)
    {
        errno = ENOMEM;
        fprintf(stderr, "asprintf NULL\n");
        return ~0;
    }
    hgw_listen(h, name);

    if (0 != mkRegReq(name, &msg, &szMsg))
    {
        fprintf(stderr, "mkRegReq error\n");
        return ~0;
    }

    if (0 != (rv = hgw_send(h, msg, szMsg, 0)))
    {
        fprintf(stderr, "%s\n", nng_strerror(rv));
    }

    nng_free(msg, szMsg);
    free(name);
    return 0;
}


/**
 * Entry point.
 */
int main(
    int argc,
    char* argv[])
{
    HGW server = hgw_create("ipc:///tmp/gw", 0, OnMessage, OnConnect, 10000);

    if (NULL == server)
    {
        perror("Hgw_create failed.");
        exit(-1);
    }

    hgw_config(server, NULL, NULL, OnTimeOut, NULL, NULL);
    hgw_loop(server);
    return 0;
}

