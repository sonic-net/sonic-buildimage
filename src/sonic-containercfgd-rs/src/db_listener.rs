use std::{collections::HashMap, os::fd::AsRawFd, time::Duration};

use anyhow::Context;
use log::{debug, error};
use swss_common::{CxxString, DbConnector, KeyOperation, SubscriberStateTable};

pub trait SonicDatabaseChanges {
    fn handle_change(
        &mut self,
        operation: KeyOperation,
        entry: &str,
        field_values: &HashMap<String, CxxString>,
    ) -> anyhow::Result<()>;
}

#[derive(Debug)]
pub struct SonicDatabaseListener<T: SonicDatabaseChanges> {
    handler: T,
    subscriber: SubscriberStateTable,
}

impl<T> SonicDatabaseListener<T>
where
    T: SonicDatabaseChanges,
{
    pub fn new(db: &str, table: &str, handler: T) -> anyhow::Result<Self> {
        let db_connector =
            DbConnector::new_named(db, false, 0).context(format!("Failed to connect to {}", db))?;
        let subscriber = SubscriberStateTable::new(db_connector, table, None, None)?;
        Ok(Self {
            handler,
            subscriber,
        })
    }

    pub fn process_existing_data(&mut self) -> anyhow::Result<()> {
        match self.subscriber.pops() {
            Ok(items) => {
                for item in items {
                    debug!("Got event on {}", item.key);
                    let result =
                        self.handler
                            .handle_change(item.operation, &item.key, &item.field_values);
                    if let Err(err) = result {
                        error!("Error in handling change: {err:#}")
                    }
                }
                Ok(())
            }
            Err(e) => {
                error!("Error popping items from session table: {}", e);
                Ok(())
            }
        }
    }

    pub fn read_data(&mut self) -> anyhow::Result<()> {
        let timeout = Duration::from_secs(1);

        match self.subscriber.read_data(timeout, false) {
            Ok(select_result) => match select_result {
                swss_common::SelectResult::Data => self.process_existing_data(),
                swss_common::SelectResult::Timeout => {
                    debug!("Timeout waiting for table updates");
                    Ok(())
                }
                swss_common::SelectResult::Signal => {
                    debug!("Signal received while waiting for table updates");
                    Ok(())
                }
            },
            Err(e) => Err(e).context("Error reading from CONFIG_DB"),
        }
    }
}

impl<T> AsRawFd for SonicDatabaseListener<T>
where
    T: SonicDatabaseChanges,
{
    fn as_raw_fd(&self) -> std::os::unix::prelude::RawFd {
        self.subscriber.get_fd().unwrap().as_raw_fd()
    }
}
