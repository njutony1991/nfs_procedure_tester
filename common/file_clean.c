#include "frame_driver.h"

char g_file_set[20][100];
int g_test_file_num = 0;

struct private_args {
    int file_index; 
    int is_finish;
    struct nfs_fh3 rootfh;       
    struct client *client; 
};

static struct private_args p_args;

static void frame_cleanup_file_cb(struct rpc_context *rpc , int status, void *data, void *private_data);

static int cleanup_files_impl(struct rpc_context *rpc, struct nfs_fh3 rootfh, struct client *client, int is_finish, int file_index) {
    if (file_index == g_test_file_num){
         client->is_finished = is_finish;
         return 0;
    }

    struct REMOVE3args args;
    memset((void*)&args, 0, sizeof(args));
    args.object.dir = rootfh;
    args.object.name = g_file_set[file_index];

    memset(&p_args, 0, sizeof(p_args));
    p_args.file_index = file_index+1; 
    p_args.rootfh = rootfh;
    p_args.client = client;
    p_args.is_finish = is_finish;

    fprintf(stdout, "Send Request to REMOVE file: %s\n", g_file_set[file_index]);
    int ret = rpc_nfs3_remove_async(rpc, frame_cleanup_file_cb, &args, &p_args);
    if (ret) {
        fprintf(stderr, "Failed to send remove request|ret:%d|file:%s\n", ret, g_file_set[file_index]);
        return -1;  
    }
}

static void frame_cleanup_file_cb(struct rpc_context *rpc , int status, void *data, void *private_data) {
    struct private_args *args = private_data;
    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "nfs/remove call failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "nfs/remove call to server failed, status:%d\n", status);
        exit(10);
    }

    fprintf(stdout, "Got reply from server for NFS/remove procedure.\n");
    REMOVE3res *res = data;
    if (res->status != NFS3_OK) {
        fprintf(stderr, "REMOVE %s not ok|status:%d\n", g_file_set[args->file_index-1], res->status);
    } else {
        fprintf(stdout, "REMOVE %s ok|status:%d\n", g_file_set[args->file_index-1], res->status);
    }

    cleanup_files_impl(rpc, args->rootfh, args->client, args->is_finish, args->file_index);
}

int cleanup_test_files(struct rpc_context *rpc, struct nfs_fh3 rootfh, struct client *client, int is_finish) {
    fprintf(stdout, "Clean Up Test Files\n");
    int ret = cleanup_files_impl(rpc, rootfh, client, is_finish, 0);
    if (ret) {
        fprintf(stderr, "Failed to send remove request|ret:%d\n", ret);
        return -1;
    }
    return 0;
}

