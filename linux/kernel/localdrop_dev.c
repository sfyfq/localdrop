#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include "../include/localdrop_ioctl.h"
#include "../include/access_control.h"
#include "ed25519/src/ed25519.h"
#include "../include/localdrop_dev.h"

#define DEVICE_NAME "localdrop"
#define CLASS_NAME "localdrop"
static dev_t localdrop_dev;
static struct cdev localdrop_cdev;
static struct class *localdrop_class;

static int localdrop_open(struct inode *inode, struct file *file);
static long localdrop_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int localdrop_release(struct inode *inode, struct file *file);

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = localdrop_open,
    .release = localdrop_release,
    .unlocked_ioctl = localdrop_ioctl,
};

static int localdrop_open(struct inode *inode, struct file *file)
{
    return localdrop_dev_atomic_open(inode, file);
}
static int localdrop_release(struct inode *inode, struct file *file)
{
    return localdrop_dev_atomic_release(inode, file);
}

static long localdrop_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct localdrop_ioctl_request *req = kzalloc(sizeof(*req), GFP_KERNEL);
    if (!req)
    {
        pr_err("Unable to allocate memory for request");
        return -ENOMEM;
    }

    int ret;

    switch (cmd)
    {
    case LOCALDROP_IOC_SIGN:
        // 1. Safely copy the blobs from the Go daemon
        if (copy_from_user(req, (void __user *)arg, sizeof(*req)))
        {
            ret = -EFAULT;
            goto out;
        }

        // 2. Validate lengths
        if (req->pub_len > MAX_BLOB_SIZE || req->priv_len > MAX_BLOB_SIZE)
        {
            ret = -EINVAL;
            goto out;
        }
        uint8_t unsealed_seed[32];
        uint8_t public_key[32];
        uint8_t private_key[64];
        uint8_t signature[64];

        ret = localdrop_tpm_unseal(req, unsealed_seed);
        if (ret)
        {
            pr_err("localdrop: TPM unseal failed with rc=%d\n", ret);
            memzero_explicit(unsealed_seed, 32);
            goto out;
        }
        /*
         * 1. Expand the 32-byte seed into the 64-byte keypair.
         * This derives the public_key and prepares the private_key buffer
         * required by the signing function.
         */
        ed25519_create_keypair(public_key, private_key, unsealed_seed);

        /*
         * 2. Generate the Ed25519 signature.
         * req->payload contains the 32-byte ephemeral X25519 public key.
         */
        ed25519_sign(
            (unsigned char *)signature,
            (const unsigned char *)req->payload,
            32,
            (const unsigned char *)public_key,
            (const unsigned char *)private_key);

        /* Clean up sensitive data */
        memzero_explicit(unsealed_seed, sizeof(unsealed_seed));
        memzero_explicit(private_key, sizeof(private_key));

        /* 4. Copy the 64-byte signature back to the user-space buffer */
        if (copy_to_user(req->signature, signature, sizeof(signature)))
        {
            ret = -EFAULT;
            goto out;
        }

        pr_info("localdrop: Received blobs for parent 0x%08x\n", req->parent_handle);
        return 0;

    default:
        ret = -ENOTTY;
        goto out;
    }

out:
    kfree(req); // Always free the memory
    return ret;
}

int localdrop_dev_init(void)
{
    int ret;
    struct device *dev;

    ret = alloc_chrdev_region(&localdrop_dev, 0, 1, DEVICE_NAME);
    if (ret < 0)
    {
        pr_err("localdrop: failed to allocate chrdev region: %d\n", ret);
        return ret;
    }

    cdev_init(&localdrop_cdev, &fops);
    localdrop_cdev.owner = THIS_MODULE;

    ret = cdev_add(&localdrop_cdev, localdrop_dev, 1);
    if (ret < 0)
    {
        pr_err("localdrop: cdev_add failed: %d\n", ret);
        goto err_unreg_chrdev;
    }

    localdrop_class = class_create(CLASS_NAME);
    if (IS_ERR(localdrop_class))
    {
        ret = PTR_ERR(localdrop_class);
        pr_err("localdrop: class_create failed: %d\n", ret);
        goto err_del_cdev;
    }

    dev = device_create(localdrop_class, NULL, localdrop_dev, NULL, DEVICE_NAME);
    if (IS_ERR(dev))
    {
        ret = PTR_ERR(dev);
        pr_err("localdrop: device_create failed: %d\n", ret);
        goto err_destroy_class;
    }

    pr_info("localdrop: registered as /dev/%s (major %d)\n",
            DEVICE_NAME, MAJOR(localdrop_dev));
    return 0;

err_destroy_class:
    class_destroy(localdrop_class);
err_del_cdev:
    cdev_del(&localdrop_cdev);
err_unreg_chrdev:
    unregister_chrdev_region(localdrop_dev, 1);
    return ret;
}

void localdrop_dev_exit(void)
{
    device_destroy(localdrop_class, localdrop_dev);
    class_destroy(localdrop_class);
    cdev_del(&localdrop_cdev);
    unregister_chrdev_region(localdrop_dev, 1);
    pr_info("localdrop: char dev unregistered\n");
}