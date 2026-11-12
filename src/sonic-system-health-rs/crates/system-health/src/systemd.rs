//! systemd: unit properties from `systemctl show` and job events over D-Bus, as the Python sysmonitor.

use std::collections::HashMap;
use std::time::Duration;

use anyhow::{anyhow, Result};
use async_trait::async_trait;
use futures_util::StreamExt;
use zbus::Connection;
use zbus_systemd::systemd1::ManagerProxy;

use crate::command::{CommandRunner, TokioCommandRunner};

const PROPERTIES: &str =
    "--property=Id,LoadState,UnitFileState,Type,ActiveState,SubState,Result,ConditionResult,ConditionTimestampMonotonic";

/// The unit properties sysmonitor reads, as `systemctl show` prints them.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct UnitProperties {
    pub load_state: String,
    pub unit_file_state: String,
    pub unit_type: String,
    pub active_state: String,
    pub sub_state: String,
    pub result: String,
    /// "yes" or "no".
    pub condition_result: String,
    pub condition_timestamp_monotonic: String,
}

#[async_trait(?Send)]
pub trait Systemd {
    async fn unit_properties(&self, unit: &str) -> Result<UnitProperties>;
}

/// `systemctl show`, one run per unit read.
#[derive(Default)]
pub struct Systemctl;

#[async_trait(?Send)]
impl Systemd for Systemctl {
    async fn unit_properties(&self, unit: &str) -> Result<UnitProperties> {
        let output = TokioCommandRunner
            .run(&["systemctl", "show", PROPERTIES, "--", unit], crate::CALL_TIMEOUT)
            .await
            .map_err(|e| anyhow!("systemctl show {unit}: {e}"))?;
        parse_show(&output)
    }
}

/// `key=value` lines; an output without LoadState (systemctl failed) is an error.
pub fn parse_show(output: &str) -> Result<UnitProperties> {
    let props: HashMap<&str, &str> = output.split('\n').filter_map(|line| line.split_once('=')).collect();
    let get = |key: &str| props.get(key).copied().unwrap_or_default().to_string();
    if !props.contains_key("LoadState") {
        return Err(anyhow!("'LoadState'"));
    }
    Ok(UnitProperties {
        load_state: get("LoadState"),
        unit_file_state: get("UnitFileState"),
        unit_type: get("Type"),
        active_state: get("ActiveState"),
        sub_state: get("SubState"),
        result: get("Result"),
        condition_result: get("ConditionResult"),
        condition_timestamp_monotonic: get("ConditionTimestampMonotonic"),
    })
}

/// Calls `on_unit` for every finished job whose result is "done" or "failed", until the bus fails.
pub async fn watch_job_removed(on_unit: impl FnMut(String)) -> Result<()> {
    let connection = tokio::time::timeout(crate::CALL_TIMEOUT, Connection::system())
        .await
        .map_err(|_| anyhow!("timed out connecting to the system bus"))??;
    watch_job_removed_on(&connection, crate::CALL_TIMEOUT, on_unit).await
}

/// `watch_job_removed` on `connection`; a call to the bus fails after `timeout`.
pub async fn watch_job_removed_on(
    connection: &Connection,
    timeout: Duration,
    mut on_unit: impl FnMut(String),
) -> Result<()> {
    // A bus that stops answering ends the watch, which is then retried, rather than holding it forever.
    let manager = tokio::time::timeout(timeout, ManagerProxy::new(connection))
        .await
        .map_err(|_| anyhow!("timed out creating the systemd manager proxy"))??;
    let mut stream = tokio::time::timeout(timeout, manager.receive_job_removed())
        .await
        .map_err(|_| anyhow!("timed out adding the JobRemoved match"))??;
    tokio::time::timeout(timeout, manager.subscribe())
        .await
        .map_err(|_| anyhow!("timed out subscribing to systemd"))??;
    while let Some(signal) = stream.next().await {
        let args = signal.args()?;
        if args.result == "done" || args.result == "failed" {
            on_unit(args.unit.to_string());
        }
    }
    Err(anyhow!("systemd signal stream ended"))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_systemctl_show() {
        let output = "Id=swss.service\nType=oneshot\nResult=exit-code\nLoadState=loaded\n\
                      ActiveState=inactive\nSubState=dead\nUnitFileState=enabled\nConditionResult=no\n\
                      ConditionTimestampMonotonic=12345\n";
        assert_eq!(
            parse_show(output).unwrap(),
            UnitProperties {
                load_state: "loaded".into(),
                unit_file_state: "enabled".into(),
                unit_type: "oneshot".into(),
                active_state: "inactive".into(),
                sub_state: "dead".into(),
                result: "exit-code".into(),
                condition_result: "no".into(),
                condition_timestamp_monotonic: "12345".into(),
            }
        );
        // A timer has no Type or Result.
        let timer = parse_show("Id=x.timer\nLoadState=loaded\nActiveState=active\n").unwrap();
        assert_eq!((timer.unit_type.as_str(), timer.result.as_str()), ("", ""));
        assert_eq!(parse_show("").unwrap_err().to_string(), "'LoadState'");
    }

    fn contains(received: &[u8], text: &[u8]) -> bool {
        received.windows(text.len()).any(|w| w == text)
    }

    /// Reads from the client until `received` holds `text`.
    async fn read_until(socket: &mut tokio::net::UnixStream, received: &mut Vec<u8>, text: &[u8]) {
        use tokio::io::AsyncReadExt;
        while !contains(received, text) {
            let mut buf = [0; 1024];
            let n = socket.read(&mut buf).await.unwrap();
            assert!(n > 0, "the client closed the connection");
            received.extend_from_slice(&buf[..n]);
        }
    }

    /// A bus that lets a client connect, then answers nothing, as a dbus-daemon that stopped answering.
    async fn silent_bus(mut socket: tokio::net::UnixStream) {
        use tokio::io::AsyncWriteExt;
        let mut received = Vec::new();
        read_until(&mut socket, &mut received, b"\r\n").await;
        socket
            .write_all(b"OK 01234567890123456789012345678901\r\n")
            .await
            .unwrap();
        read_until(&mut socket, &mut received, b"BEGIN\r\n").await;
        if contains(&received, b"NEGOTIATE_UNIX_FD") {
            socket.write_all(b"AGREE_UNIX_FD\r\n").await.unwrap();
        }
        // The answer to the client's Hello, which only its type and body are read of; any path will do.
        let hello = zbus::Message::method_call("/", "Hello").unwrap().build(&()).unwrap();
        let reply = zbus::Message::method_return(&hello.header())
            .unwrap()
            .build(&(":1.1",))
            .unwrap();
        socket.write_all(&reply.data()[..]).await.unwrap();
        std::future::pending::<()>().await;
    }

    #[tokio::test]
    async fn a_bus_that_stops_answering_ends_the_watch() {
        let (client, server) = tokio::net::UnixStream::pair().unwrap();
        let bus = tokio::spawn(silent_bus(server));
        let connection = tokio::time::timeout(
            Duration::from_secs(5),
            zbus::connection::Builder::unix_stream(client).build(),
        )
        .await
        .expect("the connection was not made")
        .unwrap();
        let watch = watch_job_removed_on(&connection, Duration::from_millis(200), |_| {});
        let result = tokio::time::timeout(Duration::from_secs(5), watch)
            .await
            .expect("the watch waits for the bus forever");
        let error = result.unwrap_err().to_string();
        assert!(error.starts_with("timed out"), "{error}");
        bus.abort();
    }
}
