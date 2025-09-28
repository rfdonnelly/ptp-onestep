use std::os::fd::AsRawFd;
use std::os::fd::FromRawFd;

use nix::libc;
use nix::net::if_::if_nametoindex;
use nix::sys::socket::{AddressFamily, LinkAddr, SockFlag, SockType, SockaddrLike, bind, socket};

use socket2::Socket;

pub fn from_ifname(ifname: &str) -> Result<Socket, ()> {
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
            sll_ifindex: ifindex as _,
            sll_family: libc::AF_PACKET as _,
            sll_protocol: (libc::ETH_P_1588 as libc::c_ushort).to_be(),
            // The rest are don't care
            sll_addr: [0; 8],
            sll_halen: 0,
            sll_hatype: 0,
            sll_pkttype: 0,
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

pub fn enable_timestamping(socket: &Socket, ifname: &str) -> Result<(), ()> {
    let mut hwtstamp_config = libc::hwtstamp_config {
        flags: 0,
        tx_type: libc::HWTSTAMP_TX_ONESTEP_SYNC as _,
        rx_filter: libc::HWTSTAMP_FILTER_PTP_V2_L2_EVENT as _,
    };
    let mut ifr_name = [0; 16];
    ifname.bytes().take(16 - 1).enumerate().for_each(|(i, b)| ifr_name[i] = b as _);
    let mut ifreq = libc::ifreq {
        ifr_name: ifr_name,
        ifr_ifru: libc::__c_anonymous_ifr_ifru {
            ifru_data: (&mut hwtstamp_config as *mut _) as *mut libc::c_char,
        },
    };
    unsafe { siocshwtstamp(socket.as_raw_fd(), &mut ifreq) }.map_err(|_| ())?;

    let so_timestamping = SoTimestamping {
        flags: (libc::SOF_TIMESTAMPING_RAW_HARDWARE |
            libc::SOF_TIMESTAMPING_RX_HARDWARE |
            libc::SOF_TIMESTAMPING_TX_HARDWARE) as _,
            bind_phc: 0,
    };
    cerr(unsafe {
        libc::setsockopt(
            socket.as_raw_fd(),
            libc::SOL_SOCKET,
            libc::SO_TIMESTAMPING,
            &so_timestamping as *const _ as *const libc::c_void,
            std::mem::size_of_val(&so_timestamping) as libc::socklen_t,
        )
    }).map_err(|_| ())?;

    Ok(())
}

#[repr(C)]
struct SoTimestamping {
    flags: libc::c_int,
    bind_phc: libc::c_int,
}

nix::ioctl_write_ptr_bad!(siocshwtstamp, libc::SIOCSHWTSTAMP, libc::ifreq);

fn cerr(t: libc::c_int) -> std::io::Result<libc::c_int> {
    match t {
        -1 => Err(std::io::Error::last_os_error()),
        _ => Ok(t),
    }
}
