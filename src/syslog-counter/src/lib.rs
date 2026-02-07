use std::io::{self, BufRead, Write};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;
use log::{error, info};
use std::process::exit;
use swss_common::*;

pub fn convert_to_CxxString(num: u64) -> CxxString {
    CxxString::new(num.to_string())
}

pub fn convert_to_u64(str: CxxString) -> Result<u64, String> {
    match str.to_str() {
        Ok(text) => text.parse::<u64>().map_err(|e| format!("Parse error: {}", e)),
        Err(_) => Err("Failed to convert CxxString to &str".to_string()),
    }
}

pub trait DbConnectorTrait: Send  {
    fn new_named(
        db_name: impl Into<String>,
        is_tcp_conn: bool,
        timeout_ms: u32,
    ) -> Result<Box<dyn DbConnectorTrait + Send + 'static>>
    where
        Self: Sized;

    fn hget(&self, key: &str, field: &str) -> Result<Option<CxxString>>;
    fn hset(&self, key: &str, field: &str, value: &CxxStr) -> Result<()>;
}

impl DbConnectorTrait for DbConnector {
    fn new_named(
        db_name: impl Into<String>,
        is_tcp_conn: bool,
        timeout_ms: u32,
    ) -> Result<Box<dyn DbConnectorTrait + Send + 'static>> {
        DbConnector::new_named(db_name, is_tcp_conn, timeout_ms)
            .map(|conn| Box::new(conn) as Box<dyn DbConnectorTrait + Send + 'static>)
    }

    fn hget(&self, key: &str, field: &str) -> Result<Option<CxxString>> {
        self.hget(key, field)
    }

    fn hset(&self, key: &str, field: &str, value: &CxxStr) -> Result<()> {
        self.hset(key, field, value)
    }
}

pub struct CountersDb {
    pub connector: Box<dyn DbConnectorTrait + Send + 'static>,
}

impl CountersDb {
    pub fn new(connector: Box<dyn DbConnectorTrait + Send + 'static>) -> Self {
        Self { connector }
    }

    pub fn get_count(&self) -> Result<u64, String> {
        match self.connector.hget("SYSLOG_COUNTER", "COUNT") {
            Ok(Some(cxx_str)) => convert_to_u64(cxx_str),
            Ok(None) => Err("COUNT was None".to_string()),
            Err(e) => Err(format!("Failed to hget COUNT: {:?}", e)),
        }

    }

    pub fn set_count(&self, count: u64) -> Result<(), String> {
        self.connector.hset("SYSLOG_COUNTER", "COUNT", &convert_to_CxxString(count))
            .map_err(|e| format!("Failed to set COUNT: {:?}", e))
    }
}

fn update_counter_db(counter: Arc<Mutex<u64>>, db: CountersDb) {
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

pub fn plugin_main(connector: Box<dyn DbConnectorTrait>) {
    info!("Syslog counter plugin started");

    let counter = Arc::new(Mutex::new(0u64));
    let counter_clone = Arc::clone(&counter);
    let db = CountersDb::new(connector);

    thread::spawn(move || {
        update_counter_db(counter_clone, db);
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
