#include "common/frame_driver.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <nfsc/libnfs.h>
#include <nfsc/libnfs-raw.h>
#include <nfsc/libnfs-raw-mount.h>
#include <nfsc/libnfs-raw-nfs.h>
#include <nfsc/libnfs-raw-portmap.h>
#include <nfsc/libnfs-raw-rquota.h>


PATHCONF3resok READDIR_PATHCONF;


void nfs_readdir_testcase_final_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    client->is_finished = 1;  
}

void nfs_readdir_testcase_notdir_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/readdir call TESTCASE READDIR STALEHANDLE failed with \"%s\"\n", (char *)data);
        exit(10);
    }

    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/readdir call TESTCASE READDIR STALEHANDLE to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE READDIR STALE HANDLE: Got reply from server for NFS/READDIR procedure.\n");

    READDIR3res *res = data;

    if (res->status == NFS3ERR_STALE) {
       fprintf(stdout, "TESTCASE READDIR STALE: READDIR STALE PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE READDIR STALE: READDIR STALE FAILED: %d\n\n", res->status);
    }
}

void nfs_readdir_testcase_stale_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/readdir call TESTCASE READDIR BADHANDLE failed with \"%s\"\n", (char *)data);
        exit(10);
    }

    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/readdir call TESTCASE READDIR BADHANDLE to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE READDIR BAD HANDLE: Got reply from server for NFS/READDIR procedure.\n");

    READDIR3res *res = data;

    if (res->status == NFS3ERR_BADHANDLE) {
       fprintf(stdout, "TESTCASE READDIR BADHANDLE: READDIR BADHANDLE PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE READDIR BADHANDLE: READDIR BADHANDLE FAILED: %d\n\n", res->status);
    }
   
    fprintf(stdout, "\nTESTCASE2: Send READDIR STALE HANDLE Request\n"); 

    struct READDIR3args args;
    memset((void*)&args, 0, sizeof(args));
    if (generate_stale_fh(client->rootfh) < 0) {
        fprintf(stderr, "TESTCASE READDIR STALE: generate stale fh FAILED\n");
        exit(10);
    }
    args.dir = g_stale_fh;
    args.cookie = 0;
    args.count = 4096;
    uint64_t verf = 0;
    memcpy(args.cookieverf, &verf, sizeof(args.cookieverf));

    int ret = rpc_nfs3_readdir_async(rpc, nfs_readdir_testcase_final_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send access request|ret:%d\n", ret);
        cleanup_stale_fh();
        exit(10);
	}
    cleanup_stale_fh();
}



void nfs_readdir_testcase_badhandle_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/pathconf call failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/pathconf call to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }
    printf("Got reply from server for NFS/PATHCONF procedure.\n");

    PATHCONF3res *res = data;
	if (res->status != NFS3_OK) {
        fprintf(stderr, "TESTCASE PREPARE: PATHCONF not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE PREPARE: PATHCONF FAILED!\n");
        exit(10);
    }

	READDIR_PATHCONF = res->PATHCONF3res_u.resok;
    fprintf(stdout, "READDIR PATHCONF result, name_max: %d\n", READDIR_PATHCONF.name_max); 

    fprintf(stdout, "\nTESTCASE1: Send READDIR BAD HANDLE Request\n");
    struct READDIR3args args;
    memset((void*)&args, 0, sizeof(args));
    if (generate_wrong_fh(10) < 0) {
        fprintf(stderr, "TESTCASE ACCESS BADHANDLE: generate wrong fh FAILED\n");
        exit(10);
    }
    args.dir = g_wrong_fh;
    args.cookie = 0;
    args.count = 4096;
    uint64_t verf = 0; 
    memcpy(args.cookieverf, &verf, sizeof(args.cookieverf));
    int ret = rpc_nfs3_readdir_async(rpc, nfs_readdir_testcase_stale_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send readdir request|ret:%d\n", ret);
        cleanup_wrong_fh();
        exit(10);
    }
    cleanup_wrong_fh();
}

void nfs_readdir_testcase_prepare_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "connection to RPC.NFSD on server %s failed\n", client->server);
        exit(10);
    }

    fprintf(stdout, "Connected to RPC.NFSD on %s:%d\n", client->server, 2049);

    fprintf(stdout, "TESTCASE PREPARE: Send PATHCONF Request\n");

    struct PATHCONF3args args;
    memset((void *)&args, 0, sizeof(args)); 
    args.object = client->rootfh; 

    if (rpc_nfs3_pathconf_async(rpc, nfs_readdir_testcase_badhandle_cb, &args, client) != 0) {
        fprintf(stderr, "Failed to send pathconf request\n\n");
        exit(10);
    }
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("usage: %s %s %s\n", argv[0], "server_ip", "mnt_path");
        exit(10);
    }

    struct client client;
    client.server = argv[1];
    client.export = argv[2];
    client.test_case_cb = nfs_readdir_testcase_prepare_cb; 

    memset(&READDIR_PATHCONF, 0, sizeof(PATHCONF3resok));

    drive_frame(client);

    return 0;
}
