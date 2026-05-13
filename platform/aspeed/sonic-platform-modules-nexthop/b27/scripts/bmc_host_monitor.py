#!/usr/bin/env python3
"""
BMC Host Request Monitor

Monitors Redis STATE_DB for host transition requests and executes
the corresponding switch CPU control commands.

Flow:
1. Monitor BMC_HOST_REQUEST in Redis STATE_DB (db 6)
2. When status is "pending", read requested_transition
3. Execute switch_cpu_utils.sh with the transition command
4. Update status to "completed" or "failed"
"""

import redis
import subprocess
import time
import logging
import sys
import os
from datetime import datetime

# Configuration
REDIS_HOST = 'localhost'
REDIS_PORT = 6379
REDIS_DB = 6  # STATE_DB
REDIS_KEY = 'BMC_HOST_REQUEST'
POLL_INTERVAL = 0.5  # seconds
SCRIPT_PATH = '/usr/local/bin/switch_cpu_utils.sh'

# Logging setup
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler('/var/log/bmc_host_monitor.log')
    ]
)
logger = logging.getLogger('bmc_host_monitor')


class BMCHostMonitor:
    """Monitor Redis for host transition requests and execute them"""

    def __init__(self):
        """Initialize Redis connection"""
        self.redis_client = None
        self.last_request_id = None
        self.connect_redis()

    def connect_redis(self):
        """Connect to Redis STATE_DB"""
        try:
            self.redis_client = redis.Redis(
                host=REDIS_HOST,
                port=REDIS_PORT,
                db=REDIS_DB,
                decode_responses=True,
                socket_connect_timeout=5,
                socket_timeout=5
            )
            # Test connection
            self.redis_client.ping()
            logger.info(f"Connected to Redis STATE_DB (db {REDIS_DB})")
        except redis.ConnectionError as e:
            logger.error(f"Failed to connect to Redis: {e}")
            raise

    def get_host_request(self):
        """Get current host request from Redis"""
        try:
            request = self.redis_client.hgetall(REDIS_KEY)
            return request if request else None
        except redis.RedisError as e:
            logger.error(f"Failed to read from Redis: {e}")
            return None

    def update_request_status(self, status, error_msg=None):
        """Update the status of the current request"""
        try:
            updates = {
                'status': status,
                'timestamp': str(int(time.time() * 1e9))  # nanoseconds
            }
            if error_msg:
                updates['error'] = error_msg

            self.redis_client.hset(REDIS_KEY, mapping=updates)
            logger.info(f"Updated request status to: {status}")
        except redis.RedisError as e:
            logger.error(f"Failed to update Redis: {e}")

    def execute_transition(self, transition):
        """Execute the switch CPU control script"""
        if not os.path.exists(SCRIPT_PATH):
            logger.error(f"Script not found: {SCRIPT_PATH}")
            return False, f"Script not found: {SCRIPT_PATH}"

        cmd = [SCRIPT_PATH, transition]
        logger.info(f"Executing: {' '.join(cmd)}")

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30,
                check=False
            )

            if result.returncode == 0:
                logger.info(f"Command succeeded: {transition}")
                logger.debug(f"Output: {result.stdout}")
                return True, None
            else:
                error_msg = f"Command failed with exit code {result.returncode}: {result.stderr}"
                logger.error(error_msg)
                return False, error_msg

        except subprocess.TimeoutExpired:
            error_msg = f"Command timed out: {transition}"
            logger.error(error_msg)
            return False, error_msg
        except Exception as e:
            error_msg = f"Failed to execute command: {e}"
            logger.error(error_msg)
            return False, error_msg

    def process_request(self, request):
        """Process a pending host request"""
        request_id = request.get('request_id')
        transition = request.get('requested_transition')
        status = request.get('status')

        logger.info(f"Processing request: {request_id}, transition: {transition}, status: {status}")

        # Validate transition
        valid_transitions = ['reset-out', 'reset-in', 'reset-cycle']
        if transition not in valid_transitions:
            error_msg = f"Invalid transition: {transition}"
            logger.error(error_msg)
            self.update_request_status('failed', error_msg)
            return

        # Execute the transition
        success, error_msg = self.execute_transition(transition)

        # Update status
        if success:
            self.update_request_status('completed')
        else:
            self.update_request_status('failed', error_msg)

    def run(self):
        """Main monitoring loop"""
        logger.info("BMC Host Monitor started")
        logger.info(f"Monitoring Redis key: {REDIS_KEY}")
        logger.info(f"Script path: {SCRIPT_PATH}")

        while True:
            try:
                request = self.get_host_request()

                if request:
                    request_id = request.get('request_id')
                    status = request.get('status')

                    # Process only new pending requests
                    if status == 'pending' and request_id != self.last_request_id:
                        self.last_request_id = request_id
                        self.process_request(request)

                time.sleep(POLL_INTERVAL)

            except KeyboardInterrupt:
                logger.info("Received shutdown signal")
                break
            except Exception as e:
                logger.error(f"Unexpected error in main loop: {e}", exc_info=True)
                time.sleep(5)  # Back off on errors


def main():
    """Entry point"""
    try:
        monitor = BMCHostMonitor()
        monitor.run()
    except Exception as e:
        logger.error(f"Fatal error: {e}", exc_info=True)
        sys.exit(1)


if __name__ == '__main__':
    main()

