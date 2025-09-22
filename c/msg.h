#ifndef __MSG_H__
#define __MSG_H__

#include <asm/byteorder.h>
#include <stdint.h>

typedef	int       Boolean;
typedef uint8_t   Enumeration8;
typedef uint16_t  Enumeration16;
typedef int8_t    Integer8;
typedef uint8_t   UInteger8;
typedef int16_t   Integer16;
typedef uint16_t  UInteger16;
typedef int32_t   Integer32;
typedef uint32_t  UInteger32;
typedef int64_t   Integer64;
typedef uint64_t  UInteger64; /* ITU-T G.8275.2 */
typedef uint8_t   Octet;

#define PACKED __attribute__((packed))

struct mac_addr {
    uint8_t octet[6];
};
#define ETHERTYPE_PTP 0x88f7

struct Timestamp {
    uint16_t seconds_msb;
    uint32_t seconds_lsb;
    UInteger32 nanoseconds;
} PACKED;

struct ClockIdentity {
    Octet id[8];
};

struct PortIdentity {
    struct ClockIdentity clockIdentity;
    UInteger16 portNumber;
} PACKED;

struct eth_header {
    struct mac_addr dst;
    struct mac_addr src;
    uint16_t ethertype;
} PACKED;

struct ptp_header {
    union {
        struct {
            unsigned messageType : 4;
            unsigned majorSdoId : 4;
        } PACKED;
        uint8_t mt_msi;
    };
    union {
        struct {
            unsigned versionPtp : 4;
            unsigned minorVersionPtp : 4;
        } PACKED;
        uint8_t version;
    };
    UInteger16 messageLength;
    UInteger8 domainNumber;
    Octet minorSdoId;
    Octet flags[2];
    Integer64 correctionField;
    UInteger32 messageTypeSpecific;
    struct PortIdentity portIdentity;
    UInteger16 sequenceId;
    UInteger8 controlField;
    Integer8 logMessagePeriod;
} PACKED;

struct ptp_sync_msg {
    struct ptp_header header;
    struct Timestamp originTimeStamp;
} PACKED;

struct eth_ptp_msg {
    struct eth_header eth;
    struct ptp_sync_msg ptp;
} PACKED;

static inline uint64_t hton64(uint64_t v) {
    return __cpu_to_be64(v);
}

static inline uint64_t ntoh64(uint64_t v) {
    return __be64_to_cpu(v);
}

void hton_ptp_header(struct ptp_header* hdr) {
    hdr->messageLength = htons(hdr->messageLength);
    hdr->portIdentity.portNumber = htons(hdr->portIdentity.portNumber);
    hdr->sequenceId = htons(hdr->sequenceId);
    hdr->correctionField = hton64(hdr->correctionField);
}

void ntoh_ptp_header(struct ptp_header* hdr) {
    hdr->messageLength = ntohs(hdr->messageLength);
    hdr->portIdentity.portNumber = ntohs(hdr->portIdentity.portNumber);
    hdr->sequenceId = ntohs(hdr->sequenceId);
    hdr->correctionField = ntoh64(hdr->correctionField);
}

void hton_timestamp(struct Timestamp* ts) {
    ts->seconds_msb = htons(ts->seconds_msb);
    ts->seconds_lsb = htonl(ts->seconds_lsb);
    ts->nanoseconds = htonl(ts->nanoseconds);
}

void ntoh_timestamp(struct Timestamp* ts) {
    ts->seconds_msb = ntohs(ts->seconds_msb);
    ts->seconds_lsb = ntohl(ts->seconds_lsb);
    ts->nanoseconds = ntohl(ts->nanoseconds);
}

void hton_ptp_sync_msg(struct ptp_sync_msg* msg) {
    hton_ptp_header(&msg->header);
    hton_timestamp(&msg->originTimeStamp);
}

void ntoh_ptp_sync_msg(struct ptp_sync_msg* msg) {
    ntoh_ptp_header(&msg->header);
    ntoh_timestamp(&msg->originTimeStamp);
}

void print_ptp_header(const char* prefix, struct ptp_header* hdr) {
    uint64_t clockIdentity =
          (uint64_t)hdr->portIdentity.clockIdentity.id[7] << 0
        | (uint64_t)hdr->portIdentity.clockIdentity.id[6] << 8
        | (uint64_t)hdr->portIdentity.clockIdentity.id[5] << 16
        | (uint64_t)hdr->portIdentity.clockIdentity.id[4] << 24
        | (uint64_t)hdr->portIdentity.clockIdentity.id[3] << 32
        | (uint64_t)hdr->portIdentity.clockIdentity.id[2] << 40
        | (uint64_t)hdr->portIdentity.clockIdentity.id[1] << 48
        | (uint64_t)hdr->portIdentity.clockIdentity.id[0] << 56;
    printf("%sheader:\n", prefix);
    printf("%s  messageType: 0x%x\n", prefix,hdr->messageType);
    printf("%s  majorSdoId: 0x%x\n", prefix, hdr->majorSdoId);
    printf("%s  versionPtp: 0x%x\n", prefix, hdr->versionPtp);
    printf("%s  minorVersionPtp: 0x%x\n", prefix, hdr->minorVersionPtp);
    printf("%s  messageLength: 0x%0x\n", prefix, hdr->messageLength);
    printf("%s  domainNumber: 0x%0x\n", prefix, hdr->domainNumber);
    printf("%s  minorSdoId: 0x%x\n", prefix, hdr->minorSdoId);
    printf("%s  flags: 0x%0x%0x\n", prefix, hdr->flags[1], hdr->flags[0]);
    printf("%s  correctionField: %ld\n", prefix, hdr->correctionField);
    printf("%s  messageTypeSpecific: 0x%0x\n", prefix, hdr->messageTypeSpecific);
    printf("%s  clockIdentity: 0x%0lx\n", prefix, clockIdentity);
    printf("%s  portNumber: 0x%0x\n", prefix, hdr->portIdentity.portNumber);
    printf("%s  sequenceId: %d\n", prefix, hdr->sequenceId);
    printf("%s  controlField: 0x%0x\n", prefix, hdr->controlField);
    printf("%s  logMessagePeriod: %d\n", prefix, hdr->logMessagePeriod);
}

void print_timestamp(const char* prefix, struct Timestamp* ts) {
    uint64_t secs = (uint64_t)ts->seconds_msb << 32 | ts->seconds_lsb;
    printf("%soriginTimeStamp:\n", prefix);
    printf("%s  secs: %ld\n", prefix, secs);
    printf("%s  nsecs: %d\n", prefix, ts->nanoseconds);
}

void print_ptp_sync_msg(struct ptp_sync_msg* msg) {
    printf("ptp_msg:\n");
    print_ptp_header("  ", &msg->header);
    print_timestamp("  ", &msg->originTimeStamp);
}

#endif // __MSG_H__
