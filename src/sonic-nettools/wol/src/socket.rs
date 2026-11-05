use libc;
use std::net::{Ipv4Addr, Ipv6Addr};
use std::{convert::TryInto, io, ptr, mem};
use std::ffi::CString;
use std::os::raw::c_int;
use std::result::Result;
use std::str::FromStr;

use crate::wol::{ WolErr, WolErrCode, vprintln};

const ANY_INTERFACE: &str = "ANY_INTERFACE";
const IPV4_ANY_ADDR : &str = "0.0.0.0";
const IPV6_ANY_ADDR : &str = "::";

type CSocket = c_int;

pub trait WolSocket {
    fn get_socket(&self) -> CSocket;

    fn send_magic_packet(&self, buf : &[u8]) -> Result<usize, WolErr> {
        let res = unsafe {
            libc::send(self.get_socket(), buf.as_ptr() as *const libc::c_void, buf.len(), 0)
        };
        if res < 0 {
            Err(WolErr {
                msg: format!("Failed to send packet, rc={}, error: {}", res, io::Error::last_os_error()),
                code: WolErrCode::InternalError as i32
            })
        } else {
            Ok(res as usize)
        }
    }
}

pub struct RawSocket{
    pub cs: CSocket
}

impl RawSocket {
    pub fn new(intf_name: &str) -> Result<RawSocket, WolErr> {
        vprintln(format!("Creating raw socket for interface: {}", intf_name));
        let res = unsafe {
            libc::socket(libc::AF_PACKET, libc::SOCK_RAW, libc::ETH_P_ALL.to_be())
        };
        if res < 0 {
            return Err(WolErr {
                msg: format!("Failed to create raw socket, rc={}, error: {}", res, io::Error::last_os_error()),
                code: WolErrCode::InternalError as i32
            })
        }
        let _socket = RawSocket{ cs: res };
        _socket.bind_to_intf(intf_name)?;
        Ok(_socket)
    }

    fn bind_to_intf(&self, intf_name: &str) -> Result<(), WolErr> {
        vprintln(format!("Binding raw socket to interface: {}", intf_name));
        let addr_ll: libc::sockaddr_ll = RawSocket::generate_sockaddr_ll(intf_name)?;

        vprintln(format!("Interface index={}, MAC={}", addr_ll.sll_ifindex, addr_ll.sll_addr.iter().map(|x| format!("{:02x}", x)).collect::<Vec<String>>().join(":")));

        let res = unsafe {
            libc::bind(
                self.get_socket(),
                &addr_ll as *const libc::sockaddr_ll as *const libc::sockaddr,
                mem::size_of::<libc::sockaddr_ll>() as u32,
            )
        };

        assert_return_code_is_zero(res, "Failed to bind raw socket to intface", WolErrCode::SocketError)?;

        Ok(())
    }

    fn generate_sockaddr_ll(intf_name: &str) -> Result<libc::sockaddr_ll, WolErr> {
        let mut addr_ll: libc::sockaddr_ll = unsafe { mem::zeroed() };
        addr_ll.sll_family = libc::AF_PACKET as u16;
        addr_ll.sll_protocol = (libc::ETH_P_ALL as u16).to_be();
        addr_ll.sll_halen = 6; // MAC address length in bytes

        let mut addrs: *mut libc::ifaddrs = ptr::null_mut();
        let res = unsafe {
            libc::getifaddrs(&mut addrs)
        };
        assert_return_code_is_zero(res, "Failed on getifaddrs function", WolErrCode::InternalError)?;

        let mut addr = addrs;
        while !addr.is_null() {
            let addr_ref = unsafe { *addr };
            if addr_ref.ifa_name.is_null() {
                addr = addr_ref.ifa_next;
                continue;
            }

            let _in = unsafe {
                std::ffi::CStr::from_ptr(addr_ref.ifa_name).to_str().unwrap()
            };
            if _in == intf_name {
                addr_ll.sll_ifindex = unsafe {
                    libc::if_nametoindex(addr_ref.ifa_name) as i32
                };
                addr_ll.sll_addr = unsafe { (*(addr_ref.ifa_addr as *const libc::sockaddr_ll)).sll_addr };
                break;
            }
            
            addr = addr_ref.ifa_next;
        }

        if addr.is_null() {
            return Err(WolErr {
                msg: format!("Failed to find interface: {}", intf_name),
                code: WolErrCode::InternalError as i32
            });
        }

        unsafe {
            libc::freeifaddrs(addrs);
        }

        Ok(addr_ll)
    }
}

