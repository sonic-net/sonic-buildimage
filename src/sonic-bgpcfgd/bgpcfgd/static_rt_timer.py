from .log import log_err, log_info, log_debug
from swsscommon import swsscommon
import time

class StaticRouteTimer(object):
    """ This class checks the static routes and deletes those entries that have not been refreshed """
    def __init__(self):
        self.db = swsscommon.SonicV2Connector()
        self.db.connect(self.db.APPL_DB)
        self.timer = None
        self.start = None

    DEFAULT_TIMER = 180
    DEFAULT_SLEEP = 60
    # keep same range as value defined in sonic-restapi/sonic_api.yaml
    MAX_TIMER     = 172800

    def set_timer(self):
        """ Check for custom route expiry time in STATIC_ROUTE_EXPIRY_TIME """
        timer = self.db.get(self.db.APPL_DB, "STATIC_ROUTE_EXPIRY_TIME", "time")
        if timer is not None:
            if timer.isdigit():
                timer = int(timer)
                if timer > 0 and timer <= self.MAX_TIMER:
                    self.timer = timer
                    return
                log_err("Custom static route expiry time of {}s is invalid!".format(timer))
        return

    def alarm(self):
        """ Clear unrefreshed static routes """
        static_routes = self.db.keys(self.db.APPL_DB, "STATIC_ROUTE:*")
        if static_routes is not None:
            for sr in static_routes:
                expiry = self.db.get(self.db.APPL_DB, sr, "expiry")
                if expiry == "false":
                    continue
                refresh = self.db.get(self.db.APPL_DB, sr, "refresh")
                if refresh == "true":
                    self.db.set(self.db.APPL_DB, sr, "refresh", "false")
                    log_debug("Refresh status of static route {} is set to false".format(sr))
                else:
                    self.db.delete(self.db.APPL_DB, sr)
                    log_debug("Static route {} deleted".format(sr))
        self.start = time.time()
        return

    def run(self):
        self.start = time.time()
        while True:
            self.set_timer()
            if self.timer:
                log_info("Static route expiry set to {}s".format(self.timer))
                time.sleep(self.timer)
                self.alarm()
            else:
                time.sleep(self.DEFAULT_SLEEP)
                if time.time() - self.start >= self.DEFAULT_TIMER:
                    self.alarm()

