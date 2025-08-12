//! Supervisor childutils equivalent
//! 
//! This module provides utilities for parsing supervisor protocol events
//! and managing listener state transitions.

use std::collections::HashMap;
use std::io::{self, Write};
use thiserror::Error;

#[derive(Error, Debug)]
pub enum ChildutilsError {
    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),
    #[error("Parse error: {0}")]
    Parse(String),
}

pub type Result<T> = std::result::Result<T, ChildutilsError>;

/// Supervisor protocol event headers
#[derive(Debug, Clone)]
pub struct EventHeaders {
    pub ver: String,
    pub server: String,
    pub serial: u64,
    pub pool: String,
    pub poolserial: u64,
    pub eventname: String,
    pub len: usize,
}

/// Process event payload headers
#[derive(Debug, Clone)]
pub struct ProcessEventHeaders {
    pub processname: String,
    pub groupname: String,
    pub from_state: String,
    pub expected: i32,
    pub pid: Option<u32>,
}

/// Supervisor listener states
#[derive(Debug, Clone, Copy)]
pub enum ListenerState {
    Acknowledged,
    Ready,
    Busy,
}

/// Parse supervisor event headers
pub fn get_headers(line: &str) -> Result<EventHeaders> {
    let mut headers = HashMap::new();
    
    // Parse space-separated key:value pairs
    for pair in line.trim().split_whitespace() {
        let parts: Vec<&str> = pair.splitn(2, ':').collect();
        if parts.len() == 2 {
            headers.insert(parts[0].to_string(), parts[1].to_string());
        }
    }
    
    // Extract required fields with defaults
    let ver = headers.get("ver").cloned().unwrap_or_default();
    let server = headers.get("server").cloned().unwrap_or_else(|| "supervisor".to_string());
    let serial = headers.get("serial")
        .and_then(|s| s.parse().ok())
        .unwrap_or(0);
    let pool = headers.get("pool").cloned().unwrap_or_else(|| "supervisor".to_string());
    let poolserial = headers.get("poolserial")
        .and_then(|s| s.parse().ok())
        .unwrap_or(0);
    let eventname = headers.get("eventname").cloned().unwrap_or_default();
    let len = headers.get("len")
        .and_then(|s| s.parse().ok())
        .unwrap_or(0);
    
    Ok(EventHeaders {
        ver,
        server,
        serial,
        pool,
        poolserial,
        eventname,
        len,
    })
}

/// Parse event payload data
pub fn eventdata(payload: &str) -> Result<(ProcessEventHeaders, String)> {
    let lines: Vec<&str> = payload.lines().collect();
    if lines.is_empty() {
        return Err(ChildutilsError::Parse("Empty payload".to_string()));
    }
    
    let header_line = lines[0];
    let mut headers = HashMap::new();
    
    // Parse space-separated key:value pairs
    for pair in header_line.trim().split_whitespace() {
        let parts: Vec<&str> = pair.splitn(2, ':').collect();
        if parts.len() == 2 {
            headers.insert(parts[0].to_string(), parts[1].to_string());
        }
    }
    
    let processname = headers.get("processname").cloned().unwrap_or_default();
    let groupname = headers.get("groupname").cloned().unwrap_or_default();
    let from_state = headers.get("from_state").cloned().unwrap_or_default();
    let expected = headers.get("expected")
        .and_then(|s| s.parse().ok())
        .unwrap_or(0);
    let pid = headers.get("pid").and_then(|p| p.parse().ok());
    
    let process_headers = ProcessEventHeaders {
        processname,
        groupname,
        from_state,
        expected,
        pid,
    };
    
    // Payload data is everything after the first line
    let payload_data = if lines.len() > 1 {
        lines[1..].join("\n")
    } else {
        String::new()
    };
    
    Ok((process_headers, payload_data))
}

/// Supervisor listener module
pub mod listener {
    use super::*;

    /// Transition to READY state
    pub fn ready() -> Result<()> {
        print!("READY\n");
        io::stdout().flush()?;
        Ok(())
    }

    /// Transition to OK state
    pub fn ok() -> Result<()> {
        print!("RESULT 2\nOK");
        io::stdout().flush()?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_get_headers() {
        let line = "ver:3.0 server:supervisor serial:21442 pool:supervisor poolserial:21442 eventname:PROCESS_STATE_EXITED len:71";
        let headers = get_headers(line).unwrap();
        
        assert_eq!(headers.ver, "3.0");
        assert_eq!(headers.server, "supervisor");
        assert_eq!(headers.serial, 21442);
        assert_eq!(headers.eventname, "PROCESS_STATE_EXITED");
        assert_eq!(headers.len, 71);
    }

    #[test]
    fn test_eventdata() {
        let payload = "processname:cat groupname:cat from_state:RUNNING expected:0 pid:2766\n";
        let (headers, data) = eventdata(payload).unwrap();
        
        assert_eq!(headers.processname, "cat");
        assert_eq!(headers.groupname, "cat");
        assert_eq!(headers.from_state, "RUNNING");
        assert_eq!(headers.expected, 0);
        assert_eq!(headers.pid, Some(2766));
        assert_eq!(data, "");
    }

    #[test]
    fn test_eventdata_with_payload() {
        let payload = "processname:test groupname:test from_state:RUNNING expected:1 pid:1234\nSome payload data\nMore data";
        let (headers, data) = eventdata(payload).unwrap();
        
        assert_eq!(headers.processname, "test");
        assert_eq!(data, "Some payload data\nMore data");
    }
}