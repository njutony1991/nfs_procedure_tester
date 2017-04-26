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

PATHCONF3resok CREATE_PATHCONF;

char LONGNAME[1010];

void construct_long_name(unsigned int name_max){
    if(name_max > 1003){
       fprintf(stderr, "NAME_MAX EXCEEDS 1000, FAIL TO CONSTRUCT LONG NAME\n"); 
       exit(10);
    }
    memset(LONGNAME,0,sizeof(LONGNAME));

    srand((unsigned int)time(NULL));
    int i;
    for(i=0;i<name_max+5;i++){
       LONGNAME[i] = 'a' + (rand() % 26);     
    }
}

void nfs_create_testcase_final_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE7 failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE7 to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE7: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;

    if ((CREATE_PATHCONF.no_trunc == 1 && res->status == NFS3ERR_NAMETOOLONG)
        || (CREATE_PATHCONF.no_trunc == 0 && res->status == NFS3_OK)) {
       fprintf(stdout, "TESTCASE7: CREATE LONGNAME  PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE7: CREATE LONGNAME FAILED: %d\n\n", res->status);
    }

    client->is_finished = 1;
}



void nfs_create_testcase7_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE6 failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE6 to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE6: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3ERR_EXIST) {
        fprintf(stderr, "TESTCASE6: CREATE EXCLUSIVE Replicated, Different Verf not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE6: CREATE EXCLUSIVE Replicated, Different Verf FAILED!\n");
    } else {  
        fprintf(stdout, "TESTCASE6: CREATE EXCLUSIVE Replicated, Different Verf PASSED!\n");
    }
    fprintf(stdout, "\nTESTCASE7: Send CREATE LONGNAME Request\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;

    construct_long_name(CREATE_PATHCONF.name_max);
    char* name = LONGNAME;
    args.where.name = name;
    args.how.mode = UNCHECKED;

    int ret = rpc_nfs3_create_async(rpc, nfs_create_testcase_final_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_create_testcase6_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE5 failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE5 to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE5: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3_OK) {
        fprintf(stderr, "TESTCASE5: CREATE EXCLUSIVE Replicated, Same Verf not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE5: CREATE EXCLUSIVE Replicated, Same Verf FAILED!\n");
    } else { 
        fprintf(stdout, "TESTCASE5: CREATE EXCLUSIVE Replicated, Same Verf PASSED!\n");
    }
    fprintf(stdout, "\nTESTCASE6: Send CREATE EXCLUSIVE Replicated Request,Different Verf\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    char* name = "create_exclusive.txt";
    args.where.name = name;
    args.how.mode = EXCLUSIVE;
    time_t different_verf = CREATE_VERF+111;
    memcpy(args.how.createhow3_u.verf, &different_verf, sizeof(args.how.createhow3_u.verf)); 

    int ret = rpc_nfs3_create_async(rpc, nfs_create_testcase7_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_create_testcase5_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE4 failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE4 to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE4: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3_OK) {
        fprintf(stderr, "TESTCASE4: CREATE EXCLUSIVE not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE4: CREATE EXCLUSIVE FAILED!\n");
    } else { 
        fprintf(stdout, "TESTCASE4: CREATE EXCLUSIVE PASSED!\n");
    }
    fprintf(stdout, "\nTESTCASE5: Send CREATE EXCLUSIVE Replicated Request,Same Verf\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    char* name = "create_exclusive.txt";
    args.where.name = name;
    args.how.mode = EXCLUSIVE;
    memcpy(args.how.createhow3_u.verf, &CREATE_VERF, sizeof(args.how.createhow3_u.verf)); 

    int ret = rpc_nfs3_create_async(rpc, nfs_create_testcase6_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_create_testcase4_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE3 failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE3 to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE3: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3ERR_EXIST) {
        fprintf(stderr, "TESTCASE3: CREATE GUARDED REPLICATED Request not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE3: CREATE GUARDED REPLICATED FAILED!\n");
    }else{ 
        fprintf(stdout, "TESTCASE3: CREATE GUAREDED REPLICATED PASSED!\n");
    }

    fprintf(stdout, "\nTESTCASE4: Send CREATE EXCLUSIVE Request\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    char* name = "create_exclusive.txt";
    args.where.name = name;
    args.how.mode = EXCLUSIVE;
    memcpy(args.how.createhow3_u.verf, &CREATE_VERF, sizeof(args.how.createhow3_u.verf)); 

    int ret = rpc_nfs3_create_async(rpc, nfs_create_testcase5_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_create_testcase3_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE2 failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE2 to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE2: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3_OK) {
        fprintf(stderr, "TESTCASE2: CREATE GUARDED not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE2: CREATE GUARDED FAILED!\n");
        goto next;
    } else {
        if (res->CREATE3res_u.resok.obj_attributes.attributes_follow == 0) {
           fprintf(stderr, "TESTCASE2: create GUARDED returns no attributes\n"); 
        }else{
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.uid != TESTCASE2_OBJATTR.uid.set_uid3_u.uid) {
                fprintf(stderr, "TESTCASE2: create GUARDED obj attributes not match|uid: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.uid);
           } 
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.gid != TESTCASE2_OBJATTR.gid.set_gid3_u.gid) {
                fprintf(stderr, "TESTCASE2: create GUARDED obj attributes not match|gid: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.gid);
           }
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.size != TESTCASE2_OBJATTR.size.set_size3_u.size) {
                fprintf(stderr, "TESTCASE2: create GUARDED obj attributes not match|size: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.size);
           }
        } 
    }

    fprintf(stdout, "TESTCASE2: CREATE GUAREDED PASSED!\n");

next:
    fprintf(stdout, "\nTESTCASE3: Send CREATE GUARDED REPLICATED Request\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    char* name = "create_guarded.txt";
    args.where.name = name;
    args.how.mode = GUARDED;
    args.how.createhow3_u.obj_attributes = TESTCASE2_OBJATTR;

    int ret = rpc_nfs3_create_async(rpc, nfs_create_testcase4_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_create_testcase2_cb(struct rpc_context *rpc , int status, void *data, void *private_data) {
    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE1 failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE1 to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE1: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3_OK) {
        fprintf(stderr, "TESTCASE1: create UNCHECKED not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE1: CREATE UNCHECKED FAILED!\n");
        goto next; 
    } else {
        if (res->CREATE3res_u.resok.obj_attributes.attributes_follow == 0) {
           fprintf(stderr, "TESTCASE1: create UNCHECKED returns no attributes\n"); 
        }else{
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.uid != TESTCASE1_OBJATTR.uid.set_uid3_u.uid){
                fprintf(stderr, "TESTCASE1: create UNCHECKED obj attributes not match|uid: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.uid);
           } 
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.gid != TESTCASE1_OBJATTR.gid.set_gid3_u.gid){
                fprintf(stderr, "TESTCASE1: create UNCHECKED obj attributes not match|gid: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.gid);
           }
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.size != TESTCASE1_OBJATTR.size.set_size3_u.size) {
                fprintf(stderr, "TESTCASE1: create UNCHECKED obj attributes not match|size: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.size);
           }
        } 
    }

    fprintf(stdout, "TESTCASE1: CREATE UNCHECKED PASSED!\n");

 next:
    fprintf(stdout, "\nTESTCASE2: Send CREATE GUARDED Request\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    char* name = "create_guarded.txt";
    args.where.name = name;
    args.how.mode = GUARDED;
    args.how.createhow3_u.obj_attributes = TESTCASE2_OBJATTR;

    int ret = rpc_nfs3_create_async(rpc, nfs_create_testcase3_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_create_testcase1_cb(struct rpc_context *rpc, int status, void *data , void *private_data)
{
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

    CREATE_PATHCONF = res->PATHCONF3res_u.resok;
    
    fprintf(stdout, "\nTESTCASE1: Send CREATE UNCHECKED Request\n");

    struct CREATE3args args;
    memset((void *)&args, 0, sizeof(args)); 
    args.where.dir = client->rootfh; 
    char *name = "create_unchecked.txt";
    args.where.name = name;
    args.how.mode = UNCHECKED;
    args.how.createhow3_u.obj_attributes = TESTCASE1_OBJATTR;
    if (rpc_nfs3_create_async(rpc, nfs_create_testcase2_cb, &args, client) != 0) {
        fprintf(stderr, "TESTCASE1: Failed to send create request\n");
        exit(10);
    }
}

void nfs_create_testcase_prepare_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "connection to RPC.NFSD on server %s failed\n", client->server);
        exit(10);
    }

    fprintf(stdout, "\nConnected to RPC.NFSD on %s:%d\n", client->server, client->nfs_port);
    fprintf(stdout, "TESTCASE PREPARE: Send PATHCONF Request\n");
    struct PATHCONF3args args;
    memset((void *)&args, 0, sizeof(args)); 
    args.object = client->rootfh; 

    if (rpc_nfs3_pathconf_async(rpc, nfs_create_testcase1_cb, &args, client) != 0) {
        fprintf(stderr, "Failed to send pathconf request\n\n");
        exit(10);
     }
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("usage: %s %s %s\n", argv[0], "server_ip", "mnt_path");
        exit(10);
    }

    CREATE_VERF = time(NULL);
    struct client client;
    client.server = argv[1];
    client.export = argv[2];
    client.test_case_cb = nfs_create_testcase_prepare_cb; 

    memset(&CREATE_PATHCONF, 0, sizeof(PATHCONF3resok));

    drive_frame(client);
    fprintf(stdout, "nfsclient finished\n");
    return 0;
}
