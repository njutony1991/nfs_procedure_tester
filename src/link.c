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

PATHCONF3resok LINK_PATHCONF;

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

void nfs_link_testcase_final_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/link call TESTCASE LINK LONG NAME failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/link call TESTCASE LINK LONG NAME to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE LINK LONG NAME: Got reply from server for NFS/LINK procedure.\n");

    LINK3res *res = data;

	if ((LINK_PATHCONF.no_trunc == 1 && res->status == NFS3ERR_NAMETOOLONG)
        || (LINK_PATHCONF.no_trunc == 0 && res->status == NFS3_OK)) {       
	   fprintf(stdout, "TESTCASE LINK LONG NAME: LINK LONGNAME PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE LINK LONG NAME: LINK LONGNAME FAILED: %d\n\n", res->status);
    }
    
	client->is_finished = 1;
}

void nfs_link_testcase_longname_cb(struct rpc_context *rpc, int status, void *data, void *private_data){

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/lookup call TESTCASE LINK LONG NAME failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/lookup call TESTCASE LINK LONG NAME to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

	LOOKUP3res *res = data;
	if(res->status != NFS3_OK){
		fprintf(stderr, "nfs/lookp failed, status: %d\b", res->status);
		exit(10);
	}
	nfs_fh3 link_args = res->LOOKUP3res_u.resok.object;

	struct LINK3args args;
    args.file = link_args;		
	args.link.dir = client->rootfh;
	construct_long_name(LINK_PATHCONF.name_max);
	args.link.name = LONGNAME;
	fprintf(stdout, "LINK LONGNAME: %s\n", args.link.name);

	int ret = rpc_nfs3_link_async(rpc, nfs_link_testcase_final_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send link request|ret:%d\n", ret);
        exit(10);
    }
}


void nfs_link_testcase_lookup_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

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

	LINK_PATHCONF = res->PATHCONF3res_u.resok;
    fprintf(stdout, "LINK PATHCONF result, name_max: %d\n", LINK_PATHCONF.name_max); 

 	struct LOOKUP3args args;
    memset((void*)&args, 0, sizeof(args));
    args.what.dir = client->rootfh;
    args.what.name = "create_new.txt";
	
    int ret = rpc_nfs3_lookup_async(rpc, nfs_link_testcase_longname_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send mkdir request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_link_testcase_prepare_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

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

    if (rpc_nfs3_pathconf_async(rpc, nfs_link_testcase_lookup_cb, &args, client) != 0) {
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
    client.test_case_cb = nfs_link_testcase_prepare_cb; 

    memset(&LINK_PATHCONF, 0, sizeof(PATHCONF3resok));

    drive_frame(client);

    return 0;
}