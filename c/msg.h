#ifndef __MSG_H__
#define __MSG_H__

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
    struct eth_header eth;
    struct ptp_header header;
    struct Timestamp originTimeStamp;
} PACKED;

#endif // __MSG_H__
