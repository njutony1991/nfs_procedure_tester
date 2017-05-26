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
#include <errno.h>

#include <nfsc/libnfs.h>
#include <nfsc/libnfs-raw.h>
#include <nfsc/libnfs-raw-mount.h>
#include <nfsc/libnfs-raw-nfs.h>
#include <nfsc/libnfs-raw-portmap.h>
#include <nfsc/libnfs-raw-rquota.h>


typedef void (*test_case_cb_t) (struct rpc_context *rpc, int status, void *data, void *private_data);

//typedef void (*make_arguments_t) (void *data); 

//struct test_case {
//    char name[20];
//    make_arguments_t make_argument;
//    struct test_case *next;
//};

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

extern char g_file_set[20][100];
extern int g_test_file_num;

extern nfs_fh3 g_wrong_fh;
extern nfs_fh3 g_stale_fh; 

int drive_frame (struct client client);
int drive_frame_with_rpc (struct client client, struct rpc_context *rpc);
int cleanup_test_files (struct rpc_context *rpc, struct nfs_fh3 rootfh, struct client *client, int is_finish);

int generate_wrong_fh (size_t data_len);
void cleanup_wrong_fh ();

int generate_stale_fh (nfs_fh3 origin_fh);
void cleanup_wrong_fh ();
#endif
