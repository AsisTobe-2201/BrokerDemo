/* Computer Consulting Associates */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <uuid/uuid.h>
#include "glib.h"
#include "cache.h"

struct cache_s* cac_init(struct cache_s* h);

/** Cache class */
struct cache_s
{
    uint32_t rv;
    void* table;
    FHash Hash;
    FComp Comp;
    FOnInsert OnInsert;
    FOnRemove OnRemove;

};

struct cache_s*
cac_create(
    void)
{
    struct cache_s* h = calloc(1, sizeof(struct cache_s));
    if (NULL == h)
    {
        errno = ENOMEM;
        return NULL;
    }
    return cac_init(h);
}

/* Initialize defaults on new cac instance. */
struct cache_s*
cac_init(
    struct cache_s* h)
{
    h->Hash = g_str_hash;
    h->Comp = g_str_equal;

    if (NULL == (h->table = g_hash_table_new(h->Hash, h->Comp)))
    {
        return NULL;
    }
    h->OnInsert = NULL;
    h->OnRemove = NULL;

    return h;
}


void
cac_free(
    struct cache_s* h)
{
    
}


int
cac_put(
    hCache h,
    char* url,
    char* uuid)
{
    g_hash_table_insert(h->table, url, uuid);

    fprintf(stdout, "g_hash_table_insert(table, %s, %s)\n", url, uuid);
    return 0;
}


int
cac_get(
    hCache h,
    char* url,
    char** uuid)
{
    char* value = g_hash_table_lookup(h->table, url);
    *uuid = value;
    if (NULL == value)
        return ~0;
    return 0;
}

