/*
 * $Id$
 *
 * FreeWRL plugin utilities header file.
 */

#include <stdio.h>
#include <string.h>

#include "npapi.h"

typedef struct _urlRequest {
    char	url[FILENAME_MAX]; /* do not allow urls larger
                                      than the max filename length */
    NPP		instance;   /* plugin instance that forked FreeWRL */
    uint32	notifyCode; /* for NPN_GetURLNotify, NPP_URLNotify
                               (not used)                          */
} urlRequest_t;

typedef urlRequest_t urlRequest;
