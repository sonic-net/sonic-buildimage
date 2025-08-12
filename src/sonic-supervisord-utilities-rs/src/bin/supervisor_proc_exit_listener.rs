//! Supervisor process exit listener binary

use sonic_supervisord_utilities_rs::supervisor_proc_exit_listener::main as supervisor_main;
use std::process;

fn main() {
    if let Err(e) = supervisor_main() {
        eprintln!("Error: {}", e);
        process::exit(1);
    }
}