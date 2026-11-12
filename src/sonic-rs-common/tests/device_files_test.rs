use std::fs;
use std::path::{Path, PathBuf};

use sonic_rs_common::multi_asic::asic_presence_list_with;
use sonic_rs_common::DeviceInfo;

const PLATFORM: &str = "x86_64-mlnx_msn2700-r0";

fn device_root(machine_conf: &str) -> (tempfile::TempDir, PathBuf) {
    let dir = tempfile::tempdir().unwrap();
    fs::create_dir_all(dir.path().join("host")).unwrap();
    fs::write(dir.path().join("host/machine.conf"), machine_conf).unwrap();
    let platform_dir = dir.path().join("usr/share/sonic/device").join(PLATFORM);
    fs::create_dir_all(&platform_dir).unwrap();
    (dir, platform_dir)
}

fn write(path: &Path, content: &str) {
    fs::write(path, content).unwrap();
}

#[test]
fn platform_from_machine_conf() {
    let (dir, platform_dir) =
        device_root(&format!("onie_version=2019\nonie_platform={PLATFORM}\n"));
    let info = DeviceInfo::with_root(dir.path());
    assert_eq!(info.platform().as_deref(), Some(PLATFORM));
    assert_eq!(info.platform_dir(), Some(platform_dir));
    assert_eq!(info.machine_info().unwrap()["onie_version"], "2019");

    let (dir, _) = device_root("aboot_platform=x86_64-arista_7050_qx32\n");
    assert_eq!(
        DeviceInfo::with_root(dir.path()).platform().as_deref(),
        Some("x86_64-arista_7050_qx32")
    );

    let dir = tempfile::tempdir().unwrap();
    assert_eq!(DeviceInfo::with_root(dir.path()).platform(), None);
}

#[test]
fn single_asic_without_conf() {
    let (dir, _) = device_root(&format!("onie_platform={PLATFORM}\n"));
    let info = DeviceInfo::with_root(dir.path());
    assert_eq!(info.num_asics(), 1);
    assert!(!info.is_multi_asic());
    assert!(!info.is_supervisor());
    assert!(!info.is_disaggregated_chassis());
    assert!(!info.is_macsec_supported());
    assert!(info.is_chassis_config_absent());
    assert_eq!(
        asic_presence_list_with(&info, || panic!("no DB on a single ASIC")),
        vec![0]
    );
}

#[test]
fn multi_asic_line_card() {
    let (dir, platform_dir) = device_root(&format!("onie_platform={PLATFORM}\n"));
    write(
        &platform_dir.join("asic.conf"),
        "NUM_ASIC=4\nDEV_ID_ASIC_0=03:00.0\n",
    );
    let info = DeviceInfo::with_root(dir.path());
    assert_eq!(info.num_asics(), 4);
    assert!(info.is_multi_asic());
    assert_eq!(
        asic_presence_list_with(&info, || panic!("line cards do not read the DB")),
        vec![0, 1, 2, 3]
    );
}

#[test]
fn multi_asic_supervisor() {
    let (dir, platform_dir) = device_root(&format!("onie_platform={PLATFORM}\n"));
    write(&platform_dir.join("asic.conf"), "num_asic=12\n");
    write(
        &platform_dir.join("platform_env.conf"),
        "SUPERVISOR=1\ndisaggregated_chassis=0\nmacsec_enabled=1\n",
    );
    let info = DeviceInfo::with_root(dir.path());
    assert!(info.is_supervisor());
    assert!(info.is_macsec_supported());
    assert!(!info.is_disaggregated_chassis());
    let fabric = || {
        vec![
            "asic0".to_string(),
            "asic5".to_string(),
            "bogus".to_string(),
        ]
    };
    assert_eq!(asic_presence_list_with(&info, fabric), vec![0, 5]);

    write(
        &platform_dir.join("platform_expected_asic_list.conf"),
        "[0, 1, 4]\n",
    );
    assert_eq!(
        asic_presence_list_with(&info, || panic!("expected list wins")),
        vec![0, 1, 4]
    );
    write(
        &platform_dir.join("platform_expected_asic_list.conf"),
        "- 2\n- 3 # comment\n",
    );
    assert_eq!(info.expected_asic_list(), vec![2, 3]);
    write(
        &platform_dir.join("platform_expected_asic_list.conf"),
        "['0', 4]\n",
    );
    assert_eq!(info.expected_asic_list(), vec![0, 4]);
}

