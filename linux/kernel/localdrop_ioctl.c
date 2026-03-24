#include <linux/tpm.h>
#include <asm/unaligned.h>
#include "../include/localdrop_ioctl.h"

int localdrop_tpm_unseal(struct localdrop_ioctl_request *req, uint8_t *out_seed)
{
    struct tpm_chip *chip;
    struct tpm_buf buf;
    uint32_t object_handle;
    uint16_t data_len;
    int rc;

    chip = tpm_default_chip();
    if (!chip)
        return -ENODEV;

    // --- STEP 1: TPM2_Load ---
    // Loads the sealed blobs into TPM transient memory using the parent handle
    rc = tpm_buf_init(&buf, TPM2_ST_SESSIONS, TPM2_CC_LOAD);
    if (rc)
        return rc;

    tpm_buf_append_u32(&buf, req->parent_handle);
    tpm_buf_append_u32(&buf, 9); // Auth Area size (Empty Password)
    tpm_buf_append_u32(&buf, TPM2_RS_PW);
    tpm_buf_append_u16(&buf, 0);
    tpm_buf_append_u8(&buf, 0);
    tpm_buf_append_u16(&buf, 0);

    tpm_buf_append_u16(&buf, req->pub_len);
    tpm_buf_append(&buf, req->pub_blob, req->pub_len);
    tpm_buf_append_u16(&buf, req->priv_len);
    tpm_buf_append(&buf, req->priv_blob, req->priv_len);

    rc = tpm_transmit_cmd(chip, &buf, 0, "localdrop: TPM2_Load");
    if (rc)
        goto out_free;

    // Extract the transient handle assigned to the loaded object
    object_handle = get_unaligned_be32(&buf.data[TPM_HEADER_SIZE]);

    // --- STEP 2: TPM2_Unseal ---
    // Retrieves the 32-byte secret from the loaded object
    tpm_buf_reset(&buf, TPM2_ST_SESSIONS, TPM2_CC_UNSEAL);
    tpm_buf_append_u32(&buf, object_handle);
    tpm_buf_append_u32(&buf, 9); // Auth Area (Empty Password)
    tpm_buf_append_u32(&buf, TPM2_RS_PW);
    tpm_buf_append_u16(&buf, 0);
    tpm_buf_append_u8(&buf, 0);
    tpm_buf_append_u16(&buf, 0);

    rc = tpm_transmit_cmd(chip, &buf, 0, "localdrop: TPM2_Unseal");
    if (rc)
        goto out_flush;

    // Extract the unsealed data (skipping the 16-bit length header)
    data_len = get_unaligned_be16(&buf.data[TPM_HEADER_SIZE + 4]);
    if (data_len == 32)
    {
        memcpy(out_seed, &buf.data[TPM_HEADER_SIZE + 6], 32);
    }
    else
    {
        rc = -EIO;
    }

out_flush:
    // --- STEP 3: TPM2_FlushContext ---
    // Release the transient slot in the TPM
    tpm_buf_reset(&buf, TPM2_ST_NO_SESSIONS, TPM2_CC_FLUSH_CONTEXT);
    tpm_buf_append_u32(&buf, object_handle);
    tpm_transmit_cmd(chip, &buf, 0, "localdrop: TPM2_Flush");

out_free:
    tpm_buf_destroy(&buf);
    return rc;
}