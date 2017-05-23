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

time_t CREATE_VERF = 0;

PATHCONF3resok LOOKUP_PATHCONF;

char LONGNAME[1010];

void construct_long_name(unsigned int name_max){
    if(name_max > 1003){
       fprintf(stderr, "NAME_MAX EXCEEDS 1000, FAIL TO CONSTRUCT LONG NAME\n"); 
       exit(10);
    }
    memset(LONGNAME,0,sizeof(LONGNAME));

    srand((unsigned int)time(NULL));
    int i;
    for(i=0; i<name_max; i++){
       LONGNAME[i] = 'a' + (rand() % 26);     
    }
}

void nfs_lookup_testcase_final_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {
    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/link call TESTCASE LOOKUP STALE failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/link call TESTCASE LOOKUP STALE to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE LOOK STALE: Got reply from server for NFS/LOOKUP procedure.\n");
    LINK3res *res = data;
    if (res->status == NFS3ERR_STALE) {
       fprintf(stdout, "TESTCASE LOOKUP STALE: LOOKUP STALE PASSED!\n\n");
    } else {
       fprintf(stderr, "TESTCASE LOOKUP STALE: LOOKUP STALE FAILED: %d\n\n", res->status);
    }

    client->is_finished = 1;

}

void nfs_lookup_testcase_stale_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/lookup call TESTCASE LOOKUP BADHANDLE failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/lookup call TESTCASE LOOKUP BADHANDLE to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE LOOKUP BADHANDLE: Got reply from server for NFS/LOOKUP procedure.\n");

    LOOKUP3res *res = data;

    if (res->status == NFS3ERR_BADHANDLE) {
       fprintf(stdout, "TESTCASE LOOKUP BADHANDLE: LOOKUP BADHANDLE PASSED!\n\n");
    } else {
       fprintf(stderr, "TESTCASE LOOKUP BADHANDLE: LOOKUP BADHANDLE FAILED: %d\n\n", res->status);
    }

    fprintf(stdout, "\nTESTCASE3: Send LOOKUP STALE HANDLE Request\n");
    struct LOOKUP3args args;
    nfs_fh3 stale_fh;
    memset(&stale_fh, 0, sizeof(nfs_fh3));
    stale_fh.data.data_len = client->rootfh.data.data_len;
    stale_fh.data.data_val = malloc(stale_fh.data.data_len);
    if(stale_fh.data.data_val == NULL) {
        fprintf(stderr,"TESTCASE LOOKUP STALEHANDLE malloc failed: %s\n", strerror(errno));
        exit(10);
    }
    memcpy(stale_fh.data.data_val, client->rootfh.data.data_val, stale_fh.data.data_len);
    size_t index = stale_fh.data.data_len-1;
    stale_fh.data.data_val[index] = stale_fh.data.data_val[index]+1;

    args.what.dir = stale_fh;
    args.what.name = "stale_handle.txt"; 

    int ret = rpc_nfs3_lookup_async(rpc, nfs_lookup_testcase_final_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send lookup request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_lookup_testcase_badhandle_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/lookup call TESTCASE LOOKUP LONG NAME failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/lookup call TESTCASE LOOKUP LONG NAME to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE LOOKUP LONG NAME: Got reply from server for NFS/LOOKUP procedure.\n");

    LOOKUP3res *res = data;

    if (res->status == NFS3ERR_NAMETOOLONG) {
       fprintf(stdout, "TESTCASE LOOKUP LONG NAME: LOOKUP LONGNAME PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE LOOKUP LONG NAME: LOOKUP LONGNAME FAILED: %d\n\n", res->status);
    }
  
    fprintf(stdout, "\nTESTCASE2: Send LOOKUP BAD HANDLE Request\n"); 
    struct LOOKUP3args args; 
    nfs_fh3 wrong_fh;
    char data_val[10];
    memset(data_val, '0', sizeof(data_val));
    wrong_fh.data.data_len = 10;
    wrong_fh.data.data_val = data_val;
    
    args.what.dir = wrong_fh;
    args.what.name = "meaningless.txt"; 

    int ret = rpc_nfs3_lookup_async(rpc, nfs_lookup_testcase_stale_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send lookup request|ret:%d\n", ret);
        exit(10);
    }
}



void nfs_lookup_testcase_longname_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

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

    fprintf(stdout, "\nTESTCASE1: Send LOOKUP LONG NAME Request\n"); 
	LOOKUP_PATHCONF = res->PATHCONF3res_u.resok;
    fprintf(stdout, "LOOKUP PATHCONF result, name_max: %d\n", LOOKUP_PATHCONF.name_max); 

    struct LOOKUP3args args;
    memset((void*)&args, 0, sizeof(args));
    args.what.dir = client->rootfh;

    construct_long_name(LOOKUP_PATHCONF.name_max);
    char* name = LONGNAME;
    args.what.name = name;
	fprintf(stdout, "LOOKUP LONGNAME: %s\n", name);
    int ret = rpc_nfs3_lookup_async(rpc, nfs_lookup_testcase_badhandle_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send lookup request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_lookup_testcase_prepare_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

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

    if (rpc_nfs3_pathconf_async(rpc, nfs_lookup_testcase_longname_cb, &args, client) != 0) {
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
    client.test_case_cb = nfs_lookup_testcase_prepare_cb; 

    memset(&LOOKUP_PATHCONF, 0, sizeof(PATHCONF3resok));

    drive_frame(client);

    return 0;
}
