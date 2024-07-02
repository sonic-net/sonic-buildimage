use clap::Parser;
use pnet::datalink::Channel::Ethernet;
use pnet::datalink::{self, DataLinkSender, MacAddr, NetworkInterface};
use std::result::Result;
use std::str::FromStr;

const BROADCAST_MAC: [u8; 6] = [0xff, 0xff, 0xff, 0xff, 0xff, 0xff];

pub fn build_and_send() -> Result<(), String> {
    let args = WolArgs::parse();
    let target_macs = parse_target_macs(&args)?;
    if !is_operstate_up(&args.interface)? {
        return Err("Error: The target interface is not up".into());
    }
    let src_mac = get_interface_mac(&args.interface)?;
    let mut tx = open_tx_channel(&args.interface)?;

    for dst_mac in target_macs {
        println!(
            "Building and sending packet to mac address {}",
            dst_mac
                .iter()
                .map(|b| format!("{:02X}", b))
                .collect::<Vec<String>>()
                .join(":")
        );
        let magic_bytes = build_magic_packet(&src_mac, &dst_mac, &args.password)?;
        send_magic_packet(
            &mut tx,
            magic_bytes,
            &args.count,
            &args.interval,
            &args.verbose,
        )?;
    }

    Ok(())
}

fn parse_mac_addr(mac_str: &str) -> Result<[u8; 6], String> {
    MacAddr::from_str(mac_str)
        .map(|mac| mac.octets())
        .map_err(|_| "Invalid MAC address".into())
}

fn parse_ipv4_addr(ipv4_str: &str) -> Result<Vec<u8>, String> {
    if !is_ipv4_address_valid(ipv4_str) {
        Err("Invalid IPv4 address".into())
    } else {
        ipv4_str
            .split('.')
            .map(|octet| octet.parse::<u8>())
            .collect::<Result<Vec<u8>, _>>()
            .map_err(|_| "Invalid IPv4 address".into())
    }
}

fn parse_password(password: &str) -> Result<Password, String> {
    if is_ipv4_address_valid(password) {
        Ok(Password(parse_ipv4_addr(password)?))
    } else if is_mac_string_valid(password) {
        parse_mac_addr(password).map(|mac| Password(mac.to_vec()))
    } else {
        Err("Invalid password".to_string())
    }
}

fn parse_target_macs(args: &WolArgs) -> Result<Vec<[u8; 6]>, String> {
    if args.broadcast && args.target_mac.is_some() {
        return Err(String::from(
            "Error: Cannot specify both --broadcast and --target-mac",
        ));
    }
    if !args.broadcast && args.target_mac.is_none() {
        return Err(String::from(
            "Error: Must specify either --broadcast or --target-mac",
        ));
    }

    if args.broadcast {
        Ok(vec![BROADCAST_MAC])
    } else {
        let target_macs: Vec<&str> = args.target_mac.as_ref().unwrap().split(',').collect();
        let mut macs = Vec::new();
        for mac_str in target_macs {
            macs.push(parse_mac_addr(mac_str)?);
        }
        Ok(macs)
    }
}

fn is_operstate_up(interface: &str) -> Result<bool, String> {
    let state_file_path = format!("/sys/class/net/{}/operstate", interface);
    match std::fs::read_to_string(state_file_path) {
        Ok(content) => Ok(content.trim() == "up"),
        Err(_) => Err(format!(
            "Could not read operstate file for interface {}",
            interface
        )),
    }
}

fn is_mac_string_valid(mac_str: &str) -> bool {
    let mac_str = mac_str.replace(':', "");
    mac_str.len() == 12 && mac_str.chars().all(|c| c.is_ascii_hexdigit())
}

fn is_ipv4_address_valid(ipv4_str: &str) -> bool {
    ipv4_str.split('.').count() == 4
        && ipv4_str
            .split('.')
            .all(|octet| octet.parse::<u64>().map_or(false, |n| n < 256))
}

fn get_interface_mac(interface_name: &String) -> Result<[u8; 6], String> {
    if let Some(interface) = datalink::interfaces()
        .into_iter()
        .find(|iface: &NetworkInterface| iface.name == *interface_name)
    {
        if let Some(mac) = interface.mac {
            Ok(mac.octets())
        } else {
            Err("Could not get MAC address of target interface".into())
        }
    } else {
        Err("Could not find target interface".into())
    }
}

