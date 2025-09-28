use std::os::fd::AsRawFd;
use std::os::fd::FromRawFd;

use nix::libc;
use nix::net::if_::if_nametoindex;
use nix::sys::socket::{AddressFamily, LinkAddr, SockFlag, SockType, SockaddrLike, bind, socket};

use socket2::Socket;

pub fn socket_from_ifname(ifname: &str) -> Result<Socket, ()> {
    let fd = socket(
        AddressFamily::Packet,
        SockType::Raw,
        SockFlag::empty(),
        // WORKAROUND: nix doesn't support arbitrary ethertypes so we need to assign the protocol
        // via the LinkAddr::from_raw abstraction below.  We need to do this anyway to bind the
        // socket to an interface by name/index.
        None,
    )
    .map_err(|_| ())?;

    let ifindex = if_nametoindex(ifname).map_err(|_| ())?;
    let link_addr = {
        let sockaddr_ll = libc::sockaddr_ll {
            sll_addr: [0; 8],
            sll_family: libc::AF_PACKET as libc::c_ushort,
            sll_halen: 6,
            sll_hatype: libc::ARPHRD_IEEE802,
            sll_ifindex: ifindex as i32,
            sll_pkttype: 0,
            sll_protocol: (libc::ETH_P_1588 as libc::c_ushort).to_be(),
        };
        let addr = &raw const sockaddr_ll as *const libc::sockaddr;
        let len = std::mem::size_of::<libc::sockaddr_ll>() as libc::socklen_t;
        unsafe { LinkAddr::from_raw(addr, Some(len)).unwrap() }
    };
    bind(fd.as_raw_fd(), &link_addr).map_err(|_| ())?;

    // Convert to a socket2::Socket which provides more Rust-like abstraction
    let socket = unsafe { Socket::from_raw_fd(fd.as_raw_fd()) };

    // Prevent Drop impl from closing the fd
    std::mem::forget(fd);

    Ok(socket)
}
