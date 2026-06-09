# HLD: SAI Notification Handling with ZMQ Southbound

## 1. Background

SONiC recently added support for ZMQ southbound communication between `orchagent` and `syncd` for regular switches. This is controlled by:

```text
DEVICE_METADATA|localhost.orch_southbound_zmq_enabled = true
```

The current issue is tracked in sonic-buildimage issue #27541. An initial short-term implementation is proposed in sonic-swss PR #4619.

In non-ZMQ mode, SAI notifications are published by `syncd` into Redis `ASIC_DB:NOTIFICATIONS`. `orchagent` consumes those notifications from Redis in its normal main loop and dispatches them to the appropriate Orch components.

In ZMQ mode, `syncd` sends notifications to `orchagent` through ZMQ. These notifications arrive in `orchagent` callback functions. Some callbacks already forward notifications to Redis, such as `on_port_state_change()`, but several callbacks are currently empty or incomplete. As a result, some notifications are dropped when ZMQ southbound is enabled.

## 2. Problem Statement

When ZMQ southbound is enabled, the following SAI notifications may not reach their existing Orch consumers:

- `fdb_event`
- `bfd_session_state_change`
- `port_host_tx_ready`
- `icmp_echo_session_state_change`

Additional notification types may also need review, depending on platform and feature coverage.

Expected consumers include:

- `FdbOrch`
- `BfdOrch`
- `PortsOrch`
- `IcmpOrch`

Without a fix:

- Hardware-learned MAC entries may not propagate to `FdbOrch`.
- BFD session state transitions may not reach `BfdOrch`.
- Port host TX readiness may not reach `PortsOrch`.
- ICMP echo session monitoring may not reach `IcmpOrch`.

## 3. Existing Notification Flows

### 3.1 Non-ZMQ Mode

```text
Vendor SAI
  -> syncd SAI callback
  -> syncd NotificationProcessor
  -> RID-to-VID translation
  -> RedisNotificationProducer
  -> ASIC_DB:NOTIFICATIONS
  -> orchagent Redis consumer
  -> orchagent main loop
  -> Orch handler
```

This is the existing proven notification path.

### 3.2 ZMQ Mode Today

```text
Vendor SAI
  -> syncd SAI callback
  -> syncd NotificationProcessor
  -> RID-to-VID translation
  -> ZeroMQNotificationProducer
  -> orchagent ZMQ notification callback
  -> callback-specific logic
```

The issue is that some callback-specific logic is missing.

Example:

```text
on_port_state_change()
  -> forwards to ASIC_DB:NOTIFICATIONS
  -> works today

on_fdb_event()
on_bfd_session_state_change()
on_port_host_tx_ready()
IcmpSaiSessionHandler::on_state_change()
  -> empty or incomplete
  -> notification may be dropped
```

## 4. Design Goals

- Preserve existing non-ZMQ behavior.
- Restore missing notifications in ZMQ mode.
- Avoid duplicate notification delivery.
- Preserve existing Orch handler behavior.
- Avoid unsafe direct access to Orch state from callback context.
- Keep the immediate fix low risk.
- Identify a cleaner long-term architecture.

## 5. Design Options

## Option 1: `syncd` Publishes Notifications via Redis in ZMQ Mode

### Description

In this option, `syncd` would use `RedisNotificationProducer` for SAI notifications even when southbound ZMQ is enabled.

Important clarification: `RedisNotificationProducer` already exists and is used in the non-ZMQ notification path. The new behavior would be to instantiate/use it in the ZMQ-mode notification path as well, instead of using the ZMQ notification producer path for SAI notifications.

### Flow

```text
Vendor SAI
  -> syncd SAI callback
  -> syncd NotificationProcessor
  -> RID-to-VID translation
  -> RedisNotificationProducer
  -> ASIC_DB:NOTIFICATIONS
  -> orchagent Redis consumer
  -> orchagent main loop
  -> Orch handler
```

### Implementation Idea

Option 1 would change the ZMQ-mode notification producer selection in `syncd` from:

```cpp
m_notifications = std::make_shared<ZeroMQNotificationProducer>(m_contextConfig->m_zmqNtfEndpoint);
```

to:

```cpp
m_notifications = std::make_shared<RedisNotificationProducer>(m_contextConfig->m_dbAsic);
```

The command/request channel would still use `ZeroMQSelectableChannel` when ZMQ mode is enabled.

### Pros

- Reuses the existing Redis notification path.
- Existing Orch consumers continue unchanged.
- Minimal `orchagent` callback changes.

### Cons

- Makes ZMQ mode asymmetric:
  - requests/responses use ZMQ
  - notifications use Redis
- Changes `syncd` ZMQ notification semantics.
- May introduce ordering differences between ZMQ command processing and Redis notifications.
- Moves notification transport policy into `syncd`.
- If both Redis and ZMQ notification producers are used, duplicate notifications must be avoided.

