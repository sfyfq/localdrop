#ifndef LOCALDROP_CONSTANTS_H
#define LOCALDROP_CONSTANTS_H

/* netlink (Linux only, ignored on other platforms) */
#define LD_NETLINK_FAMILY        31
#define LD_NETLINK_GROUP          1

/* network */
#define LD_DEFAULT_PORT        7777
#define LD_CONNECT_TIMEOUT_MS  1000
#define LD_READ_TIMEOUT_MS     1000

/* IPC (platform-specific implementations, shared names) */
#define LD_IPC_SOCKET_PATH   "/var/run/localdrop.sock"  /* Linux/macOS */
#define LD_IPC_PIPE_NAME     "\\\\.\\pipe\\localdrop"   /* Windows     */

#endif /* LOCALDROP_CONSTANTS_H */