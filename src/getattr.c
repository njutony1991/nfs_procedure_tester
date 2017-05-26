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

/**
 * GETATTR3args args
 * test case1:
 *   args.object :a wrong file handle 
 *   expect: NFS3ERR_BADHANDLE
 * test case2:      
 *   args.object :a stale file handle 
 *   expect: NFS3ERR_STALE
 **/

PATHCONF3resok GETATTR_PATHCONF;

char LONGNAME[1010];


void nfs_getattr_testcase_final_cb (struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/getattr call TESTCASE GETATTR STALE failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/getattr call TESTCASE GETATTR STALE to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE GETATTR STALE: Got reply from server for NFS/GETATTR procedure.\n");
    GETATTR3res *res = data;
    if (res->status == NFS3ERR_STALE) {
       fprintf(stdout, "TESTCASE GETATTR STALE: GETATTR STALE PASSED!\n\n");
    } else {
       fprintf(stderr, "TESTCASE GETATTR STALE: GETATTR STALE FAILED: %d\n\n", res->status);
    }

    client->is_finished = 1; 
}


void nfs_getattr_testcase_stale_cb (struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;
    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/getattr call TESTCASE GETATTR BADHANDLE failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/getattr call TESTCASE GETATTR BADHANDLE to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE GETATTR BADHANDLE: Got reply from server for NFS/GETATTR procedure.\n");
    GETATTR3res *res = data;
	if (res->status == NFS3ERR_BADHANDLE) {
	   fprintf(stdout, "TESTCASE GETATTR BADHANDLE: GETATTR BADHANDLE PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE GETATTR BADHANDLE: GETATTR BADHANDLE FAILED: %d\n\n", res->status);
    }

    fprintf(stdout, "\nTESTCASE2: Send GETATTR STALE HANDLE Request\n");
    struct GETATTR3args args;
    if (generate_stale_fh(client->rootfh) < 0) {
        fprintf(stderr, "TESTCASE GETATTR STALE: generate stale fh FAILED\n");
        exit(10);
    }
    args.object = g_stale_fh;
    int ret = rpc_nfs3_getattr_async(rpc, nfs_getattr_testcase_final_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send getattr request|ret:%d\n", ret);
        cleanup_stale_fh();
        exit(10);
    }
    cleanup_stale_fh(); 
}


void nfs_getattr_testcase_badhandle_cb (struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/pathconf call failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/pathconf call to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE PATHCONF: Got reply from server for NFS/PATHCONF procedure.\n");

    PATHCONF3res *res = data;
    if (res->status != NFS3_OK) {
        fprintf(stderr, "TESTCASE PREPARE: PATHCONF not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE PREPARE: PATHCONF FAILED!\n");
        exit(10);
    }
    fprintf(stdout, "Got reply from server for NFS/PATHCONF procedure.\n");

    fprintf(stdout, "\nTESTCASE1: Send GETATTR BADHANDLE Request\n");
    struct GETATTR3args args;
    if (generate_wrong_fh(10) < 0) {
        fprintf(stderr, "TESTCASE GETATTR BADHANDLE: generate wrong fh FAILED\n");
        exit(10);
    }
    args.object = g_wrong_fh;
	int ret = rpc_nfs3_getattr_async(rpc, nfs_getattr_testcase_stale_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send GETATTR request|ret:%d\n", ret);
        cleanup_wrong_fh();
        exit(10);
    }
    cleanup_wrong_fh();
}


void nfs_getattr_testcase_prepare_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "connection to RPC.NFSD on server %s failed\n", client->server);
        exit(10);
    }

    fprintf(stdout, "Connected to RPC.NFSD on %s:%d\n", client->server, 2049);
    fprintf(stdout, "\nTESTCASE PREPARE: Clean Up Test Files\n");

    fprintf(stdout, "TESTCASE PREPARE: Send PATHCONF Request\n");

    struct PATHCONF3args args;
    memset((void *)&args, 0, sizeof(args)); 
    args.object = client->rootfh; 

    if (rpc_nfs3_pathconf_async(rpc, nfs_getattr_testcase_badhandle_cb, &args, client) != 0) {
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
    client.test_case_cb = nfs_getattr_testcase_prepare_cb; 

    memset(&GETATTR_PATHCONF, 0, sizeof(PATHCONF3resok));

    drive_frame(client);

    return 0;
}