fn build_magic_packet(
    src_mac: &[u8; 6],
    dst_mac: &[u8; 6],
    password: &Option<Password>,
) -> Result<Vec<u8>, String> {
    let password_len = password.as_ref().map_or(0, |p| p.ref_bytes().len());
    let mut pkt = vec![0u8; 116 + password_len];
    pkt[0..6].copy_from_slice(dst_mac);
    pkt[6..12].copy_from_slice(src_mac);
    pkt[12..14].copy_from_slice(&[0x08, 0x42]);
    pkt[14..20].copy_from_slice(&[0xff; 6]);
    pkt[20..116].copy_from_slice(&dst_mac.repeat(16));
    if let Some(p) = password {
        pkt[116..116 + password_len].copy_from_slice(p.ref_bytes());
    }
    Ok(pkt)
}

fn send_magic_packet(
    tx: &mut Box<dyn DataLinkSender>,
    packet: Vec<u8>,
    count: &u8,
    interval: &u64,
    verbose: &bool,
) -> Result<(), String> {
    for nth in 0..*count {
        // if let Err(e) = tx.send_to(&packet, None).unwrap() {
        //     eprintln!("Failed to send magic packet: {}", e);
        //     break;
        // }
        match tx.send_to(&packet, None) {
            Some(Ok(_)) => {}
            Some(Err(e)) => {
                return Err(format!("Failed to send magic packet: {}", e));
            }
            None => {
                return Err("Not sure if packet was sent".into());
            }
        }
        if *verbose {
            println!(
                "  | -> Sent the {}th packet and sleep for {} seconds",
                &nth + 1,
                &interval
            );
            println!(
                "    | -> Packet bytes in hex {}",
                &packet
                    .iter()
                    .fold(String::new(), |acc, b| acc + &format!("{:02X}", b))
            )
        }
        std::thread::sleep(std::time::Duration::from_millis(*interval));
    }
    Ok(())
}

fn open_tx_channel(interface: &str) -> Result<Box<dyn DataLinkSender>, String> {
    if let Some(interface) = datalink::interfaces()
        .into_iter()
        .find(|iface: &NetworkInterface| iface.name == interface)
    {
        match datalink::channel(&interface, Default::default()) {
            Ok(Ethernet(tx, _)) => Ok(tx),
            Ok(_) => Err("Unhandled channel type".into()),
            Err(e) => Err(format!(
                "An error occurred when creating the datalink channel: {}",
                e
            )),
        }
    } else {
        Err("Could not find target interface".into())
    }
}

#[derive(Parser, Debug)]
#[command(
    next_line_help = true,
    about = "
This tool can generate and send wake on LAN magic packets with target interface and mac

Examples:
    wol Ethernet10 00:11:22:33:44:55
    wol Ethernet10 00:11:22:33:44:55 -b
    wol Vlan1000 00:11:22:33:44:55,11:33:55:77:99:bb -p 00:22:44:66:88:aa
    wol Vlan1000 00:11:22:33:44:55,11:33:55:77:99:bb -p 192.168.1.1 -c 3 -i 2000"
)]
struct WolArgs {
    /// The name of the network interface to send the magic packet through
    #[arg(short, long)]
    interface: String,

    /// The MAC address of the target device, formatted as a colon-separated string (e.g. "00:11:22:33:44:55")
    #[arg(short = 'm', long)]
    target_mac: Option<String>,

    /// The flag to indicate if we use the broadcast MAC address ff:ff:ff:ff:ff:ff as the target MAC address [default: false]
    #[arg(short, long, default_value_t = false)]
    broadcast: bool,

    /// An optional 4 or 6 byte password, in ethernet hex format or quad-dotted decimal (e.g. "127.0.0.1" or "00:11:22:33:44:55")
    #[arg(short, long, value_parser = parse_password)]
    password: Option<Password>,

    /// The number of times to send the magic packet [default: 1][range: 1-5]
    #[arg(short, long, default_value_t = 1, value_parser = clap::value_parser!(u8).range(1..6))]
    count: u8,

    /// The interval in milliseconds between each magic packet transmission [default: 0][range: 0-2000]
    #[arg(short = 't', long, default_value_t = 0, value_parser = clap::value_parser!(u64).range(0..2001))]
    interval: u64,

    /// The flag to indicate if we should print verbose output
    #[arg(short, long)]
    verbose: bool,
}

#[derive(Debug, Clone)]
struct Password(Vec<u8>);

