from swsscommon.swsscommon import ConfigDBConnector
db = ConfigDBConnector()
db.connect()
def apply_config():
    cfg = db.get_entry('PTP', 'global')
    ports = db.get_keys('PTP_PORT')
    # render ptp4l.conf, then os.kill(ptp4l_pid, signal.SIGHUP)
db.subscribe('PTP', lambda table, key, op, fvs: apply_config())
db.subscribe('PTP_PORT', lambda table, key, op, fvs: apply_config())
db.listen()
