#include <linux/module.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "../include/interceptor.h"

/* ─── modifier state ───────────────────────────────────────── */

/* Each bit tracks whether that modifier is currently held.
 * Using a single u8 rather than three bools avoids any race
 * where a partial read sees an inconsistent combination. */
#define MOD_CTRL (1 << 0)
#define MOD_SHIFT (1 << 1)
#define MOD_ALT (1 << 2)
#define MOD_ALL (MOD_CTRL | MOD_SHIFT | MOD_ALT)

typedef u8 modState_t;

/* Per-device state — allocated in connect(), freed in disconnect() */
struct interceptor_handle
{
    struct input_handle handle; /* must be first member */
    modState_t mod_state;       /* modifier bitmask for THIS device only */
};

/* Callback registered by main.c */
static interceptor_callback_t interceptor_cb = NULL;

/* ─── forward declarations ─────────────────────────────────── */

static int interceptor_connect(struct input_handler *handler,
                               struct input_dev *dev,
                               const struct input_device_id *id);
static void interceptor_disconnect(struct input_handle *handle);
static bool interceptor_filter(struct input_handle *handle,
                               unsigned int type,
                               unsigned int code,
                               int value);

/* ─── input subsystem registration ────────────────────────── */

/* Match any device that produces EV_KEY events (i.e. keyboards). */
static const struct input_device_id interceptor_ids[] = {
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT,
        .evbit = {BIT_MASK(EV_KEY)},
    },
    {} /* terminating entry required by input subsystem */
};
MODULE_DEVICE_TABLE(input, interceptor_ids);

static struct input_handler interceptor_handler = {
    .filter = interceptor_filter,
    .connect = interceptor_connect,
    .disconnect = interceptor_disconnect,
    .name = "ld_interceptor",
    .id_table = interceptor_ids,
};

/* ─── event handler ────────────────────────────────────────── */

/* Returns the modifier bit for a given keycode, or 0 if not a modifier */
static u8 mod_bit_for_key(unsigned int code)
{
    switch (code)
    {
    case KEY_LEFTCTRL:
    case KEY_RIGHTCTRL:
        return MOD_CTRL;
    case KEY_LEFTSHIFT:
    case KEY_RIGHTSHIFT:
        return MOD_SHIFT;
    case KEY_LEFTALT:
    case KEY_RIGHTALT:
        return MOD_ALT;
    default:
        return 0;
    }
}

/* Updates mod_state and returns true if state changed */
static bool update_modifiers(struct interceptor_handle *hh,
                             unsigned int code,
                             int value)
{
    u8 bit = mod_bit_for_key(code);
    if (!bit)
        return false;

    if (value == 1)
        hh->mod_state |= bit;
    else
        hh->mod_state &= ~bit;
    return true;
}
static bool interceptor_filter(struct input_handle *handle,
                               unsigned int type,
                               unsigned int code,
                               int value)
{
    /* Only process key press (1) and release (0).
     * value == 2 is key repeat — ignore to avoid re-triggering. */
    if (!handle)
    {
        return false;
    }

    if (type != EV_KEY || value == 2)
        return false;

    struct interceptor_handle *hh = container_of(handle, struct interceptor_handle, handle);

    if (!hh)
    {
        return false;
    }

    if (update_modifiers(hh, code, value))
        return false;

    if ((hh->mod_state & MOD_ALL) != MOD_ALL)
    {
        return false;
    }

    switch (code)
    {
    case KEY_V:
    {
        if (value == 1)
        {
            pr_info("ld_hotkey: Ctrl+Shift+Alt+V on [%s]\n",
                    handle->dev->name);
            if (interceptor_cb)
                interceptor_cb(PASTE);
        }
        return true; /* suppress press and release */
    }
    case KEY_C:
        if (value == 1)
        {
            pr_info("ld_hotkey: Ctrl+Shift+Alt+V on [%s]\n",
                    handle->dev->name);
            if (interceptor_cb)
                interceptor_cb(COPY);
        }
        return true; /* suppress press and release */
    }
    return false;
}

/* ─── connect / disconnect ─────────────────────────────────── */

static int interceptor_connect(struct input_handler *handler,
                               struct input_dev *dev,
                               const struct input_device_id *id)
{
    struct interceptor_handle *hh;
    int err;

    hh = kzalloc(sizeof(*hh), GFP_KERNEL);
    if (!hh)
        return -ENOMEM;

    hh->handle.dev = dev;
    hh->handle.handler = handler;
    hh->handle.name = "ld_interceptor_handle";

    err = input_register_handle(&hh->handle);
    if (err)
    {
        pr_err("ld_interceptor: failed to register handle for %s\n", dev->name);
        goto err_free;
    }

    err = input_open_device(&hh->handle);
    if (err)
    {
        pr_err("ld_interceptor: failed to open device %s\n", dev->name);
        goto err_unregister;
    }

    pr_info("ld_interceptor: connected to [%s]\n", dev->name);
    return 0;

err_unregister:
    input_unregister_handle(&hh->handle);
err_free:
    kfree(hh);
    return err;
}

static void interceptor_disconnect(struct input_handle *handle)
{
    struct interceptor_handle *hh = container_of(handle,
                                                 struct interceptor_handle,
                                                 handle);

    pr_info("ld_interceptor: disconnected from [%s]\n", handle->dev->name);
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(hh); // free the entire object
}

/* ─── public API ───────────────────────────────────────────── */

int interceptor_init(interceptor_callback_t cb)
{
    int err;

    if (!cb)
    {
        pr_err("ld_interceptor: no callback provided\n");
        return -EINVAL;
    }

    interceptor_cb = cb;

    err = input_register_handler(&interceptor_handler);
    if (err)
    {
        pr_err("ld_interceptor: failed to register input handler\n");
        return err;
    }

    pr_info("ld_interceptor: input handler registered\n");
    return 0;
}

void interceptor_exit(void)
{
    input_unregister_handler(&interceptor_handler);
    interceptor_cb = NULL;
    pr_info("ld_interceptor: unregistered\n");
}
