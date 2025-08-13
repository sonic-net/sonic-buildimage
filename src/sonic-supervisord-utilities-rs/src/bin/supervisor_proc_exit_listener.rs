//! Supervisor process exit listener binary

use sonic_supervisord_utilities_rs::supervisor_proc_exit_listener::main as supervisor_main;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    supervisor_main()?;
    Ok(())
}