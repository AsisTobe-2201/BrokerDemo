/* COPYRIGHT JFINK */
/**
 * @file
 *
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "nng/nng.h"
#include "nng/protocol/pipeline0/pull.h"
#include "nng/protocol/pipeline0/push.h"
#include "nng/protocol/pubsub0/pub.h"
#include "nng/protocol/pubsub0/sub.h"
#include "hGw.h"

#define IsCntrl(x) do { return x & ~0x7fffffff } while(0)
#define DEBUG(fmt,...) fprintf(stderr, "%s:%d"fmt"\n", __func__,__LINE__,#__VA_ARGS__)

/*
 * Private class
 */
struct hgw
{
    nng_socket   send_soc;
    nng_socket   recv_soc;
    fOnErr_t     OnError;
    fOnConnect_t OnConnect;
    fOnListen_t  OnListen;
    fOnTmo_t     OnTmo;
    fOnMsg_t     OnMsg;
    fOnCtrl_t    OnCtrl;
    uint32_t     tmo;
    int          rv;
    uint8_t      bIsSrvr;
    uint8_t      bFreeMsg;
    uint8_t      cansend;
    uint8_t      unused3;
    const char*  url;
    const char*  gw;
};


/* forward decls */
int hgw_init(HGW h);
int hgw_initServer(HGW h);
int hgw_initClient(HGW h);
int hgw_clientRegister(HGW h);
int OnMsg(HGW h, char* buf, size_t len);
int OnTmo(HGW h, void* args);
int OnErr(HGW h, void* args);


HGW
hgw_create(
    const char* url,
    int bIsSvr,
    fOnMsg_t OnMsg,
    fOnConnect_t OnConnect,
    int tmo)
{
    errno = 0;
    if (NULL == url)
    {
        errno = EINVAL;
        return NULL;
    }
    struct hgw* h = calloc(1, sizeof (*h));

    if (NULL == h)
    {
        errno = ENOMEM;
        return NULL;
    }
    h->bIsSrvr = bIsSvr;
    h->cansend = 0;
    h->bFreeMsg = 1;
    h->tmo = tmo;
    h->url = nng_strdup(url);
    h->OnMsg = OnMsg;
    h->OnConnect = OnConnect;

    if (0 != (hgw_init(h)))
    {
        goto cleanup;
    }

    return h;

  cleanup:
    free(h);
    return NULL;
}


/*
 * Attempts to listen on this instance's URL.  If that fails, attempts to
 * dial into this instance URL.  When successful, calls the OnConnect()
 * handler.  A bug was submitted, addressed and closed that forced the use
 * of the bIsSrvr flag.  Once that version is incorporated, the bIsSrvr flag
 * is no longer needed.
 */
int
hgw_init(
    HGW h) /**< [IN] self. */
{
    h->rv = (h->bIsSrvr) ? hgw_initServer(h) : hgw_initClient(h);

    if (0 == h->rv)
    {
        h->rv = h->OnConnect(h,NULL);
    }

    return h->rv;
}


/*
 * Initializes a server for the instance URL.  The socket created is a sub0
 * with topic set to all and receive timeout set to tmo.
 */
int
hgw_initServer(
    HGW h)
{
    if (0 != (h->rv = nng_sub0_open(&h->recv_soc)))
    {
        return h->rv;
    }

    if (0!=(h->rv = nng_setopt(h->recv_soc, NNG_OPT_SUB_SUBSCRIBE, "", 0)))
    {
        return h->rv;
    }

    if (0 != (h->rv = nng_setopt_ms(h->recv_soc, NNG_OPT_RECVTIMEO, h->tmo)))
    {
        return h->rv;
    }

    if (0 != (h->rv = nng_listen(h->recv_soc, h->url, NULL, 0)))
    {
        return h->rv;
    }
    h->bIsSrvr = 1;
    h->gw = h->url;
    h->url = NULL;

    return 0;
}


/*
 * Initializes a client.  Creates a pub0 socket and trys to dial into the
 * instance URL.
 */
int
hgw_initClient(
    HGW h) /**< [IN] self instance. */
{
    if (0 != (h->rv = nng_pub0_open(&h->send_soc)))
    {
        return h->rv;
    }

    if (0 != (h->rv = nng_dial(h->send_soc, h->url, NULL, 0)))
    {
        return h->rv;
    }
    h->gw = h->url;
    h->url = NULL;
    h->cansend = 1;

    return 0;
}