impl WolSocket for RawSocket {
    fn get_socket(&self) -> CSocket { self.cs }
}

#[derive(Debug)]
pub struct UdpSocket{
    pub cs: CSocket
}

impl UdpSocket {
    pub fn new(intf_name: &str, dst_port: u16, ip_addr: &str) -> Result<UdpSocket, WolErr> {
        vprintln(format!("Creating udp socket for interface: {}, destination port: {}, ip address: {}", intf_name, dst_port, ip_addr));
        let res = match ip_addr.contains(':') {
            true => unsafe {libc::socket(libc::AF_INET6, libc::SOCK_DGRAM, libc::IPPROTO_UDP)},
            false => unsafe {libc::socket(libc::AF_INET, libc::SOCK_DGRAM, libc::IPPROTO_UDP)},
        };
        if res < 0 {
            return Err(WolErr {
                msg: format!("Failed to create udp socket, rc={}, error: {}", res, io::Error::last_os_error()),
                code: WolErrCode::SocketError as i32
            })
        }
        let _socket = UdpSocket{ cs: res };
        _socket.enable_broadcast()?;
        _socket.bind_to_intf(intf_name)?;
        _socket.connect_to_addr(dst_port, ip_addr)?;
        Ok(_socket)
    }

    fn enable_broadcast(&self) -> Result<(), WolErr> {
        vprintln(String::from("Enabling broadcast on udp socket"));
        let res = unsafe {
            libc::setsockopt(
                self.get_socket(),
                libc::SOL_SOCKET,
                libc::SO_BROADCAST,
                &1 as *const i32 as *const libc::c_void,
                mem::size_of::<i32>().try_into().unwrap(),
            )
        };
        assert_return_code_is_zero(res, "Failed to enable broadcast on udp socket", WolErrCode::SocketError)?;
        
        Ok(())
    }

    fn bind_to_intf(&self, intf: &str) -> Result<(), WolErr> {
        vprintln(format!("Binding udp socket to interface: {}", intf));
        let c_intf = CString::new(intf).map_err(|_| WolErr {
            msg: String::from("Invalid interface name for binding"),
            code: WolErrCode::SocketError as i32,
        })?;
        let res = unsafe {
            libc::setsockopt(
                self.get_socket(),
                libc::SOL_SOCKET,
                libc::SO_BINDTODEVICE,
                c_intf.as_ptr() as *const libc::c_void,
                c_intf.as_bytes_with_nul().len() as u32,
            )
        };
        assert_return_code_is_zero(res, "Failed to bind udp socket to interface", WolErrCode::SocketError)?;

        Ok(())
    }

    fn connect_to_addr(&self, port: u16, ip_addr: &str) -> Result<(), WolErr> {
        vprintln(format!("Setting udp socket destination as address: {}, port: {}", ip_addr, port));
        let (addr, addr_len) = match ip_addr.contains(':') {
            true => (
                &ipv6_addr(port, ip_addr, ANY_INTERFACE)? as *const libc::sockaddr_in6 as *const libc::sockaddr,
                mem::size_of::<libc::sockaddr_in6>() as u32
            ),
            false => (
                &ipv4_addr(port, ip_addr)? as *const libc::sockaddr_in as *const libc::sockaddr,
                mem::size_of::<libc::sockaddr_in>() as u32
            ),
        };
        let res = unsafe {
            libc::connect(
                self.get_socket(),
                addr,
                addr_len
            )
        };
        assert_return_code_is_zero(res,"Failed to connect udp socket to address", WolErrCode::SocketError)?;

        Ok(())
    }
}

impl WolSocket for UdpSocket {
    fn get_socket(&self) -> CSocket { self.cs }
}

fn ipv4_addr(port: u16, addr: &str) -> Result<libc::sockaddr_in, WolErr> {
    let _addr = match addr == IPV4_ANY_ADDR {
        true => libc::in_addr { s_addr: libc::INADDR_ANY },
        false => libc::in_addr { s_addr: u32::from(Ipv4Addr::from_str(addr).map_err(|e| 
                WolErr{
                    msg: format!("Failed to parse ipv4 address: {}", e),
                    code: WolErrCode::SocketError as i32
                }
            )?).to_be()
        },
    };
    Ok(
        libc::sockaddr_in {
            sin_family: libc::AF_INET as u16,
            sin_port: port.to_be(),
            sin_addr: _addr,
            sin_zero: [0; 8],
        }
    )
}

