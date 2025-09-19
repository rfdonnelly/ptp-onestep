#ifndef __SOCKET_H__
#define __SOCKET_H__

// Arguments
//
// * ifname - The name of the network interface (e.g., eth0)
//
// Returns
//
// * The file descriptor of the created socket
int socket_create(const char* ifname);

// Arguments
//
// * fd - The file descriptor of the socket
// * ifname - The name of the network interface (e.g., eth0)
void socket_enable_timestamping(int fd, const char *ifname);

#endif
