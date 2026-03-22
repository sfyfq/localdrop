#ifndef INTERCEPTOR_H
#define INTERCEPTOR_H

/* Callback type invoked when the hotkey combination is detected.
 * Implemented in main.c — wires into netlink in phase 2. */
typedef void (*interceptor_callback_t)(void);

int interceptor_init(interceptor_callback_t cb);
void interceptor_exit(void);

#endif /* INTERCEPTOR_H */