#include "frame_driver.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mount_mnt_cb(struct rpc_context *rpc, int status, void *data, void *private_data)
{
    struct client *client = private_data;
    mountres3 *mnt = data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "mount/mnt call failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "mount/mnt call to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "Got reply from server for MOUNT/MNT procedure.\n");
	if (mnt->fhs_status == MNT3ERR_NAMETOOLONG) {
		fprintf(stderr, "mount/mnt name too long error\n");
		exit(10);
	}

    client->rootfh.data.data_len = mnt->mountres3_u.mountinfo.fhandle.fhandle3_len;
    client->rootfh.data.data_val = malloc(client->rootfh.data.data_len);
    memcpy(client->rootfh.data.data_val, mnt->mountres3_u.mountinfo.fhandle.fhandle3_val, client->rootfh.data.data_len);

    fprintf(stdout, "Disconnect socket from mountd server\n");
    if (rpc_disconnect(rpc, "normal disconnect") != 0) {
        fprintf(stderr, "Failed to disconnect socket to mountd\n");
        exit(10);
    }

    fprintf(stdout, "Connect to RPC.NFSD on %s:%d\n", client->server, 2049);
    if (rpc_connect_async(rpc, client->server, 2049, client->test_case_cb, client) != 0) {
        fprintf(stderr, "Failed to start connection\n");
        exit(10);
    }
}

void mount_null_cb(struct rpc_context *rpc, int status, void *data, void *private_data)
{
    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "mount null call failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "mount null call to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "Got reply from server for MOUNT/NULL procedure.\n");

    fprintf(stdout, "Send MOUNT/MNT command for %s\n", client->export);
    if (rpc_mount_mnt_async(rpc, mount_mnt_cb, client->export, client) != 0) {
        fprintf(stderr, "Failed to send mnt request\n");
        exit(10);
    }
}


void mount_connected_cb(struct rpc_context *rpc, int status, void *data , void *private_data)
{
    struct client *client = private_data;

    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "connection to RPC.MOUNTD on server %s failed\n", client->server);
        exit(10);
    }

    fprintf(stdout, "Connected to RPC.MOUNTD on %s:%d\n", client->server, client->mount_port);
    fprintf(stdout, "Send NULL request to check if RPC.MOUNTD is actually running\n");
    if (rpc_mount_null_async(rpc, mount_null_cb, client) != 0) {
        fprintf(stderr, "Failed to send null request\n");
        exit(10);
    }
}

void getport_mountd_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {
    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "portmapper getport call failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "portmapper getport call to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    client->mount_port = *(uint32_t *)data;
    fprintf(stdout, "GETPORT returned RPC.MOUNTD is on port:%d\n", client->mount_port);

    if (client->mount_port == 0) {
        fprintf(stderr, "RPC.MOUNTD is not available on server : %s:%d\n", client->server, client->mount_port);
        exit(10);
    }       

    fprintf(stdout, "Disconnect socket from portmap server\n");
    if (rpc_disconnect(rpc, "normal disconnect") != 0) {
        fprintf(stderr, "Failed to disconnect socket to portmapper\n");
        exit(10);
    }

    fprintf(stdout, "Connect to RPC.MOUNTD on %s:%d\n", client->server, client->mount_port);
    if (rpc_connect_async(rpc, client->server, client->mount_port, mount_connected_cb, client) != 0) {
        fprintf(stderr, "Failed to start connection\n");
        exit(10);
    }
}

void pmap2_connected_cb(struct rpc_context *rpc, int status, void *data, void *private_data) {
    struct client *client = private_data;

    if (status == RPC_STATUS_ERROR) {
        fprintf(stderr, "portmapper connected failed with \"%s\"\n", (char *)data);
        exit(10);
    }
    
    if (status != RPC_STATUS_SUCCESS) {
        fprintf(stderr, "portmapper connected to server %s failed, status:%d\n", client->server, status);
        exit(10);
    }

    fprintf(stdout, "Send getport request asking for MOUNT port\n");
    if (rpc_pmap2_getport_async(rpc, MOUNT_PROGRAM, MOUNT_V3, IPPROTO_TCP, getport_mountd_cb, client) != 0) {
        fprintf(stderr, "Failed to send getport request\n");
        exit(10);
    }
}

int drive_frame(struct client client) {
    struct rpc_context *rpc;
    rpc = rpc_init_context();
    if (rpc == NULL) {
        fprintf(stderr, "failed to init context\n");
        exit(10);
    }
	
	drive_frame_with_rpc(client, rpc);
    return 0;
}


int drive_frame_with_rpc(struct client client, struct rpc_context *rpc) {
    struct pollfd pfd;

    setbuf(stdout, NULL);
    if (rpc == NULL) {
        fprintf(stderr, "rpc context null\n");
        exit(10);
    }

    fprintf(stdout, "server:%s|export:%s\n", client.server, client.export);
    client.is_finished = 0;
    if (rpc_connect_async(rpc, client.server, 111, pmap2_connected_cb, &client) != 0) {
        fprintf(stderr, "Failed to start connection\n");
        exit(10);
    }

    for (;;) {
        pfd.fd = rpc_get_fd(rpc);
        pfd.events = rpc_which_events(rpc);

        if (poll(&pfd, 1, -1) < 0) {
            fprintf(stderr, "Poll failed");
            rpc_destroy_context(rpc);
            return -1; 
        }
        if (rpc_service(rpc, pfd.revents) < 0) {
            fprintf(stderr, "rpc_service failed\n");
            break;
        }
        if (client.is_finished) {
            break;
        }
    }
    
    rpc_destroy_context(rpc);
    rpc=NULL;
    fprintf(stdout, "nfsclient finished\n");
    return 0;
} 
