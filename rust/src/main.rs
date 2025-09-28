use pnet::datalink::NetworkInterface;
use pnet::packet::ethernet::{EtherTypes, EthernetPacket, MutableEthernetPacket};
use pnet::util::MacAddr;

use onestep::packet::ptp::{ClockIdentity, MutablePtpPacket, PortIdentity, PtpPacket, SyncPacket};
use onestep::socket::socket_from_ifname;

const PKT_ETH_SIZE: usize = EthernetPacket::minimum_packet_size();
const PKT_PTP_HDR_SIZE: usize = PtpPacket::minimum_packet_size();
const PKT_PTP_SYNC_SIZE: usize = SyncPacket::minimum_packet_size();

const PKT_PTP_HDR_OFFSET: usize = PKT_ETH_SIZE;

fn main() -> Result<(), ()> {
    let mut buf = [0u8; PKT_ETH_SIZE + PKT_PTP_HDR_SIZE + PKT_PTP_SYNC_SIZE];
    let interface = get_interface("eth3")?;
    let source_addr = interface.mac.ok_or(())?;
    create_sync_msg(source_addr, &mut buf)?;
    print_buf(&buf);
    let socket = socket_from_ifname("eth3")?;
    socket.send(&buf);

    Ok(())
}

fn print_buf(buf: &[u8]) {
    for (i, b) in buf.iter().enumerate() {
        print!("{:02x} ", b);
        if i % 16 == 15 {
            println!();
        } else if i % 8 == 7 {
            print!(" ");
        }
    }
    println!();
}

fn get_interface(ifname: &str) -> Result<NetworkInterface, ()> {
    let interfaces = pnet::datalink::interfaces();

    interfaces
        .into_iter()
        .find(|iface| iface.name == ifname)
        .ok_or(())
}

fn create_sync_msg(source_addr: MacAddr, buf: &mut [u8]) -> Result<(), ()> {
    {
        let mut eth = MutableEthernetPacket::new(buf).ok_or(())?;
        eth.set_destination(MacAddr::new(0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e));
        eth.set_source(source_addr);
        eth.set_ethertype(EtherTypes::Ptp);
    }
    {
        let mut ptp = MutablePtpPacket::new(&mut buf[PKT_PTP_HDR_OFFSET..]).ok_or(())?;
        ptp.set_majorSdoId(1);
        ptp.set_minorVersionPtp(1);
        ptp.set_versionPtp(2);
        ptp.set_messageLength(44);
        ptp.set_portIdentity(PortIdentity {
            clockIdentity: ClockIdentity([
                source_addr.0,
                source_addr.1,
                source_addr.2,
                0xff,
                0xfe,
                source_addr.3,
                source_addr.4,
                source_addr.5,
            ]),
            portNumber: 1,
        });
        ptp.set_sequenceId(23);
        ptp.set_logMessageInterval((-3i8).cast_unsigned());
    }

    Ok(())
}
