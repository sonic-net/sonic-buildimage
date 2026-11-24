use super::{redis_hget, CONFIG_DB_REDIS_URL};

#[test]
#[ignore = "requires a read-only Redis fixture at /var/run/redis/redis.sock"]
fn configured_socket_reads_config_db_and_denies_writes() {
    assert_eq!(redis_hget("WATCHDOG_READ_TEST", "value"),
               Some("config-db-four".to_string()));
    let mut connection = redis::Client::open(CONFIG_DB_REDIS_URL).unwrap()
        .get_connection().unwrap();
    let result: redis::RedisResult<i64> = redis::cmd("HSET")
        .arg("WATCHDOG_READ_TEST").arg("value").arg("forbidden")
        .query(&mut connection);
    assert_eq!(result.unwrap_err().code(), Some("NOPERM"));
    assert_eq!(redis_hget("WATCHDOG_READ_TEST", "value"),
               Some("config-db-four".to_string()));
}
