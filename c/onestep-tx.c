#include <arpa/inet.h>
#include <assert.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

typedef uint8_t mac_addr[6];
#define ETHERTYPE_PTP 0x88f7

struct eth_header {
    mac_addr dst;
    mac_addr src;
    uint16_t ethertype;
} __attribute__((packed));

struct ieee1588sync {
    struct eth_header eth;
    union {
        struct {
            unsigned messageType : 4;
            unsigned majorSdoId : 4;
            unsigned versionPtp : 4;
            unsigned minorVersionPtp : 4;
        } __attribute__((packed));
        uint16_t version;
    };
    uint16_t messageLength;
    uint8_t domainNumber;
    uint8_t minorSdoId;
    uint16_t flags;
    uint64_t correctionField;
    uint32_t messageTypeSpecific;
    uint8_t clockIdentity[8];
    uint16_t sourcePortId;
    uint16_t sequenceId;
    uint8_t controlField;
    uint8_t logMessagePeriod;
    uint8_t originTimeStampS[6];
    uint32_t originTimeStampNS;
} __attribute__((packed));

int interface_index(int fd, const char* ifname) {
    struct ifreq ifreq = {};
    strncpy(ifreq.ifr_name, ifname, strlen(ifname));
    int err = ioctl(fd, SIOCGIFINDEX, &ifreq);
    assert(!err);
    return ifreq.ifr_ifindex;
}

void interface_mac_addr(int fd, const char* ifname, uint8_t* addr) {
    struct ifreq ifreq = {};
    strncpy(ifreq.ifr_name, ifname, strlen(ifname));
    int err = ioctl(fd, SIOCGIFHWADDR, &ifreq);
    assert(!err);
    memcpy(addr, ifreq.ifr_hwaddr.sa_data, 6);
}

// Reference: linuxptp:raw.c
int create_socket(const char* ifname) {
    int err;

    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_1588));
    assert(fd);

    int ifindex = interface_index(fd, ifname);
    assert(ifindex);

    struct sockaddr_ll addr = {
        .sll_ifindex = ifindex,
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ETH_P_1588),
    };
    err = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    assert(!err);

    err = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname));
    assert(!err);

    return fd;
}

void enable_timestamping(int fd, const char *ifname) {
    int err = 0;

    struct hwtstamp_config hwtstamp_config = {
        .tx_type = HWTSTAMP_TX_ONESTEP_SYNC,
        .rx_filter = HWTSTAMP_FILTER_NONE,
    };

    struct ifreq ifreq = {
        .ifr_data = (void*)&hwtstamp_config,
    };
    strncpy(ifreq.ifr_name, ifname, strlen(ifname));

    err = ioctl(fd, SIOCGHWTSTAMP, &ifreq);
    assert(!err);

    struct so_timestamping so_timestamping = {
        .flags =
            SOF_TIMESTAMPING_TX_HARDWARE |
            SOF_TIMESTAMPING_RAW_HARDWARE,
    };
    err = setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, &so_timestamping, sizeof(so_timestamping));
    assert(!err);

    int value = 1;
    err = setsockopt(fd, SOL_SOCKET, SO_SELECT_ERR_QUEUE, &value, sizeof(value));
    assert(!err);
}

void print_pkt(uint8_t* buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        printf("%02x ", buf[i]);
        if (i % 16 == 15) {
            printf("\n");
        } else if (i % 8 == 7) {
            printf(" ");
        }
    }
    printf("\n");
}

void send_sync(int fd, const char* ifname) {
    mac_addr src;
    interface_mac_addr(fd, ifname, (uint8_t*)&src);

    struct ieee1588sync pkt = {
        .eth = {
            .dst = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e},
            .ethertype = htons(ETHERTYPE_PTP),
        },
        .majorSdoId = 1,
        .minorVersionPtp = 1,
        .versionPtp = 2,
        .messageLength = htons(44),
        .sourcePortId = htons(1),
        .sequenceId = htons(23),
        .logMessagePeriod = -3,
    };
    memcpy(pkt.eth.src, src, 6);
    memcpy(pkt.clockIdentity, src, 3);
    pkt.clockIdentity[3] = 0xff;
    pkt.clockIdentity[4] = 0xfe;
    memcpy(&pkt.clockIdentity[5], &src[3], 3);

    printf("Sending pkt:\n");
    print_pkt((uint8_t*)&pkt, sizeof(pkt));

    send(fd, &pkt, sizeof(pkt), 0);
}

int main() {
    const char* ifname = "eth1";
    int fd = create_socket(ifname);
    enable_timestamping(fd, ifname);

    send_sync(fd, ifname);
}
