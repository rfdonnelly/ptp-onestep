#include <arpa/inet.h>
#include <assert.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "msg.h"
#include "socket.h"

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

struct ptp_sync_msg create_sync_msg(struct mac_addr src) {
    // WORKAROUND: GCC -Wmissing-braces has trouble with this compound literal
    #pragma GCC diagnostic ignored "-Wmissing-braces"
    return (struct ptp_sync_msg){
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
    #pragma GCC diagnostic pop
}

struct eth_ptp_msg create_eth_frame(struct mac_addr src, struct ptp_sync_msg ptp) {
    // WORKAROUND: GCC -Wmissing-braces has trouble with this compound literal
    #pragma GCC diagnostic ignored "-Wmissing-braces"
    return (struct eth_ptp_msg){
        .eth = {
            .dst = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e},
            .src = src,
            .ethertype = htons(ETHERTYPE_PTP),
        },
        .ptp = ptp,
    };
    #pragma GCC diagnostic pop
}

struct mac_addr interface_mac_addr(int fd, const char* ifname) {
    struct ifreq ifreq = {};
    strncpy(ifreq.ifr_name, ifname, strlen(ifname));
    int err = ioctl(fd, SIOCGIFHWADDR, &ifreq);
    assert(!err);
    return *(struct mac_addr*)&ifreq.ifr_hwaddr.sa_data;
}

int main() {
    const char* ifname = "eth1";
    int fd = socket_create(ifname);
    socket_enable_timestamping(fd, ifname);

    struct mac_addr src_addr = interface_mac_addr(fd, ifname);
    struct ptp_sync_msg msg = create_sync_msg(src_addr);
    struct eth_ptp_msg buf = create_eth_frame(src_addr, msg);

    print_buf((uint8_t*)&buf, sizeof(buf));

    send(fd, &buf, sizeof(buf), 0);

    close(fd);
}
