use std::io::{self, BufRead, Write};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;
use log::{error, info};
use std::process::exit;
use swss_common::*;

pub fn convert_to_CxxString(opt: Option<u64>) -> Option<CxxString> {
    opt.map(|num| CxxString::new(num.to_string()))
}

pub fn convert_to_u64(opt: Option<&CxxString>) -> Option<u64> {
    opt.and_then(|s| match s.to_str() {
        Ok(text) => text.parse::<u64>().ok(),
        Err(_) => None,
    })
}

pub struct CountersDb {
    pub connector: DbConnector,
}

impl CountersDb {
    pub fn connect() -> Result<Self, String> {
        DbConnector::new_named("COUNTERS_DB", false, 0)
            .map(|connector| Self { connector })
            .map_err(|e| format!("Failed to connect to COUNTERS_DB: {:?}", e))
    }

    pub fn get_count(&self) -> Result<u64, String> {
        let raw = self.connector.hget("SYSLOG_COUNTER", "COUNT")
            .map_err(|e| format!("Failed to hget COUNT: {:?}", e))?;
        convert_to_u64(raw.as_ref())
            .ok_or_else(|| "Failed to parse COUNT as u64".to_string())
    }

    pub fn set_count(&self, count: u64) -> Result<(), String> {
        let cxx_val = convert_to_CxxString(Some(count))
            .ok_or_else(|| "Failed to convert count to CxxString".to_string())?;
        self.connector.hset("SYSLOG_COUNTER", "COUNT", cxx_val.as_cxx_str())
            .map_err(|e| format!("Failed to set COUNT: {:?}", e))
    }
}

fn update_counter_db(counter: Arc<Mutex<u64>>) {
    let db = match CountersDb::connect() {
        Ok(db) => db,
        Err(e) => {
            error!("Initialization error: {}", e);
            exit(1);
        }
    };

    match db.get_count() {
        Ok(initial_value) => {
            let mut count = counter.lock().unwrap();
            *count = initial_value;
            info!("Initialized counter to {}", initial_value);
        }
        Err(e) => {
            info!("Failed to read initial count: {}, set count to 0", e);
            let mut count = counter.lock().unwrap();
            *count = 0;
        }
    }

    loop {
        let count = {
            let count = counter.lock().unwrap();
            *count
        };

        if let Err(e) = db.set_count(count) {
            error!("Database update error: {}", e);
            // Consider retrying instead of exiting
            exit(1);
        }

        thread::sleep(Duration::from_secs(60));
    }
}

pub fn plugin_main() {
    env_logger::init();
    info!("Syslog counter plugin started");

    let counter = Arc::new(Mutex::new(0u64));
    let counter_clone = Arc::clone(&counter);

    thread::spawn(move || {
        update_counter_db(counter_clone);
    });

    println!("OK");

    let stdin = io::stdin();
    for line in stdin.lock().lines() {
        match line {
            Ok(_) => {
                let mut count = counter.lock().unwrap();
                *count += 1;
                println!("OK");
                io::stdout().flush().unwrap();
            }
            Err(e) => {
                error!("Unrecoverable input error: {}", e);
                exit(1);
            }
        }
    }
}
