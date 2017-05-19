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

PATHCONF3resok RENAME_PATHCONF;

char LONGNAME[1010];

void construct_long_name(unsigned int name_max){
    if(name_max > 1003){
       fprintf(stderr, "NAME_MAX EXCEEDS 1000, FAIL TO CONSTRUCT LONG NAME\n"); 
       exit(10);
    }
    memset(LONGNAME,0,sizeof(LONGNAME));

    srand((unsigned int)time(NULL));
    int i;
    for(i=0; i<name_max+1; i++){
       LONGNAME[i] = 'a' + (rand() % 26);     
    }
}

void nfs_rename_testcase_final_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/rename call TESTCASE RENAME TO LONG NAME failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/rename call TESTCASE RENAME TO LONG NAME to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE RENAME TO LONG NAME: Got reply from server for NFS/RENAME procedure.\n");

    LOOKUP3res *res = data;

    if (res->status == NFS3ERR_NAMETOOLONG) {
       fprintf(stdout, "TESTCASE RENAME TO LONG NAME: RENAME TO LONGNAME PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE RENAME TO LONG NAME: RENAME TO LONGNAME FAILED: %d\n\n", res->status);
    }
    
	client->is_finished = 1;
}

void nfs_rename_testcase_to_longname_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {
    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/rename call TESTCASE RENAME FROM LONG NAME failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/rename call TESTCASE RENAME FROM LONG NAME to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }
    fprintf(stdout, "TESTCASE RENAME FROM LONG NAME: Got reply from server for NFS/RENAME procedure.\n");

    REMOVE3res *res = data;

    if (res->status == NFS3ERR_NAMETOOLONG) {
       fprintf(stdout, "TESTCASE RENAME FROM LONG NAME: RENAME FROM LONGNAME PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE RENAME FROM LONG NAME: RENAME FROM LONGNAME FAILED: %d\n\n", res->status);
	}

    struct RENAME3args args;
    memset((void*)&args, 0, sizeof(args));
    args.from.dir = client->rootfh;
    args.from.name = "f.txt";

	args.to.dir = client->rootfh;
    construct_long_name(100);
	args.to.name = LONGNAME;

	fprintf(stdout, "RENAME TO LONGNAME: %s\n", args.to.name);

    int ret = rpc_nfs3_rename_async(rpc, nfs_rename_testcase_final_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send rename request|ret:%d\n", ret);
        exit(10);
    }
}



void nfs_rename_testcase_from_longname_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "connection to RPC.NFSD on server %s failed, status:%d\n", client->server);
        exit(10);
    }

    struct RENAME3args args;
    memset((void*)&args, 0, sizeof(args));
    args.from.dir = client->rootfh;
    construct_long_name(100);
    char* name = LONGNAME;
    args.from.name = name;


	args.to.dir = client->rootfh;
	args.to.name = "1111.txt";

	fprintf(stdout, "RENAME FROM LONGNAME: %s\n", args.from.name);

    int ret = rpc_nfs3_rename_async(rpc, nfs_rename_testcase_to_longname_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send rename request|ret:%d\n", ret);
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
    client.test_case_cb = nfs_rename_testcase_from_longname_cb; 

    memset(&RENAME_PATHCONF, 0, sizeof(PATHCONF3resok));

    struct rpc_context *rpc = rpc_init_context();
    rpc_set_auth(rpc, authnone_create());

    drive_frame_with_rpc(client, rpc);

    return 0;
}
