#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include "interceptor.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("localdrop");
MODULE_DESCRIPTION("localdrop kernel module - hotkey + netlink + tty inject");
MODULE_VERSION("0.1");

/* ─── hotkey callback ──────────────────────────────────────── */

/* Called by hotkey.c when Ctrl+Shift+Alt+V is detected.
 * Phase 2 will add: netlink_send_trigger() here. */
static void on_interceptor(void)
{
    pr_info("localdrop: hotkey fired\n");
    /* TODO phase 2: netlink_send_trigger() */
}

/* ─── init / exit ──────────────────────────────────────────── */

static int __init localdrop_init(void)
{
    int err;

    pr_info("localdrop: loading\n");

    err = interceptor_init(on_interceptor);
    if (err)
    {
        pr_err("localdrop: interceptor_init failed (%d)\n", err);
        return err;
    }

    /* TODO phase 2: netlink_init() */
    /* TODO phase 4: tty_inject_init() */

    pr_info("localdrop: ready\n");
    return 0;
}

static void __exit localdrop_exit(void)
{
    /* TODO phase 4: tty_inject_exit() */
    /* TODO phase 2: netlink_exit() */
    interceptor_exit();
    pr_info("localdrop: unloaded\n");
}

module_init(localdrop_init);
module_exit(localdrop_exit);