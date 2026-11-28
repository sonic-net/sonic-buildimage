//! Structured events, published as the Python daemon did through sonic-events-host.

use std::cell::RefCell;
use std::collections::HashMap;

use swss_common::EventPublisher;

use crate::log;

pub const EVENTS_PUBLISHER_SOURCE: &str = "sonic-events-host";

/// The parameters of one event.
pub type Params<'a> = &'a [(&'a str, &'a str)];

pub trait EventSink {
    /// Publishes one batch of events with the same tag.
    fn publish(&self, tag: &str, events: &[Params<'_>]);
}

/// Publishes nothing; the CLI uses it so `show system-health` sends no events.
pub struct NoEvents;

impl EventSink for NoEvents {
    fn publish(&self, _tag: &str, _events: &[Params<'_>]) {}
}

/// Each batch opens a publisher, publishes and closes it, as Python does. libswsscommon keeps one publisher per
/// source in a process, shared with any Python code running in it, so none is held between batches.
pub struct SwssEvents;

impl EventSink for SwssEvents {
    fn publish(&self, tag: &str, events: &[Params<'_>]) {
        if events.is_empty() {
            return;
        }
        let publisher = match EventPublisher::new(EVENTS_PUBLISHER_SOURCE) {
            Ok(publisher) => publisher,
            Err(e) => {
                log::error(log::HEALTHD, &format!("Failed to initialize events publisher: {e:?}"));
                return;
            }
        };
        for params in events {
            let params: HashMap<String, String> = params.iter().map(|(k, v)| (k.to_string(), v.to_string())).collect();
            if let Err(e) = publisher.publish(tag, Some(&params)) {
                log::error(log::HEALTHD, &format!("Failed to publish event {tag}: {e:?}"));
            }
        }
        // Dropping the publisher closes it.
    }
}

/// An event: tag and parameters.
pub type Event = (String, Vec<(String, String)>);

/// Records events for tests, and the size of each batch.
#[derive(Default)]
pub struct RecordedEvents {
    pub events: RefCell<Vec<Event>>,
    pub batches: RefCell<Vec<usize>>,
}

impl EventSink for RecordedEvents {
    fn publish(&self, tag: &str, events: &[Params<'_>]) {
        if events.is_empty() {
            return;
        }
        self.batches.borrow_mut().push(events.len());
        for params in events {
            self.events.borrow_mut().push((
                tag.to_string(),
                params.iter().map(|(k, v)| (k.to_string(), v.to_string())).collect(),
            ));
        }
    }
}