## Option 2: `orchagent` Callback Re-posts to `ASIC_DB:NOTIFICATIONS`

### Description

In this option, `syncd` continues to send notifications to `orchagent` through ZMQ. The ZMQ callback in `orchagent` receives the notification and re-publishes it into `ASIC_DB:NOTIFICATIONS` only when ZMQ mode is enabled.

This follows the existing `on_port_state_change()` model and is the approach used by the initial fix in sonic-swss PR #4619.

### Flow

```text
Vendor SAI
  -> syncd SAI callback
  -> syncd NotificationProcessor
  -> RID-to-VID translation
  -> ZeroMQNotificationProducer
  -> orchagent ZMQ callback
  -> NotificationProducer in orchagent
  -> ASIC_DB:NOTIFICATIONS
  -> orchagent Redis consumer
  -> orchagent main loop
  -> Orch handler
```

### Pseudo-code

```cpp
void on_fdb_event(uint32_t count, const sai_fdb_event_notification_data_t *data)
{
    if (gRedisCommunicationMode != SAI_REDIS_COMMUNICATION_MODE_ZMQ_SYNC)
    {
        return;
    }

    static thread_local swss::DBConnector db("ASIC_DB", 0);
    static thread_local swss::NotificationProducer producer(&db, "NOTIFICATIONS");

    std::string payload = sai_serialize_fdb_event_ntf(count, data);
    std::vector<swss::FieldValueTuple> values;

    producer.send(SAI_SWITCH_NOTIFICATION_NAME_FDB_EVENT, payload, values);
}
```

### Pros

- Lowest risk short-term fix.
- No `syncd` change.
- Keeps ZMQ transport between `syncd` and `orchagent`.
- Reuses existing Orch Redis notification processing path.
- Follows existing `on_port_state_change()` behavior.
- Can be implemented and validated event-by-event.

### Cons

- Adds an extra Redis hop after ZMQ delivery.
- Slightly higher latency than direct/in-process handling.
- Still depends on Redis internally for notification dispatch.
- Does not provide the cleanest long-term ZMQ notification model.

## Option 3: Directly Invoke Orch Logic from Callback

### Description

In this option, the ZMQ callback directly invokes the relevant Orch handling logic instead of re-posting to Redis.

### Flow

```text
Vendor SAI
  -> syncd SAI callback
  -> syncd NotificationProcessor
  -> RID-to-VID translation
  -> ZeroMQNotificationProducer
  -> orchagent ZMQ callback
  -> direct Orch handler call
```

### Pseudo-code

```cpp
void on_fdb_event(uint32_t count, const sai_fdb_event_notification_data_t *data)
{
    if (gRedisCommunicationMode != SAI_REDIS_COMMUNICATION_MODE_ZMQ_SYNC)
    {
        return;
    }

    std::string payload = sai_serialize_fdb_event_ntf(count, data);

    gFdbOrch->handleNotification(
        SAI_SWITCH_NOTIFICATION_NAME_FDB_EVENT,
        payload,
        {});
}
```

### Pros

- Avoids Redis re-posting.
- Lower theoretical latency.
- Direct data path.
- Can be optimized event-by-event.

### Cons

- Higher concurrency risk.
- Notification handling runs in the sairedis/ZMQ channel notification thread instead of the normal `orchagent` main loop.
- Existing Orch code may assume single-threaded main-loop execution.
- Requires thread-safety validation per notification type.
- More invasive and harder to prove safe.

## Option 4: In-process Notification Queue Drained by Orch Main Loop

### Description

In this option, the ZMQ callback does not re-post to Redis and does not directly invoke Orch logic. Instead, the callback packages the notification and enqueues it into an internal `orchagent` notification queue. The normal `orchagent` main loop drains the queue and dispatches the event to the appropriate Orch handler.

### Flow

```text
Vendor SAI
  -> syncd SAI callback
  -> syncd NotificationProcessor
  -> RID-to-VID translation
  -> ZeroMQNotificationProducer
  -> orchagent ZMQ callback
  -> in-process notification queue
  -> orchagent main loop
  -> common notification dispatcher
  -> Orch handler
```

### Relationship to Existing Redis Path

```text
Existing Redis path:
ASIC_DB:NOTIFICATIONS
  -> Redis consumer
  -> common dispatcher
  -> Orch handler

New ZMQ path:
ZMQ callback
  -> in-process notification queue
  -> common dispatcher
  -> Orch handler
```

Both paths should converge into common dispatch logic.

### Pseudo-code

```cpp
struct SaiNotification
{
    std::string name;
    std::string payload;
    std::vector<swss::FieldValueTuple> values;
};
```

