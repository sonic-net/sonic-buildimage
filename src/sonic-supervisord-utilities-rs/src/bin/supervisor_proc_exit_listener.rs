//! Supervisor process exit listener binary

use sonic_supervisord_utilities_rs::proc_exit_listener::main_with_args;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    main_with_args(None)?;
    Ok(())
}