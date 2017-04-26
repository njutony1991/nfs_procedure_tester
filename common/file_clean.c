#include "frame_driver.h"

char g_file_set[20][100];
int g_test_file_num = 0;

void frame_cleanup_file_cb(struct rpc_context *rpc , int status, void *data, void *private_data){
    char *to_remove = private_data;
    if (status == RPC_STATUS_ERROR) {
        printf("nfs/remove call failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        printf("nfs/remove call to server failed, status:%d\n", status);
        exit(10);
    }
    printf("Got reply from server for NFS/remove procedure.\n");
    REMOVE3res *res = data;
    if (res->status != NFS3_OK) {
        printf("REMOVE %s not ok|status:%d\n", to_remove, res->status);
    } else {
        printf("REMOVE %s ok|status:%d\n", to_remove, res->status);
    }
}

int cleanup_test_files(struct rpc_context *rpc, struct nfs_fh3 rootfh){
    int i;
    for (i=0;i<g_test_file_num;i++) { 
        struct REMOVE3args args;
        memset((void*)&args, 0, sizeof(args));
        args.object.dir = rootfh;
        args.object.name = g_file_set[i];

        int ret = rpc_nfs3_remove_async(rpc, frame_cleanup_file_cb, &args, g_file_set[i]);
        if (ret) {
            printf("Failed to send remove request|ret:%d\n", ret);
            return -1;
        }
    }
    return 0;
}