```cpp
class SaiNotificationQueue
{
public:
    void enqueue(SaiNotification notification)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(notification));
        }

        notifyMainLoop();
    }

    bool tryDequeue(SaiNotification &notification)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_queue.empty())
        {
            return false;
        }

        notification = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

private:
    std::mutex m_mutex;
    std::queue<SaiNotification> m_queue;
    SelectableEvent m_wakeupEvent;

    void notifyMainLoop()
    {
        m_wakeupEvent.notify();
    }
};
```

`notifyMainLoop()` represents a wakeup mechanism for the existing `orchagent` main loop. The implementation could use an `eventfd`, pipe, `SelectableEvent`, or an existing swss selectable wakeup primitive. The important requirement is that enqueueing a notification makes a selectable object readable so `OrchDaemon::start()` wakes immediately.

Callback:

```cpp
void on_fdb_event(uint32_t count, const sai_fdb_event_notification_data_t *data)
{
    if (gRedisCommunicationMode != SAI_REDIS_COMMUNICATION_MODE_ZMQ_SYNC)
    {
        return;
    }

    SaiNotification notification;
    notification.name = SAI_SWITCH_NOTIFICATION_NAME_FDB_EVENT;
    notification.payload = sai_serialize_fdb_event_ntf(count, data);

    gSaiNotificationQueue.enqueue(std::move(notification));
}
```

Main-loop integration:

Option 4 should integrate with the existing `OrchDaemon::start()` `Select` loop through a new `Selectable`/`Executor`, not by directly calling Orch logic from the ZMQ callback.

```text
ZMQ notification thread
  -> orchagent callback
  -> enqueue notification into in-process queue
  -> notifyMainLoop()
  -> wake OrchDaemon select loop
  -> queue executor execute()
  -> drain queued notifications
  -> common notification dispatcher
  -> Orch handler
```

The existing main loop already waits on selectable executors:

```cpp
void OrchDaemon::start(long heartBeatInterval)
{
    for (Orch *o : m_orchList)
    {
        m_select->addSelectables(o->getSelectables());
    }

    while (true)
    {
        Selectable *s;
        int ret = m_select->select(&s, SELECT_TIMEOUT);

        if (ret == Select::OBJECT)
        {
            auto *executor = static_cast<Executor *>(s);
            executor->execute();
        }
    }
}
```

The queued ZMQ notification path would add a new executor to this same model:

```cpp
class SaiNotificationQueueExecutor : public Executor
{
public:
    void execute() override
    {
        m_queue.clearWakeupEvent();

        SaiNotification notification;
        while (m_queue.tryDequeue(notification))
        {
            dispatchSaiNotification(notification);
        }
    }
};
```

During `orchagent` initialization:

```cpp
Orch::addExecutor(new SaiNotificationQueueExecutor(...));
```

The conceptual equivalent is:

```cpp
void processQueuedZmqNotifications()
{
    SaiNotification notification;

    while (gSaiNotificationQueue.tryDequeue(notification))
    {
        dispatchSaiNotification(notification);
    }
}
```

Common dispatcher:

```cpp
void OrchDaemon::dispatchSaiNotification(const SaiNotification &notification)
{
    if (notification.name == SAI_SWITCH_NOTIFICATION_NAME_FDB_EVENT)
    {
        gFdbOrch->handleNotification(notification.name, notification.payload, notification.values);
    }
    else if (notification.name == SAI_SWITCH_NOTIFICATION_NAME_PORT_STATE_CHANGE)
    {
        gPortsOrch->handleNotification(notification.name, notification.payload, notification.values);
    }
    else if (notification.name == SAI_SWITCH_NOTIFICATION_NAME_BFD_SESSION_STATE_CHANGE)
    {
        gBfdOrch->handleNotification(notification.name, notification.payload, notification.values);
    }
}
```

### Note on `on_port_state_change()`

If Option 4 becomes the long-term architecture, existing ZMQ callbacks that currently re-post to Redis, such as `on_port_state_change()`, should eventually be migrated to the queue-based model too.

Short-term:

```text
on_port_state_change()
  -> Redis re-post
```

Long-term:

```text
on_port_state_change()
  -> in-process queue
```

### Pros

- Avoids Redis re-posting.
- Reduces Redis load.
- Lower latency than Option 2.
- Preserves Orch main-loop execution model.
- Lower concurrency risk than direct callback handling.
- Cleaner long-term architecture.
- Can support event-by-event migration.

### Cons

- Requires new queue and wakeup infrastructure in `orchagent`.
- Requires clear ordering and fairness rules between queued ZMQ notifications, Redis notifications, timers, and other Orch tasks.
- Requires backpressure behavior.
- Requires lifecycle handling for the shared queue and wakeup mechanism during startup, shutdown, and callback teardown.
- More implementation and test effort than Option 2.
- Larger design review scope.

## 6. References

- sonic-buildimage issue #27541: Missing notification forwarding for FDB/BFD when ZMQ southbound is enabled
- sonic-swss PR #4619: Forward SAI notifications to Redis in ZMQ southbound mode