impl Password {
    fn ref_bytes(&self) -> &Vec<u8> {
        &self.0
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use clap::CommandFactory;

    #[test]
    fn test_parse_mac_addr() {
        let mac_str = "00:11:22:33:44:55";
        let mac = parse_mac_addr(mac_str).unwrap();
        assert_eq!(mac, [0x00, 0x11, 0x22, 0x33, 0x44, 0x55]);

        let mac_str = "00:11:22:33:44:GG";
        assert!(parse_mac_addr(mac_str).is_err());
        assert_eq!(parse_mac_addr(mac_str).unwrap_err(), "Invalid MAC address");

        let mac_str = "00-01-22-33-44-55";
        assert!(parse_mac_addr(mac_str).is_err());
        assert_eq!(parse_mac_addr(mac_str).unwrap_err(), "Invalid MAC address");
    }

    #[test]
    fn test_parse_ipv4_addr() {
        let ipv4_str = "127.0.0.1";
        let ipv4 = parse_ipv4_addr(ipv4_str).unwrap();
        assert_eq!(ipv4, [127, 0, 0, 1]);

        let ipv4_str = "127.0.0.256";
        assert!(parse_ipv4_addr(ipv4_str).is_err());
        assert_eq!(
            parse_ipv4_addr(ipv4_str).unwrap_err(),
            "Invalid IPv4 address"
        );

        let ipv4_str = "127.0.0";
        assert!(parse_ipv4_addr(ipv4_str).is_err());
        assert_eq!(
            parse_ipv4_addr(ipv4_str).unwrap_err(),
            "Invalid IPv4 address"
        );

        let ipv4_str = "::1";
        assert!(parse_ipv4_addr(ipv4_str).is_err());
        assert_eq!(
            parse_ipv4_addr(ipv4_str).unwrap_err(),
            "Invalid IPv4 address"
        );
    }

    #[test]
    fn test_parse_password() {
        let password_str = "127.0.0.1";
        let password = parse_password(password_str);
        assert_eq!(*password.unwrap().ref_bytes(), [127, 0, 0, 1]);

        let password_str = "00:11:22:33:44:55";
        let password = parse_password(password_str);
        assert_eq!(*password.unwrap().ref_bytes(), [0, 17, 34, 51, 68, 85]);

        let password_str = "127.0.0.256";
        assert!(parse_password(password_str).is_err());

        let password_str = "127.0.0";
        assert!(parse_password(password_str).is_err());

        let password_str = "::1";
        assert!(parse_password(password_str).is_err());

        let password_str = "00:11:22:33:44:GG";
        assert!(parse_password(password_str).is_err());

        let password_str = "00-01-22-33-44-55";
        assert!(parse_password(password_str).is_err());
    }

    #[test]
    fn test_parse_target_macs() {
        let mut args = WolArgs {
            interface: "Ethernet10".to_string(),
            target_mac: Some("00:11:22:33:44:55".to_string()),
            broadcast: false,
            password: None,
            count: 1,
            interval: 0,
            verbose: false,
        };
        let target_macs = parse_target_macs(&args).unwrap();
        assert_eq!(target_macs.len(), 1);
        assert_eq!(target_macs[0], [0x00, 0x11, 0x22, 0x33, 0x44, 0x55]);

        args.target_mac = Some("00:11:22:33:44:55,11:22:33:44:55:66,22:33:44:55:66:77".to_string());
        let target_macs = parse_target_macs(&args).unwrap();
        assert_eq!(target_macs.len(), 3);
        assert_eq!(target_macs[0], [0x00, 0x11, 0x22, 0x33, 0x44, 0x55]);
        assert_eq!(target_macs[1], [0x11, 0x22, 0x33, 0x44, 0x55, 0x66]);
        assert_eq!(target_macs[2], [0x22, 0x33, 0x44, 0x55, 0x66, 0x77]);

        args.broadcast = true;
        args.target_mac = None;
        let target_macs = parse_target_macs(&args).unwrap();
        assert_eq!(target_macs.len(), 1);
        assert_eq!(target_macs[0], BROADCAST_MAC);

        args.broadcast = true;
        args.target_mac = Some("00:11:22:33:44:55".to_string());
        assert!(parse_target_macs(&args).is_err());
        assert_eq!(
            parse_target_macs(&args).unwrap_err(),
            "Error: Cannot specify both --broadcast and --target-mac"
        );

        args.broadcast = false;
        args.target_mac = None;
        assert!(parse_target_macs(&args).is_err());
        assert_eq!(
            parse_target_macs(&args).unwrap_err(),
            "Error: Must specify either --broadcast or --target-mac"
        );

        args.broadcast = false;
        args.target_mac = Some("00:01".to_string());
        assert!(parse_target_macs(&args).is_err());
        assert_eq!(parse_target_macs(&args).unwrap_err(), "Invalid MAC address");
    }

    #[test]
    fn test_is_operstate_up() {
        // assert!(is_operstate_up("lo").is_ok());
        // assert!(is_operstate_up("eth0").is_err());
    }

    #[test]
    fn test_is_mac_string_valid() {
        assert!(is_mac_string_valid("00:11:22:33:44:55"));
        assert!(!is_mac_string_valid(""));
        assert!(!is_mac_string_valid("0:1:2:3:4:G"));
        assert!(!is_mac_string_valid("00:11:22:33:44:GG"));
        assert!(!is_mac_string_valid("00-11-22-33-44-55"));
        assert!(!is_mac_string_valid("00:11:22:33:44:55:66"));
    }

    #[test]
    fn test_is_ipv4_address_valid() {
        assert!(is_ipv4_address_valid("192.168.1.1"));
        assert!(!is_ipv4_address_valid(""));
        assert!(!is_ipv4_address_valid("0::1"));
        assert!(!is_ipv4_address_valid("192.168.1"));
        assert!(!is_ipv4_address_valid("192.168.1.256"));
        assert!(!is_ipv4_address_valid("192.168.1.1.1"));
    }

    #[test]
    fn test_get_interface_mac() {
        // assert!(get_interface_mac("lo").is_ok());
        // assert!(get_interface_mac("eth0").is_err());
    }

    #[test]
    fn test_build_magic_packet() {
        let src_mac = [0x00, 0x11, 0x22, 0x33, 0x44, 0x55];
        let dst_mac = [0xff, 0xff, 0xff, 0xff, 0xff, 0xff];
        let four_bytes_password = Some(Password(vec![0x00, 0x11, 0x22, 0x33]));
        let magic_packet = build_magic_packet(&src_mac, &dst_mac, &four_bytes_password).unwrap();
        assert_eq!(magic_packet.len(), 120);
        assert_eq!(&magic_packet[0..6], &dst_mac);
        assert_eq!(&magic_packet[6..12], &src_mac);
        assert_eq!(&magic_packet[12..14], &[0x08, 0x42]);
        assert_eq!(&magic_packet[14..20], &[0xff; 6]);
        assert_eq!(&magic_packet[20..116], dst_mac.repeat(16));
        assert_eq!(&magic_packet[116..120], &[0x00, 0x11, 0x22, 0x33]);
        let six_bytes_password = Some(Password(vec![0x00, 0x11, 0x22, 0x33, 0x44, 0x55]));
        let magic_packet = build_magic_packet(&src_mac, &dst_mac, &six_bytes_password).unwrap();
        assert_eq!(magic_packet.len(), 122);
        assert_eq!(&magic_packet[0..6], &dst_mac);
        assert_eq!(&magic_packet[6..12], &src_mac);
        assert_eq!(&magic_packet[12..14], &[0x08, 0x42]);
        assert_eq!(&magic_packet[14..20], &[0xff; 6]);
        assert_eq!(&magic_packet[20..116], dst_mac.repeat(16));
        assert_eq!(
            &magic_packet[116..122],
            &[0x00, 0x11, 0x22, 0x33, 0x44, 0x55]
        );
    }

    #[test]
    fn test_build_magic_packet_without_password() {
        let src_mac = [0x00, 0x11, 0x22, 0x33, 0x44, 0x55];
        let dst_mac = [0xff, 0xff, 0xff, 0xff, 0xff, 0xff];
        let magic_packet = build_magic_packet(&src_mac, &dst_mac, &None).unwrap();
        assert_eq!(magic_packet.len(), 116);
        assert_eq!(&magic_packet[0..6], &dst_mac);
        assert_eq!(&magic_packet[6..12], &src_mac);
        assert_eq!(&magic_packet[12..14], &[0x08, 0x42]);
        assert_eq!(&magic_packet[14..20], &[0xff; 6]);
        assert_eq!(&magic_packet[20..116], dst_mac.repeat(16));
    }

    #[test]
    fn test_send_magic_packet() {
    }

    #[test]
    fn test_open_tx_channel() {
        // assert!(open_tx_channel("lo").is_ok());
        // assert!(open_tx_channel("eth0").is_err());
    }

    #[test]
    fn verify_cli() {
        WolArgs::command().debug_assert();
    }

    #[test]
    fn verify_cli_with_password() {
        WolArgs::parse_from(vec![
            "wol",
            "-i",
            "Ethernet10",
            "-m",
            "00:11:22:33:44:55",
            "-p",
            "1.1.1.1",
        ]);
    }
}
