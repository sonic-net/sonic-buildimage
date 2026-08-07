#!/usr/bin/env python_nos
import traceback
from tech_support.collector_common import logger


class CollectorBase:
    name = "base"

    def collect(self):
        raise NotImplementedError

    def safe_collect(self):
        try:
            result = self.collect()
            return {"name": self.name, "status": "ok", "data": result}
        except Exception as e:
            logger.error(f"{self.name} collect failed: {e}")
            return {"name": self.name, "status": "error", "error": str(e), "traceback": traceback.format_exc()}
