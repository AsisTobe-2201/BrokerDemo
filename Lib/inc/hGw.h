/* Copyright (c) Jim Fink, all rights reserved */
/**
 * @file
 *
 * Broker interface.
 *
 * This file provides the public interface for the broker interface.
 */
#ifndef _HGW_H_
#define _HGW_H_

#include <stdint.h>
#include "nng/nng.h"

/** Flags */
enum
{
    BR_SERVER = 1 << 31
};


/** Type for instances. */
typedef struct hgw* HGW;

/** Open Socket */
typedef int (*fOpen_t)(nng_socket s);

/** Connect (listen or dial) */
typedef int (*fConnect_t)(nng_socket s);

/** On Message Handler */
typedef int (*fOnMsg_t)(HGW h, char* buf, size_t len);

/** On Control Message Handler */
typedef int (*fOnCtrl_t)(HGW h, char* buf, size_t len);

/** On Time out handler */
typedef int (*fOnTmo_t)(HGW h, void* args);

/** On connect handler */
typedef int (*fOnConnect_t)(HGW h, void* args);

/** On listen handler */
typedef int (*fOnListen_t)(HGW h, void* args);

/** On Error handler */
typedef int (*fOnErr_t)(HGW h, void* args);

void
hgw_config(HGW h,
           fOnErr_t OnError,
           fOnConnect_t OnConnect,
           fOnTmo_t OnTmo,
           fOnMsg_t OnMsg,
           fOnCtrl_t OnCtrl);

/** Constructor */
HGW hgw_create(
    const char* url,
    int bSvrv,
    fOnMsg_t OnMsg,
    fOnConnect_t OnCnct,
    int tmo);

/** Register receive URL */
int hgw_listen(
    HGW h,
    const char* url);

/** Destructor */
void hgw_free(HGW h);

/** Set Options */
int hgw_setOpts(HGW h, int options);

/** Event/Message loop */
int hgw_loop(HGW h);

/* Send */
int hgw_send(HGW h, void *data, size_t size, int flags);

/* Receive */
int hgw_recv(HGW h, void *data, size_t *sizep, int flags);

/* helpers */
char* hgw_mkname(HGW h);
#endif