#[test]
fn the_last_num_asic_counts() {
    let (dir, platform_dir) = device_root(&format!("onie_platform={PLATFORM}\n"));
    write(
        &platform_dir.join("asic.conf"),
        "NUM_ASIC=2\nDEV_ID_ASIC_0=03:00.0\nnum_asic=4\n",
    );
    assert_eq!(DeviceInfo::with_root(dir.path()).num_asics(), 4);
}

#[test]
fn disaggregated_chassis_key_is_case_sensitive() {
    let (dir, platform_dir) = device_root(&format!("onie_platform={PLATFORM}\n"));
    write(
        &platform_dir.join("platform_env.conf"),
        "DISAGGREGATED_CHASSIS=1\n",
    );
    assert!(!DeviceInfo::with_root(dir.path()).is_disaggregated_chassis());
    write(
        &platform_dir.join("platform_env.conf"),
        "disaggregated_chassis=1\n",
    );
    assert!(DeviceInfo::with_root(dir.path()).is_disaggregated_chassis());
}

#[test]
fn container_platform_dir_wins() {
    let (dir, platform_dir) = device_root(&format!("onie_platform={PLATFORM}\n"));
    write(&platform_dir.join("asic.conf"), "NUM_ASIC=2\n");
    let container = dir.path().join("usr/share/sonic/platform");
    fs::create_dir_all(&container).unwrap();
    write(&container.join("asic.conf"), "NUM_ASIC=6\n");
    let info = DeviceInfo::with_root(dir.path());
    assert_eq!(info.num_asics(), 6);
    assert_eq!(info.platform_dir(), Some(container));
}

#[test]
fn asic_type_from_sonic_version() {
    let (dir, _) = device_root(&format!("onie_platform={PLATFORM}\n"));
    fs::create_dir_all(dir.path().join("etc/sonic")).unwrap();
    write(
        &dir.path().join("etc/sonic/sonic_version.yml"),
        "build_version: 'master.1'\nasic_type: mellanox\ncommit_id: 'abc'\n",
    );
    assert_eq!(
        DeviceInfo::with_root(dir.path()).asic_type().as_deref(),
        Some("mellanox")
    );
}

#[test]
fn port_config_file() {
    let (dir, platform_dir) = device_root(&format!("onie_platform={PLATFORM}\n"));
    let info = DeviceInfo::with_root(dir.path());
    let hwsku = platform_dir.join("SKU");
    fs::create_dir_all(hwsku.join("0")).unwrap();
    assert_eq!(info.port_config_file("SKU", None), None);

    write(&hwsku.join("port_config.ini"), "# name lanes\n");
    assert_eq!(
        info.port_config_file("SKU", None),
        Some(hwsku.join("port_config.ini"))
    );
    assert_eq!(info.port_config_file("SKU", Some("0")), None);
    write(&hwsku.join("0/port_config.ini"), "");
    assert_eq!(
        info.port_config_file("SKU", Some("0")),
        Some(hwsku.join("0/port_config.ini"))
    );

    write(&hwsku.join("hwsku.json"), "{}");
    write(&platform_dir.join("platform.json"), r#"{"interfaces": {}}"#);
    assert_eq!(
        info.port_config_file("SKU", None),
        Some(hwsku.join("port_config.ini"))
    );
    write(
        &platform_dir.join("platform.json"),
        r#"{"interfaces": {"Ethernet0": {}}}"#,
    );
    assert_eq!(
        info.port_config_file("SKU", None),
        Some(platform_dir.join("platform.json"))
    );
}