/* Returns pointer to nng_alloc string, h->rv is the length of the string. */
char*
hgw_mkname(
    HGW h)
{
    const char* fmt = "ipc:///tmp/client%d";
    pid_t tid = getpid();
    int szName = snprintf(NULL, 0, fmt, tid);
    char* name = nng_alloc(szName);
    sprintf(name, fmt, tid);
    h->rv = szName;
    return name;
}


int
hgw_listen(
    HGW h,
    const char* url)
{
	int rv = ~0;

    if (0 != (h->rv = nng_pull0_open(&h->recv_soc)))
    {
        fprintf(stderr, "Error nng_pull0_open() %s\n",nng_strerror(h->rv));
        return h->rv;
    }

    if (0 != (h->rv = nng_setopt_ms(h->recv_soc, NNG_OPT_RECVTIMEO, h->tmo)))  
    {
        return h->rv;
    }

    if (0 != (h->rv = nng_listen(h->recv_soc, url, NULL, 0)))
    {
        fprintf(stderr, "Error nng_listen() %s\n",nng_strerror(h->rv));
        fprintf(stderr, "\tURL:%s\n",url);
        nng_close(h->recv_soc);
        return h->rv;
    }
    h->bIsSrvr = 0;

    if (NULL != h->OnListen)
        h->OnListen(h, NULL);

    return 0;
}


/*
 *  server (sub)  <---------//-<--------- (pub) client
 *   recv_soc                               send_soc
 *
 *  server (push) -------->-//----------> (pull) client
 *   send_soc                               recv-soc
 */
int
hgw_clientRegister(
    HGW h)
{
	int rv = ~0;
    char* name = hgw_mkname(h);


    if (0 != (h->rv = nng_pull0_open(&h->recv_soc)))
    {
        return h->rv;
    }

    if (0 != (h->rv = nng_listen(h->recv_soc, name, NULL, 0)))
    {
        nng_close(h->recv_soc);
        return h->rv;
    }

    h->bIsSrvr = 0;
    h->url = name;
    return 0;
}


int
hgw_IsReg(HGW h, char* data, size_t sz)
{
    fprintf(stderr, "..%s\n", data);
    return strncmp(data, "client", sizeof("client"));
}



int
hgw_IsSrv(HGW h)
{
    fprintf(stderr, ".. srv:%s\n",h->bIsSrvr?"yes":"no");
    return h->bIsSrvr;
}


int hgw_defaultTmo(HGW h, void* args)
{
    (void) args;
    (void) h;
    return 0;
}


int
hgw_loop(HGW h)
{
    while (0 == h->rv)
    {
        char* data;
        size_t sz;

        if ((NULL != h->OnMsg) && (0 == (h->rv = hgw_recv(h, &data, &sz, NNG_FLAG_ALLOC))))
        {
                h->rv = (h->OnMsg)(h, data, sz);
                if (h->bFreeMsg) nng_free(data, sz);
        }

        else if ((NULL != h->OnTmo) && (NNG_ETIMEDOUT == h->rv))
        {
                h->rv = (h->OnTmo)(h, NULL);
        }

        else if (NNG_ETIMEDOUT == h->rv)
        {
            /* timed out, but no handler */
            h->rv = 0;
        }
    }
    return h->rv;
}


/** Configure */
void
hgw_config(
    HGW h,
    fOnErr_t OnError,
    fOnConnect_t OnConnect,
    fOnTmo_t OnTmo,
    fOnMsg_t OnMsg,
    fOnCtrl_t OnCtrl)
{
    if (OnError)
        h->OnError = OnError;

    if (OnConnect)
        h->OnConnect = OnConnect;

    if (OnTmo)
        h->OnTmo = OnTmo;

    if (OnMsg)
        h->OnMsg = OnMsg;

    if (OnCtrl)
        h->OnCtrl = OnCtrl;
}


/** Destructor */
void
hgw_free(
    HGW h)
{
    nng_strfree((char*)h->url);
    nng_strfree((char*)h->gw);
    free(h);
}


int
hgw_setOpts(
    HGW h,
    int Options)
{
    return 0;
}


/* Send */
int
hgw_send(
    HGW h,
    void *data,
    size_t size,
    int flags)
{
    /* need to find "to" and use that socket */
    if (h->cansend == 0)
        fprintf(stderr, "ERROR hgw_send on null\n");

    return nng_send(h->send_soc, data, size, flags);
}


/* Receive */
int
hgw_recv(
    HGW h,
    void *data,
    size_t *sizep,
    int flags)
{
     h->rv = nng_recv(h->recv_soc, data, sizep, flags);
     if (0 != h->rv)
         fprintf(stderr, "recv: %s\n", nng_strerror(h->rv));
     return h->rv;
}


