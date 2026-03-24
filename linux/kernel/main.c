#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include "../include/interceptor.h"
#include "../include/localdrop_dev.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("localdrop");
MODULE_DESCRIPTION("localdrop kernel module - key interceptor, TPM sign, tty inject");
MODULE_VERSION("0.1");

/* ─── hotkey callback ──────────────────────────────────────── */

/* Called by hotkey.c when Ctrl+Shift+Alt+V is detected.
 * Phase 2 will add: netlink_send_trigger() here. */
static void on_interceptor(enum Hotkey key)
{
    pr_info("localdrop: hotkey fired\n");
}

static int __init localdrop_init(void)
{
    int ret;
    pr_info("localdrop: loading\n");
    ret = interceptor_init(on_interceptor);
    if (ret)
    {
        pr_err("localdrop: interceptor_init failed (%d)\n", ret);
        goto err_interceptor_exit;
    }

    ret = localdrop_dev_init();
    if (ret)
    {
        pr_err("localdrop: char dev init failed (%d)\n", ret);
        goto err_dev_exit;
    }

    pr_info("localdrop: ready\n");
    return 0;

err_dev_exit:
    localdrop_dev_exit();
err_interceptor_exit:
    interceptor_exit();
    return 1;
}

static void __exit localdrop_exit(void)
{
    interceptor_exit();
    localdrop_dev_exit();
    pr_info("localdrop: unloaded\n");
}

module_init(localdrop_init);
module_exit(localdrop_exit);