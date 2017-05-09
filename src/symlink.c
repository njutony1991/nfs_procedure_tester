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
    .size.set_it = 0,
    .size.set_size3_u.size = 0 
};

PATHCONF3resok SYMLINK_PATHCONF;

#define NFSPATH_MAXLENGTH (8*1024) 

char LONGNAME[1010];
char LONGNFSPATH[NFSPATH_MAXLENGTH + 100];

void construct_long_name(unsigned int name_max) {
    if(name_max > 1003){
       fprintf(stderr, "NAME_MAX EXCEEDS 1000, FAIL TO CONSTRUCT LONG NAME\n"); 
       exit(10);
    }
    memset(LONGNAME,0,sizeof(LONGNAME));

    srand((unsigned int)time(NULL));
    int i;
    for (i=0; i<name_max+1; i++){
       LONGNAME[i] = 'a' + (rand() % 26);     
    }
}

void construct_long_nfspath(unsigned int path_max) {
	if (path_max > (NFSPATH_MAXLENGTH + 50)) {
		fprintf(stderr, "PATH MAX EXCEEDS 8K, FAIL TO CONSTRUCT LONG PATH\n");
		exit(10);
	}
	memset(LONGNFSPATH, 0, sizeof(LONGNFSPATH));
	srand((unsigned int)time(NULL));
	int i;
	for	(i=0; i<path_max+1; i++) {
		LONGNFSPATH[i] = 'a' + (rand() % 26);
	}
}

void nfs_symlink_testcase_final_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/symlink call TESTCASE SYMLINK LONG NFSPATH failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/symlink call TESTCASE SYMLINK LONG NFSPATH to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE SYMLINK LONG NFSPATH: Got reply from server for NFS/SYMLINK procedure.\n");

    SYMLINK3res *res = data;

	if (res->status == NFS3ERR_ACCES){ 
	   fprintf(stdout, "TESTCASE SYMLINK LONG NFSPATH: SYMLINK LONG NFSPATH PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE SYMLINK LONG NFSPATH: SYMLINK LONG NFSPATH FAILED: %d\n\n", res->status);
    }
    
	client->is_finished = 1;
}

void nfs_symlink_testcase_longnfspath_cb(struct rpc_context *rpc, int status, void *data, void *private_data){

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/symlink call TESTCASE SYMLINK LONG NAME failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/symlink call TESTCASE SYMLINK LONG NAME to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

	SYMLINK3res *res = data;
  	if ((SYMLINK_PATHCONF.no_trunc == 1 && res->status == NFS3ERR_NAMETOOLONG)
        || (SYMLINK_PATHCONF.no_trunc == 0 && res->status == NFS3_OK)) {
        fprintf(stdout, "TESTCASE SYMLINK LONG NAME: SYMLINK LONGNAME PASSED!\n\n");
	} else {
	    fprintf(stderr, "TESTCASE SYMLINK LONG NAME: SYMLINK LONGNAME FAILED: %d\n\n", res->status);
	}

	struct SYMLINK3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    args.where.name = "symlink.test14"; 
	construct_long_nfspath(NFSPATH_MAXLENGTH + 22);
	args.symlink.symlink_data = LONGNFSPATH;
	args.symlink.symlink_attributes = TESTCASE2_OBJATTR; 
	fprintf(stdout, "SYMLINK TEST Long NFS Path: %s\n", LONGNFSPATH);

	int ret = rpc_nfs3_symlink_async(rpc, nfs_symlink_testcase_final_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send symlink request|ret:%d\n", ret);
        exit(10);
    }
}


void nfs_symlink_testcase_longname_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

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

	SYMLINK_PATHCONF = res->PATHCONF3res_u.resok;
    fprintf(stdout, "SYMLINK PATHCONF result, name_max: %d\n", SYMLINK_PATHCONF.name_max); 

 	struct SYMLINK3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
	construct_long_name(SYMLINK_PATHCONF.name_max);	
    args.where.name = LONGNAME;
	fprintf(stdout, "SYMLINK TEST Long Name: %s\n", LONGNAME);
	args.symlink.symlink_data = "test.txt";

    int ret = rpc_nfs3_symlink_async(rpc, nfs_symlink_testcase_longnfspath_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send symlink request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_symlink_testcase_prepare_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

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

    if (rpc_nfs3_pathconf_async(rpc, nfs_symlink_testcase_longname_cb, &args, client) != 0) {
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
    client.test_case_cb = nfs_symlink_testcase_prepare_cb; 

    memset(&SYMLINK_PATHCONF, 0, sizeof(PATHCONF3resok));

    drive_frame(client);

    return 0;
}
