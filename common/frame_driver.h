#ifndef _H_FRAME_DRIVER_H_
#define _H_FRAME_DRIVER_H_


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <nfsc/libnfs.h>
#include <nfsc/libnfs-raw.h>
#include <nfsc/libnfs-raw-mount.h>
#include <nfsc/libnfs-raw-nfs.h>
#include <nfsc/libnfs-raw-portmap.h>
#include <nfsc/libnfs-raw-rquota.h>

typedef void (*test_case_cb_t) (struct rpc_context *rpc, int status, void *data, void *private_data);

struct client {
    char *server;
    char *export;
    uint32_t mount_port;
    uint32_t nfs_port;
    uint32_t rquota_port;
    int is_finished;
    struct nfs_fh3 rootfh;
    test_case_cb_t test_case_cb;
};

int drive_frame (struct client client);
#endif
