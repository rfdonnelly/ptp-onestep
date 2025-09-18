#include <arpa/inet.h>
#include <assert.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "msg.h"

int interface_index(int fd, const char* ifname) {
    struct ifreq ifreq = {};
    strncpy(ifreq.ifr_name, ifname, strlen(ifname));
    int err = ioctl(fd, SIOCGIFINDEX, &ifreq);
    assert(!err);
    return ifreq.ifr_ifindex;
}

struct mac_addr interface_mac_addr(int fd, const char* ifname) {
    struct ifreq ifreq = {};
    strncpy(ifreq.ifr_name, ifname, strlen(ifname));
    int err = ioctl(fd, SIOCGIFHWADDR, &ifreq);
    assert(!err);
    return *(struct mac_addr*)&ifreq.ifr_hwaddr.sa_data;
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

// Reference: linuxptp:sk.c
void enable_timestamping(int fd, const char *ifname) {
    int err = 0;

    // NOTE: Driver seems to require an RX filter even though we aren't
    // interested in RX here.  Without an RX filter, the ioctl call
    // returns an error.
    struct hwtstamp_config hwtstamp_config = {
        .tx_type = HWTSTAMP_TX_ONESTEP_SYNC,
        .rx_filter = HWTSTAMP_FILTER_PTP_V2_L2_EVENT,
    };

    struct ifreq ifreq = {
        .ifr_data = (void*)&hwtstamp_config,
    };
    strncpy(ifreq.ifr_name, ifname, strlen(ifname));

    // printf("DEBUG fd:%d hwtstamp_config:{tx_type:0x%x rx_filter:0x%x}\n", fd, hwtstamp_config.tx_type, hwtstamp_config.rx_filter);
    err = ioctl(fd, SIOCSHWTSTAMP, &ifreq);
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

void print_buf(uint8_t* buf, size_t size) {
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

void create_sync(struct mac_addr src, struct ptp_sync_msg* frame) {
    *frame = (struct ptp_sync_msg){
        .eth = {
            .dst = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e},
            .src = src,
            .ethertype = htons(ETHERTYPE_PTP),
        },
        .header = {
            .majorSdoId = 1,
            .minorVersionPtp = 1,
            .versionPtp = 2,
            .messageLength = htons(44),
            .portIdentity = {
                .clockIdentity = {
                    src.octet[0],
                    src.octet[1],
                    src.octet[2],
                    0xff,
                    0xfe,
                    src.octet[3],
                    src.octet[4],
                    src.octet[5],
                },
                .portNumber = htons(1),
            },
            .sequenceId = htons(23),
            .logMessagePeriod = -3,
        },
    };
}

int main() {
    const char* ifname = "eth1";
    int fd = create_socket(ifname);
    enable_timestamping(fd, ifname);

    struct ptp_sync_msg buf;
    struct mac_addr src_addr = interface_mac_addr(fd, ifname);
    create_sync(src_addr, &buf);

    print_buf((uint8_t*)&buf, sizeof(buf));

    send(fd, &buf, sizeof(buf), 0);
}
