mod lib;
use lib::*;
use log::{error, info};
use std::process::exit;
use swss_common::*;

fn main() {
    env_logger::init();

    let connector: Box<dyn DbConnectorTrait + Send + 'static> = DbConnector::new_with_auth(
        DbConnectionInfo::Named {
            db_name: "COUNTERS_DB".into(),
            is_tcp_conn: false,
        },
        0,
        true,
    )
        .map(|conn| Box::new(conn) as Box<dyn DbConnectorTrait + Send + 'static>)
        .unwrap_or_else(|e| {
            error!("Initialization error: {}", e);
            exit(1);
        });

    plugin_main(connector);
}