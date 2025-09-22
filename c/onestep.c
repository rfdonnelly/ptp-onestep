#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
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
            .messageLength = 44,
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
                .portNumber = 1,
            },
            .sequenceId = 23,
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

int main_tx(const char* ifname) {
    int fd = socket_create(ifname);
    socket_enable_timestamping(fd, ifname);

    struct mac_addr src_addr = interface_mac_addr(fd, ifname);
    struct ptp_sync_msg msg = create_sync_msg(src_addr);
    print_ptp_sync_msg(&msg);
    hton_ptp_sync_msg(&msg);
    struct eth_ptp_msg buf = create_eth_frame(src_addr, msg);

    // print_buf((uint8_t*)&buf, sizeof(buf));

    send(fd, &buf, sizeof(buf), 0);

    close(fd);

    return 0;
}

int rx(int fd, void* buf, int buflen, struct timespec* timestamp) {
    char msg_control[256] = { 0 };
    struct iovec msg_iov = {
        .iov_base = buf,
        .iov_len = buflen,
    };
    struct msghdr msg = {
        .msg_iov = &msg_iov,
        .msg_iovlen = 1,
        .msg_control = msg_control,
        .msg_controllen = sizeof(msg_control),
    };

    int cnt = recvmsg(fd, &msg, 0);
    assert(cnt >= 0);

    struct timespec* timespecs = NULL;
    for (struct cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm != NULL; cm = CMSG_NXTHDR(&msg, cm)) {
        if (SOL_SOCKET == cm->cmsg_level && SO_TIMESTAMPING == cm->cmsg_type) {
            if (cm->cmsg_len < sizeof(struct timespec) * 3) {
                fprintf(stderr, "warning: short SO_TIMESTAMPING message\n");
                return -EMSGSIZE;
            }

            timespecs = (struct timespec*)CMSG_DATA(cm);
        } else {
            fprintf(stderr, "warning: unexpected message type/level\n");
        }
    }

    if (timespecs) {
        *timestamp = timespecs[2];
    }

    if (cnt < 0) {
        return -errno;
    } else {
        return cnt;
    }
}

int main_rx(const char* ifname) {
    int fd = socket_create(ifname);
    socket_enable_timestamping(fd, ifname);

    char buf[1500] = { 0 };

    struct timespec timestamp = { 0 };
    int cnt = rx(fd, &buf, sizeof(buf), &timestamp);
    if (cnt < 0) {
        errno = cnt;
        perror("error: receive failed");
        return 1;
    }

    // print_buf((uint8_t*)&buf, cnt);
    struct eth_ptp_msg* eth_frame = (struct eth_ptp_msg*)buf;
    ntoh_ptp_sync_msg(&eth_frame->ptp);
    print_ptp_sync_msg(&eth_frame->ptp);
    printf("rx_timestamp: sec: %ld nsec: %ld\n", timestamp.tv_sec, timestamp.tv_nsec);

    return cnt < 0;
}

void print_usage(const char* argv0) {
    printf("usage: %s <tx|rx> <interface>\n", argv0);
}

int main(int argc, char *argv[]) {
    const char* cmd = argv[1];
    const char* ifname = argv[2];

    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    if (!strcmp(cmd, "tx")) {
        return main_tx(ifname);
    } else if (!strcmp(cmd, "rx")) {
        return main_rx(ifname);
    } else {
        print_usage(argv[0]);
        return 1;
    }
}
