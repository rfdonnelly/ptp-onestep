use pnet_macros::packet;
use pnet_macros_support::types::*;
use pnet_packet::PrimitiveValues;

#[allow(non_snake_case)]
#[packet]
pub struct Ptp {
    pub majorSdoId: u4,
    pub messageType: u4,
    pub minorVersionPtp: u4,
    pub versionPtp: u4,
    pub messageLength: u16be,
    pub domainNumber: u8,
    pub minorSdoId: u8,
    pub flags: u16be,
    pub correctionField: u64be,
    pub messageTypeSpecific: u32be,
    #[construct_with(u8, u8, u8, u8, u8, u8, u8, u8, u16)]
    pub portIdentity: PortIdentity,
    pub sequenceId: u16be,
    pub controlField: u8,
    // #[construct_with(u8)]
    pub logMessageInterval: u8,
    #[payload]
    pub payload: Vec<u8>,
}

#[allow(non_snake_case)]
#[packet]
pub struct Sync {
    #[construct_with(u16, u32, u32)]
    pub originTimestamp: Timestamp,
    #[payload]
    pub payload: Vec<u8>,
}

#[allow(non_snake_case)]
#[derive(PartialEq, Eq, PartialOrd, Ord, Clone, Copy, Debug, Hash)]
pub struct PortIdentity {
    pub clockIdentity: ClockIdentity,
    pub portNumber: u16be,
}

impl PortIdentity {
    pub fn new(a: u8, b: u8, c: u8, d: u8, e: u8, f: u8, g: u8, h: u8, i: u16) -> Self {
        Self {
            clockIdentity: ClockIdentity::new(a, b, c, d, e, f, g, h),
            portNumber: i,
        }
    }
}

impl PrimitiveValues for PortIdentity {
    type T = (u8, u8, u8, u8, u8, u8, u8, u8, u16);

    fn to_primitive_values(&self) -> Self::T {
        let a = self.clockIdentity;
        (
            a.0[0],
            a.0[1],
            a.0[2],
            a.0[3],
            a.0[4],
            a.0[5],
            a.0[6],
            a.0[7],
            self.portNumber,
        )
    }
}

#[derive(PartialEq, Eq, PartialOrd, Ord, Clone, Copy, Debug, Hash)]
pub struct ClockIdentity(pub [u8; 8]);

impl PrimitiveValues for ClockIdentity {
    type T = (u8, u8, u8, u8, u8, u8, u8, u8);

    fn to_primitive_values(&self) -> Self::T {
        (
            self.0[0], self.0[1], self.0[2], self.0[3], self.0[4], self.0[5], self.0[6], self.0[7],
        )
    }
}

impl ClockIdentity {
    pub fn new(a: u8, b: u8, c: u8, d: u8, e: u8, f: u8, g: u8, h: u8) -> Self {
        Self([a, b, c, d, e, f, g, h])
    }
}

#[derive(PartialEq, Eq, PartialOrd, Ord, Clone, Copy, Debug, Hash)]
pub struct Timestamp {
    pub seconds_msb: u16,
    pub seconds_lsb: u32,
    pub nanoseconds: u32,
}

impl Timestamp {
    pub fn new(a: u16, b: u32, c: u32) -> Self {
        Self {
            seconds_msb: a,
            seconds_lsb: b,
            nanoseconds: c,
        }
    }
}

impl PrimitiveValues for Timestamp {
    type T = (u16, u32, u32);

    fn to_primitive_values(&self) -> Self::T {
        (self.seconds_msb, self.seconds_lsb, self.nanoseconds)
    }
}
