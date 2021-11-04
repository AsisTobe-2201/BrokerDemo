/* Computer Consulting Associates */

#ifndef _GW_H_
#define _GW_H_
#include <stdint.h>

#define GWMAG 1 << 31

enum msgid
{
    GWREGREQ = GWMAG | 1
   ,GWREGRSP = GWMAG | 2
};


/* Message header */
struct message_header
{
    uint32_t msgid; /**< Unique message id. */
    uuid_t   dest;  /**< Destination UUID. */
};


/* Register Request */
struct register_request
{
    struct message_header hdr;
    uint8_t url[]; /**< Destingation URL '\0' terminated. */
};


 /** Message structure, opaque. */
 struct message
 {
     struct message_header hdr;
     uint8_t bytes[];
 };

#endif
