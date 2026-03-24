#ifndef LOCALDROP_IOCTL_H
#define LOCALDROP_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

// Max sizes for TPM2B blobs
#define MAX_BLOB_SIZE 512

struct localdrop_ioctl_request
{
    __u32 parent_handle;
    __u8 pub_blob[512];
    __u16 pub_len;
    __u8 priv_blob[512];
    __u16 priv_len;

    /* The data to be signed (usually the 32-byte X25519 public key) */
    __u8 payload[32];
    /* The resulting 64-byte Ed25519 signature */
    __u8 signature[64];
};

// Define the IOCTL command
#define LOCALDROP_IOC_MAGIC 'k'
#define LOCALDROP_IOC_SIGN _IOWR(LOCALDROP_IOC_MAGIC, 1, struct localdrop_ioctl_request)
int localdrop_tpm_unseal(struct localdrop_ioctl_request *req, uint8_t *out_seed);
#endif