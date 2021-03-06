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


struct sattr3 TESTCASE1_OBJATTR = {
    .uid.set_it = 1,
    .uid.set_uid3_u.uid = 1,
    .gid.set_it = 1,
    .gid.set_gid3_u.gid = 1,
    .size.set_it = 1,
    .size.set_size3_u.size = 222
};

struct sattr3 TESTCASE2_OBJATTR = {
    .uid.set_it = 1,
    .uid.set_uid3_u.uid = 0,
    .gid.set_it = 1,
    .gid.set_gid3_u.gid = 0,
    .size.set_it = 1,
    .size.set_size3_u.size = 333
};

PATHCONF3resok ACCESS_PATHCONF;

void nfs_access_testcase_final_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/access call TESTCASE ACCESS STALEHANDLE failed with \"%s\"\n", (char *)data);
        exit(10);
    }

    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/access call TESTCASE ACCESS STALEHANDLE to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE ACCESS STALE HANDLE: Got reply from server for NFS/ACCESS procedure.\n");

    ACCESS3res *res = data;

    if (res->status == NFS3ERR_STALE) {
       fprintf(stdout, "TESTCASE ACCESS STALE: ACCESS STALE PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE ACCESS STALE: ACCESS STALE FAILED: %d\n\n", res->status);
    }
    client->is_finished = 1;  
}

void nfs_access_testcase_stale_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/access call TESTCASE ACCESS BADHANDLE failed with \"%s\"\n", (char *)data);
        exit(10);
    }

    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/access call TESTCASE ACCESS BADHANDLE to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE ACCESS BAD HANDLE: Got reply from server for NFS/ACCESS procedure.\n");

    ACCESS3res *res = data;

    if (res->status == NFS3ERR_BADHANDLE) {
       fprintf(stdout, "TESTCASE ACCESS BADHANDLE: ACCESS BADHANDLE PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE ACCESS BADHANDLE: ACCESS BADHANDLE FAILED: %d\n\n", res->status);
    }
   
    fprintf(stdout, "\nTESTCASE2: Send ACCESS STALE HANDLE Request\n"); 

    struct ACCESS3args args;
    memset((void*)&args, 0, sizeof(args));
    if (generate_stale_fh(client->rootfh) < 0) {
        fprintf(stderr, "TESTCASE ACCESS STALE: generate stale fh FAILED\n");
        exit(10);
    }
    args.object = g_stale_fh;
    args.access = ACCESS3_READ | ACCESS3_LOOKUP | ACCESS3_MODIFY | ACCESS3_EXECUTE;

    int ret = rpc_nfs3_access_async(rpc, nfs_access_testcase_final_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send access request|ret:%d\n", ret);
        cleanup_stale_fh();
        exit(10);
	}
    cleanup_stale_fh();
}



void nfs_access_testcase_badhandle_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

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

	ACCESS_PATHCONF = res->PATHCONF3res_u.resok;
    fprintf(stdout, "ACCESS PATHCONF result, name_max: %d\n", ACCESS_PATHCONF.name_max); 

    fprintf(stdout, "\nTESTCASE1: Send ACCESS BAD HANDLE Request\n");
    struct ACCESS3args args;
    memset((void*)&args, 0, sizeof(args));
    if (generate_wrong_fh(10) < 0) {
        fprintf(stderr, "TESTCASE ACCESS BADHANDLE: generate wrong fh FAILED\n");
        exit(10);
    }
    args.object = g_wrong_fh;
    args.access = ACCESS3_READ | ACCESS3_LOOKUP | ACCESS3_MODIFY | ACCESS3_EXECUTE;

    int ret = rpc_nfs3_access_async(rpc, nfs_access_testcase_stale_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send access request|ret:%d\n", ret);
        cleanup_wrong_fh();
        exit(10);
    }
    cleanup_wrong_fh();
}

void nfs_access_testcase_prepare_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

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

    if (rpc_nfs3_pathconf_async(rpc, nfs_access_testcase_badhandle_cb, &args, client) != 0) {
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
    client.test_case_cb = nfs_access_testcase_prepare_cb; 

    memset(&ACCESS_PATHCONF, 0, sizeof(PATHCONF3resok));

    drive_frame(client);

    return 0;
}
