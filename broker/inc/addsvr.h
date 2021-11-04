/* Computer Consulting Associates */

#ifndef _ADDSVR_H_
#define _ADDSVR_H_
#include <stdint.h>

#include "gw.h"


#define ADD2DW  1
#define ADDRSP  2


struct addmsg
{
    uint32_t a;
    uint32_t b;
};



int addsvr(uint32_t a, uint32_t b, uint8_t** buf, size_t* sz);


#endif
