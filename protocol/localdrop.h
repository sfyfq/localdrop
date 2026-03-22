#ifndef LOCALDROP_PROTO_H
#define LOCALDROP_PROTO_H

/* ─── type portability ─────────────────────────────────────── */
#if defined(__KERNEL__)
  #include <linux/types.h>
  typedef u32  ld_u32;
  typedef u16  ld_u16;
  typedef u8   ld_u8;
#else
  #include <stdint.h>
  typedef uint32_t  ld_u32;
  typedef uint16_t  ld_u16;
  typedef uint8_t   ld_u8;
#endif

/* ─── packing portability ──────────────────────────────────── */
#if defined(_MSC_VER)
  #define LD_PACKED_BEGIN  __pragma(pack(push, 1))
  #define LD_PACKED_END    __pragma(pack(pop))
  #define LD_PACKED
#else
  #define LD_PACKED_BEGIN
  #define LD_PACKED_END
  #define LD_PACKED        __attribute__((packed))
#endif

/* ─── protocol constants ───────────────────────────────────── */
#define LOCALDROP_VERSION         1
#define LOCALDROP_MAX_PAYLOAD   256

/* message types */
#define LD_MSG_TRIGGER            1    /* hotkey fired → daemon  */
#define LD_MSG_DATA               2    /* data response → kernel */
#define LD_MSG_ACK                3    /* generic ack            */
#define LD_MSG_ERROR              4    /* error response         */

/* error codes */
#define LD_ERR_OK                 0
#define LD_ERR_TLS_FAIL           1
#define LD_ERR_PEER_UNREACHABLE   2
#define LD_ERR_TIMEOUT            3
#define LD_ERR_INVALID_MSG        4

/* ─── wire format ──────────────────────────────────────────── */
LD_PACKED_BEGIN

struct ld_msg_header {
    ld_u8  version;                    /* always LOCALDROP_VERSION  */
    ld_u8  type;                       /* LD_MSG_* constant         */
    ld_u16 payload_len;                /* length of following data  */
    ld_u32 seq;                        /* sequence number           */
} LD_PACKED;

struct ld_msg_data {
    struct ld_msg_header header;
    char payload[LOCALDROP_MAX_PAYLOAD];
} LD_PACKED;

LD_PACKED_END

/* ─── helper macros ────────────────────────────────────────── */
#define LD_HEADER_SIZE   sizeof(struct ld_msg_header)
#define LD_MSG_SIZE      sizeof(struct ld_msg_data)

#endif /* LOCALDROP_PROTO_H */