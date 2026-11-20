use super::{redis_connect, redis_hget};

#[test]
#[ignore = "requires a read-only Redis fixture at /var/run/redis/redis.sock"]
fn configured_socket_reads_config_db_and_denies_writes() {
    let mut connection = redis_connect().expect("configured Unix socket connection");
    assert_eq!(redis_hget(&mut connection, "WATCHDOG_READ_TEST", "value"),
               Some("config-db-four".to_string()));
    let result: redis::RedisResult<i64> = redis::cmd("HSET")
        .arg("WATCHDOG_READ_TEST").arg("value").arg("forbidden")
        .query(&mut connection);
    assert_eq!(result.unwrap_err().code(), Some("NOPERM"));
    assert_eq!(redis_hget(&mut connection, "WATCHDOG_READ_TEST", "value"),
               Some("config-db-four".to_string()));
}
