mod lib;
use lib::*;
use log::{error, info};
use std::process::exit;
use swss_common::*;

fn main() {
    env_logger::init();

    let connector: Box<dyn DbConnectorTrait + Send + 'static> = <DbConnector as DbConnectorTrait>::new_named("COUNTERS_DB", false, 0)
        .unwrap_or_else(|e| {
            error!("Initialization error: {}", e);
            exit(1);
        });

    plugin_main(connector);
}