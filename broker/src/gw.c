/* Computer Consulting Associates */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <errno.h>

#include "nng/nng.h"
#include "nng/protocol/pipeline0/pull.h"
#include "nng/protocol/pipeline0/push.h"
#include "cache.h"
#include "hGw.h"
#include "gw.h"

#define DEBUG_LVL 3

#if defined(DEBUG_LVL) && DEBUG_LVL > 0
 #define DEBUG(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
 #define DEBUG(fmt, args...) /* Don't do anything in release builds */
#endif


// void uuid_generate(uuid_t out);
// void uuid_generate_random(uuid_t out);
// void uuid_generate_time(uuid_t out);
// int uuid_generate_time_safe(uuid_t out);

/** Local cache of uuid/url mapping. */
hCache cache;

/** NIL UUID */
uuid_t GW_NIL = { 0 };


/**
 * Evaluates true if the message is a Register Request message.
 *
 * This function returns true if the lessage is a register request from a
 * client, else false.
 */
int
isRegMsg(
    HGW h,     /**< [IN] self instance. */
    char* buf, /**< [IN] message buffer. */
    size_t sz) /**< [IN] size of message buffer */
{
    if (sizeof(struct message_header) > sz)
        return 0;

    struct message_header* hdr = (struct message_header*)buf;
    return hdr->msgid == GWREGREQ;
}


/**
 * Returns true if this client is already registered.
 */
int
isReg(
    HGW h,      /**< [IN] Sel handle. */
    char* buf,  /**< [IN] buffer.     */
    size_t sz)  /**< [IN] size.       */
{

    int rv;
    char* strDest;
    char* strUUID;

    if (NULL == cache)
    {
        DEBUG( "Cache is NULL.\n");
    }

    struct message* msg = (struct message *)buf;
    strDest = calloc(33, sizeof('\0'));
    uuid_unparse(msg->hdr.dest, strDest);
    char* url = (char*)msg->bytes;

    if (0 == (rv = cac_get(cache, (char*)msg->bytes, &strUUID)))
    {
        DEBUG( "Found url %s -> %s\n", url, strDest);
    }
    return ~rv;
}


/**
 *
 */
int mkRegRsp(
    char* strUUID,
    struct message** out,
    size_t *sz)
{

    struct message* msg = NULL;
    const int szUUID = 32;
    *sz = szUUID + sizeof(struct message_header);

    msg = calloc(1, *sz);
    msg->hdr.msgid = GWREGRSP;
    int rv = uuid_parse(strUUID, msg->bytes);
    if (0 == rv)
    {
        *out = msg;
    }
    return rv;
}


/**
 *
 */
int
gw_register_client(
    HGW h,
    uint8_t* buf,
    size_t sz)
{
    char* url;     /* owned by cache */
    char* strUUID; /* owned by cache */

    struct message* msg = (struct message*)buf;
    int len = asprintf(&url, "%s", msg->bytes);
    (void)len;
    strUUID = calloc(33, sizeof('\0'));
    uuid_unparse(msg->hdr.dest, strUUID);

    cac_put(cache, url, strUUID);
    return 0;
}


/**
 * Registers client with broker.
 *
 * Opens a push socket using URL provided in name and dials.
 *
 */
int
registerClient(
    HGW h,      /**< [IN] Self pointer.   */
    char* name, /**< [IN] Name of client. */
    size_t sz)  /**< [IN] Len of name.    */
{
    int rv;
    nng_socket s;

    DEBUG("got regreq from %s.\n", name);

    if (0 != (rv = nng_push0_open(&s)))
    {
        DEBUG( "nng_push0_open error: %s\n", nng_strerror(rv));
        return rv;
    }

    if (0 != (rv = nng_dial(s, name, NULL, 0)))
    {
        DEBUG( "nng_dial error: %s\n", nng_strerror(rv));
        return rv;
    }

    if (0 != (rv = nng_send(s, "ack", 3,0)))
    {
        DEBUG( "nng_send error: %s\n", nng_strerror(rv));
        return rv;
    }
    return rv;
}


/**
 *
 */
int
OnMessage(
    HGW h,
    char* buf,
    size_t sz)
{
    if (0 == sz)
        return 0;

    int bIsRegReq = isRegMsg(h, buf, sz);
    int bIsReg = isReg(h, buf, sz);

    if (!isRegMsg(h, buf, sz))
    {
        /* Regular message, e.g not a broker control message. */
        fprintf(stdout, "%s\n",buf);
    }
    else if ((1 == bIsRegReq) && (bIsReg) )
    {
        /* Register message and this client is already registered. */
        fprintf(stdout, "Already registered.\n");
    }
    else
    {
        /* Register message and this client needs to be registered. */
        gw_register_client(h, (uint8_t*)buf, sz);
    }
    return 0;
}


/**
 * Called when the listen is completed.
 *
 */
int OnConnect(
    HGW h,      /**< [IN] self. */
    void* args) /**< [IN] args. */
{
    fprintf(stdout, "GW connected.\n");
    return 0;
}

//
///**
// * Called when the recv times out, every tmo mS.
// *
// */
//int
//OnTmo(
//    HGW h,
//    void* args)
//{
//    DEBUG( "...Timeout\n");
//    return 0;
//}
//

int
OnCtrl(
    HGW h,
    char* buf,
    size_t sz)
{
    DEBUG( "Control\n");
    return 0;
}


int main(
    int argc,
    char* argv[])
{
    cache = cac_create();

    HGW server = hgw_create("ipc:///tmp/gw", 1, OnMessage, OnConnect, 10000);
    if (NULL == server)
    {
        perror("Hgw_create failed.");
    }

    hgw_loop(server);
    return 0;
}

