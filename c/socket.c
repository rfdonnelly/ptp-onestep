#include <arpa/inet.h>
#include <assert.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

int interface_index(int fd, const char* ifname) {
    struct ifreq ifreq = {};
    strncpy(ifreq.ifr_name, ifname, strlen(ifname));
    int err = ioctl(fd, SIOCGIFINDEX, &ifreq);
    assert(!err);
    return ifreq.ifr_ifindex;
}

// Reference: linuxptp:raw.c
int socket_create(const char* ifname) {
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
void socket_enable_timestamping(int fd, const char *ifname) {
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
