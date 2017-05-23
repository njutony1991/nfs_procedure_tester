#include "frame_driver.h"

nfs_fh3 g_wrong_fh;
nfs_fh3 g_stale_fh;

int generate_wrong_fh(size_t data_len) {
    g_wrong_fh.data.data_len = data_len;
    g_wrong_fh.data.data_val = malloc(data_len);
    if (g_wrong_fh.data.data_val == NULL) {
        fprintf(stderr,"generate_wrong_fh malloc failed: %s\n", strerror(errno));
        return -1; 
    }
    memset(g_wrong_fh.data.data_val, 0, data_len);
    return 0;
}

void cleanup_wrong_fh() {
    g_wrong_fh.data.data_len = 0;
    free(g_wrong_fh.data.data_val);
    g_wrong_fh.data.data_val = NULL;
}

int generate_stale_fh(nfs_fh3 origin_fh) {
    g_stale_fh.data.data_len = origin_fh.data.data_len; 
    g_stale_fh.data.data_val = malloc(g_stale_fh.data.data_len);
    if (g_stale_fh.data.data_val == NULL) {
        fprintf(stderr,"generate_stale_fh malloc failed: %s\n", strerror(errno)); 
        return -1;
    }
    memcpy(g_stale_fh.data.data_val, origin_fh.data.data_val, g_stale_fh.data.data_len); 
    size_t index = g_stale_fh.data.data_len-1;
    g_stale_fh.data.data_val[index] = g_stale_fh.data.data_val[index]+3; 
    return 0;
}

void cleanup_stale_fh() {
    g_stale_fh.data.data_len = 0;
    free(g_stale_fh.data.data_val);
    g_stale_fh.data.data_val = NULL;
}