fn ipv6_addr(port: u16, addr: &str, intf_name: &str) -> Result<libc::sockaddr_in6, WolErr> {
    let _addr = match addr == IPV6_ANY_ADDR {
        true => libc::IN6ADDR_ANY_INIT,
        false => libc::in6_addr { s6_addr: Ipv6Addr::from_str(addr).map_err(|e| 
                WolErr{
                    msg: format!("Failed to parse ipv6 address: {}", e),
                    code: WolErrCode::SocketError as i32
                }
            )?.octets()
        },
    };
    let _scope_id= match intf_name == ANY_INTERFACE {
        true => 0,
        false => unsafe { libc::if_nametoindex(CString::new(intf_name).map_err(|_| 
                WolErr{
                    msg: String::from("Invalid interface name for binding"),
                    code: WolErrCode::SocketError as i32
                }
            )?.as_ptr()) as u32
        }
    };
    Ok(
        libc::sockaddr_in6 {
            sin6_family: libc::AF_INET6 as u16,
            sin6_port: port.to_be(),
            sin6_flowinfo: 0,
            sin6_addr: _addr,
            sin6_scope_id: _scope_id.to_be(),
        }
    )
}

fn assert_return_code_is_zero(rc: i32, msg: &str, err_code: WolErrCode) -> Result<(), WolErr> {
    if rc != 0 {
        Err(WolErr {
            msg: format!("{}, rc={},error: {}", msg, rc, io::Error::last_os_error()),
            code: err_code as i32,
        })
    } else {
        Ok(())
    }
}


#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_ipv4_addr() {
        let port = 1234;
        let addr = ipv4_addr(port, IPV4_ANY_ADDR).unwrap();
        assert_eq!(addr.sin_family , libc::AF_INET as u16);
        assert_eq!(addr.sin_port.to_le() , port.to_be());
        assert_eq!(addr.sin_addr.s_addr , libc::INADDR_ANY);
        assert_eq!(addr.sin_zero , [0; 8]);

        let ip = "1.1.1.1";
        let addr = ipv4_addr(port, &ip).unwrap();
        assert_eq!(addr.sin_family , libc::AF_INET as u16);
        assert_eq!(addr.sin_port.to_le() , port.to_be());
        assert_eq!(addr.sin_addr.s_addr , u32::from(Ipv4Addr::from_str(ip).unwrap()).to_be());
        assert_eq!(addr.sin_zero , [0; 8]);
    }

    #[test]
    fn test_ipv6_addr() {
        let port = 1234;
        let addr = ipv6_addr(port, IPV6_ANY_ADDR, ANY_INTERFACE).unwrap();
        assert_eq!(addr.sin6_family , libc::AF_INET6 as u16);
        assert_eq!(addr.sin6_port.to_le() , port.to_be());
        assert_eq!(addr.sin6_flowinfo , 0);
        assert_eq!(addr.sin6_addr.s6_addr , libc::IN6ADDR_ANY_INIT.s6_addr);
        assert_eq!(addr.sin6_scope_id , 0);

        let ip = "2001:db8::1";
        let addr = ipv6_addr(port, &ip, ANY_INTERFACE).unwrap();
        assert_eq!(addr.sin6_family , libc::AF_INET6 as u16);
        assert_eq!(addr.sin6_port.to_le() , port.to_be());
        assert_eq!(addr.sin6_flowinfo , 0);
        assert_eq!(addr.sin6_addr.s6_addr , Ipv6Addr::from_str(ip).unwrap().octets());
        assert_eq!(addr.sin6_scope_id , 0);
    }

    #[test]
    fn test_assert_return_code_is_zero() {
        let rc = 0;
        assert_eq!(assert_return_code_is_zero(rc, "", WolErrCode::InternalError).is_ok(), true);

        let rc = -1;
        let msg = "test";
        let err_code = WolErrCode::InternalError;
        let result = assert_return_code_is_zero(rc, msg, err_code);
        assert_eq!(result.is_err(), true);
        assert_eq!(result.as_ref().unwrap_err().code, WolErrCode::InternalError as i32);
        assert_eq!(result.unwrap_err().msg, format!("{}, rc=-1,error: {}", msg, io::Error::last_os_error()));
    }
}