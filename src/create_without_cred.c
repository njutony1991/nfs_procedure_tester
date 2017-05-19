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
    for (i=0; i<name_max; i++) {
       LONGNAME[i] = 'a' + (rand() % 26);     
    }
}

void nfs_create_testcase_final_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE LONG NAME failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE LONG NAME to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE CREATE LONG NAME: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;

    if ((CREATE_PATHCONF.no_trunc == 1 && res->status == NFS3ERR_NAMETOOLONG)
        || (CREATE_PATHCONF.no_trunc == 0 && res->status == NFS3_OK)) {
       fprintf(stdout, "TESTCASE CREATE LONG NAME: CREATE LONGNAME PASSED!\n\n");   
    } else {
       fprintf(stderr, "TESTCASE CREATE LONG NAME: CREATE LONGNAME FAILED: %d\n\n", res->status);
    }
    
    fprintf(stdout, "TESTCASE FINAL: Clean Up Test Files\n");
    //clean up test files and set termination flags;
    cleanup_test_files(rpc, client->rootfh, client, 1);
}



void nfs_testcase_create_long_name_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE EXCLUSIVE REPLICATE DIFFERENT VERF failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE EXCLUSIVE REPLICATE DIFFERENT VERF to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE CREATE EXCLUSIVE REPLICATE DIFFERENT VERF: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3ERR_EXIST) {
        fprintf(stderr, "TESTCASE CREATE EXCLUSIVE REPLICATE DIFFERENT VERF: CREATE EXCLUSIVE Replicated, Different Verf not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE CREATE EXCLUSIVE REPLICATE DIFFERENT VERF: CREATE EXCLUSIVE Replicated, Different Verf FAILED!\n");
    } else {  
        fprintf(stdout, "TESTCASE CREATE EXCLUSIVE REPLICATE DIFFERENT VERF: CREATE EXCLUSIVE Replicated, Different Verf PASSED!\n");
    }
    fprintf(stdout, "\nTESTCASE7: Send CREATE LONGNAME Request\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;

    construct_long_name(CREATE_PATHCONF.name_max);
    char* name = LONGNAME;
    args.where.name = name;
    args.how.mode = UNCHECKED;

	fprintf(stdout, "Create Long Name: %s\n", name);
	fprintf(stdout, "Name Length: %d, limit: %d\n", strlen(name), CREATE_PATHCONF.name_max);
    int ret = rpc_nfs3_create_async(rpc, nfs_create_testcase_final_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_testcase_create_exclusive_replicate_different_verf_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE EXCLUSIVE REPLICATE SAME VERF failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE EXCLUSIVE REPLICATE SAME VERF to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE CREATE EXCLUSIVE REPLICATE SAME VERF: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3_OK) {
        fprintf(stderr, "TESTCASE CREATE EXCLUSIVE REPLICATE SAME VERF: CREATE EXCLUSIVE Replicated, Same Verf not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE CREATE EXCLUSIVE REPLICATE SAME VERF: CREATE EXCLUSIVE Replicated, Same Verf FAILED!\n");
    } else { 
        fprintf(stdout, "TESTCASE CREATE EXCLUSIVE REPLICATE SAME VERF: CREATE EXCLUSIVE Replicated, Same Verf PASSED!\n");
    }
    fprintf(stdout, "\nTESTCASE6: Send CREATE EXCLUSIVE Replicated Request,Different Verf\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    char* name = g_file_set[2];
    args.where.name = name;
    args.how.mode = EXCLUSIVE;
    time_t different_verf = CREATE_VERF+111;
    memcpy(args.how.createhow3_u.verf, &different_verf, sizeof(args.how.createhow3_u.verf)); 

    int ret = rpc_nfs3_create_async(rpc, nfs_testcase_create_long_name_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_testcase_create_exclusive_replicate_same_verf_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE EXCLUSIVE failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE EXCLUSIVE to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE CREATE EXCLUSIVE: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3_OK) {
        fprintf(stderr, "TESTCASE CREATE EXCLUSIVE: CREATE EXCLUSIVE not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE CREATE EXCLUSIVE: CREATE EXCLUSIVE FAILED!\n");
    } else { 
        fprintf(stdout, "TESTCASE CREATE EXCLUSIVE: CREATE EXCLUSIVE PASSED!\n");
    }
    fprintf(stdout, "\nTESTCASE5: Send CREATE EXCLUSIVE Replicated Request,Same Verf\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    char* name = g_file_set[2];
    args.where.name = name;
    args.how.mode = EXCLUSIVE;
    memcpy(args.how.createhow3_u.verf, &CREATE_VERF, sizeof(args.how.createhow3_u.verf)); 

    int ret = rpc_nfs3_create_async(rpc, nfs_testcase_create_exclusive_replicate_different_verf_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_testcase_create_exclusive_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE GUARDED REPLICATE failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE GUARDED REPLICATE to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE CREATE GUARDED REPLICATE: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3ERR_EXIST) {
        fprintf(stderr, "TESTCASE CREATE GUARDED REPLICATE: CREATE GUARDED REPLICATE Request not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE CREATE GUARDED REPLICATE: CREATE GUARDED REPLICATE FAILED!\n");
    } else { 
        fprintf(stdout, "TESTCASE CREATE GUARDED REPLICATE: CREATE GUARDED REPLICATE PASSED!\n");
    }

    fprintf(stdout, "\nTESTCASE4: Send CREATE EXCLUSIVE Request\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    char* name = g_file_set[2];
    args.where.name = name;
    args.how.mode = EXCLUSIVE;
    memcpy(args.how.createhow3_u.verf, &CREATE_VERF, sizeof(args.how.createhow3_u.verf)); 

    int ret = rpc_nfs3_create_async(rpc, nfs_testcase_create_exclusive_replicate_same_verf_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_testcase_create_guarded_replicate_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {

    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE GUARDED failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE GUARDED to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE CREATE GUARDED: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3_OK) {
        fprintf(stderr, "TESTCASE CREATE GUARDED: CREATE GUARDED not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE CREATE GUARDED: CREATE GUARDED FAILED!\n");
        goto next;
    } else {
        if (res->CREATE3res_u.resok.obj_attributes.attributes_follow == 0) {
           fprintf(stderr, "TESTCASE CREATE GUARDED: create GUARDED returns no attributes\n"); 
        }else{
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.uid != TESTCASE2_OBJATTR.uid.set_uid3_u.uid) {
                fprintf(stderr, "TESTCASE CREATE GUARDED: create GUARDED obj attributes not match|uid: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.uid);
           } 
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.gid != TESTCASE2_OBJATTR.gid.set_gid3_u.gid) {
                fprintf(stderr, "TESTCASE CREATE GUARDED: create GUARDED obj attributes not match|gid: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.gid);
           }
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.size != TESTCASE2_OBJATTR.size.set_size3_u.size) {
                fprintf(stderr, "TESTCASE CREATE GUARDED: create GUARDED obj attributes not match|size: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.size);
           }
        } 
    }

    fprintf(stdout, "TESTCASE CREATE GUARDED: CREATE GUAREDED PASSED!\n");

next:
    fprintf(stdout, "\nTESTCASE3: Send CREATE GUARDED REPLICATE Request\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    char* name = g_file_set[1];
    args.where.name = name;
    args.how.mode = GUARDED;
    args.how.createhow3_u.obj_attributes = TESTCASE2_OBJATTR;

    int ret = rpc_nfs3_create_async(rpc, nfs_testcase_create_exclusive_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_testcase_create_guarded_cb(struct rpc_context *rpc , int status, void *data, void *private_data) {
    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE UNCHECKED failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/create call TESTCASE CREATE UNCHECKED to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "TESTCASE CREATE UNCHECKED: Got reply from server for NFS/CREATE procedure.\n");

    CREATE3res *res = data;
    if (res->status != NFS3_OK) {
        fprintf(stderr, "TESTCASE CREATE UNCHECKED: CREATE UNCHECKED not ok|status:%d\n", res->status);
        fprintf(stderr, "TESTCASE CREATE UNCHECKED: CREATE UNCHECKED FAILED!\n");
        goto next; 
    } else {
        if (res->CREATE3res_u.resok.obj_attributes.attributes_follow == 0) {
           fprintf(stderr, "TESTCASE CREATE UNCHECKED: CREATE UNCHECKED returns no attributes\n"); 
        } else {
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.uid != TESTCASE1_OBJATTR.uid.set_uid3_u.uid) {
                fprintf(stderr, "TESTCASE CREATE UNCHECKED: CREATE UNCHECKED obj attributes not match|uid: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.uid);
           } 
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.gid != TESTCASE1_OBJATTR.gid.set_gid3_u.gid) {
                fprintf(stderr, "TESTCASE CREATE UNCHECKED: CREATE UNCHECKED obj attributes not match|gid: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.gid);
           }
           if (res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.size != TESTCASE1_OBJATTR.size.set_size3_u.size) {
                fprintf(stderr, "TESTCASE CREATE UNCHECKED: CREATE UNCHECKED obj attributes not match|size: %d\n",
                    res->CREATE3res_u.resok.obj_attributes.post_op_attr_u.attributes.size);
           }
        } 
    }

    fprintf(stdout, "TESTCASE CREATE UNCHECKED: CREATE UNCHECKED PASSED!\n");

 next:
    fprintf(stdout, "\nTESTCASE2: Send CREATE GUARDED Request\n");

    struct CREATE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.where.dir = client->rootfh;
    char* name = g_file_set[1];
    args.where.name = name;
    args.how.mode = GUARDED;
    args.how.createhow3_u.obj_attributes = TESTCASE2_OBJATTR;

    int ret = rpc_nfs3_create_async(rpc, nfs_testcase_create_guarded_replicate_cb, &args, client);
    if (ret) {
        fprintf(stderr, "Failed to send create request|ret:%d\n", ret);
        exit(10);
    }
}

void nfs_testcase_create_unchecked_cb(struct rpc_context *rpc, int status, void *data , void *private_data)
{
    struct client *client = private_data;

    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "connection to RPC.NFSD on server %s failed, status: %d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "\nTESTCASE1: Send CREATE UNCHECKED Request\n");

    struct CREATE3args args;
    memset((void *)&args, 0, sizeof(args)); 
    args.where.dir = client->rootfh; 
    char *name = g_file_set[0];
    args.where.name = name;
    args.how.mode = UNCHECKED;
    args.how.createhow3_u.obj_attributes = TESTCASE1_OBJATTR;
    if (rpc_nfs3_create_async(rpc, nfs_testcase_create_guarded_cb, &args, client) != 0) {
        fprintf(stderr, "TESTCASE CREATE UNCHECKED: Failed to send create request\n");
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
    client.test_case_cb = nfs_testcase_create_unchecked_cb; 

    memset(&CREATE_PATHCONF, 0, sizeof(PATHCONF3resok));

    g_test_file_num = 3; 
    strcpy(g_file_set[0], "create_unchecked.txt");
    strcpy(g_file_set[1], "create_guarded.txt");
    strcpy(g_file_set[2], "create_exclusive.txt"); 

    struct rpc_context *rpc = rpc_init_context();
    rpc_set_auth(rpc, authnone_create());
    drive_frame_with_rpc(client, rpc);

    return 0;
}
