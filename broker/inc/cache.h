/* Computer Consulting Associates */
#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>
#include <uuid/uuid.h>

typedef struct cache_s* hCache;
typedef void* FHash;
typedef void* FComp;
typedef void* FOnInsert;
typedef void* FOnRemove;
hCache cac_create(void);
int cac_put(hCache h, char* url, char* uuid);
int cac_get(hCache h, char* url, char** uuid);
void cac_free(hCache h);

#endif

