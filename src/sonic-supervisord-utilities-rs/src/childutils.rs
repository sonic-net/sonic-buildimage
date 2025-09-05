//! Supervisor childutils equivalent
//! 
//! This module provides utilities for parsing supervisor protocol events
//! and managing listener state transitions.

use std::collections::HashMap;
use std::io::{self, Write};


/// Parse supervisor event headers - returns HashMap like Python version
pub fn get_headers(line: &str) -> HashMap<String, String> {
    let mut headers = HashMap::new();
    
    // Parse space-separated key:value pairs
    for pair in line.trim().split_whitespace() {
        let parts: Vec<&str> = pair.splitn(2, ':').collect();
        if parts.len() == 2 {
            headers.insert(parts[0].to_string(), parts[1].to_string());
        }
    }
    
    headers
}

/// Parse event payload data - returns HashMap and data like Python version
pub fn eventdata(payload: &str) -> (HashMap<String, String>, String) {
    if let Some((header_line, data)) = payload.split_once('\n') {
        let headers = get_headers(header_line);
        (headers, data.to_string())
    } else {
        // No newline found, treat entire payload as headers with empty data
        let headers = get_headers(payload);
        (headers, String::new())
    }
}

/// Supervisor listener module
pub mod listener {
    use super::*;

    /// Transition to READY state - matches Python version that ignores flush() return
    pub fn ready() {
        print!("READY\n");
        let _ = io::stdout().flush(); // Ignore flush result like Python
    }

    /// Transition to OK state - matches Python version that ignores flush() return  
    pub fn ok() {
        print!("RESULT 2\nOK");
        let _ = io::stdout().flush(); // Ignore flush result like Python
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_get_headers() {
        let line = "ver:3.0 server:supervisor serial:21442 pool:supervisor poolserial:21442 eventname:PROCESS_STATE_EXITED len:71";
        let headers = get_headers(line);
        
        assert_eq!(headers.get("ver"), Some(&"3.0".to_string()));
        assert_eq!(headers.get("server"), Some(&"supervisor".to_string()));
        assert_eq!(headers.get("serial"), Some(&"21442".to_string()));
        assert_eq!(headers.get("eventname"), Some(&"PROCESS_STATE_EXITED".to_string()));
        assert_eq!(headers.get("len"), Some(&"71".to_string()));
    }

    #[test]
    fn test_eventdata() {
        let payload = "processname:cat groupname:cat from_state:RUNNING expected:0 pid:2766\n";
        let (headers, data) = eventdata(payload);
        
        assert_eq!(headers.get("processname"), Some(&"cat".to_string()));
        assert_eq!(headers.get("groupname"), Some(&"cat".to_string()));
        assert_eq!(headers.get("from_state"), Some(&"RUNNING".to_string()));
        assert_eq!(headers.get("expected"), Some(&"0".to_string()));
        assert_eq!(headers.get("pid"), Some(&"2766".to_string()));
        assert_eq!(data, "");
    }

    #[test]
    fn test_eventdata_with_payload() {
        let payload = "processname:test groupname:test from_state:RUNNING expected:1 pid:1234\nSome payload data\nMore data";
        let (headers, data) = eventdata(payload);
        
        assert_eq!(headers.get("processname"), Some(&"test".to_string()));
        assert_eq!(data, "Some payload data\nMore data");
    }
}