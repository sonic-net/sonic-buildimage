#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import copy
import threading
import time
from sonic_py_common.logger import Logger

logger = Logger()
EMPTY_SET = set()


class WaitSfpReadyTask(threading.Thread):
    """When bring a module from powered off to powered on, it takes 3 seconds
    for module to load its firmware. This class is designed to perform a wait for
    those modules who are loading firmware.
    """
    WAIT_TIME = 3
    
    def __init__(self):
        # Set daemon to True so that the thread will be destroyed when daemon exits.
        super().__init__(daemon=True)
        self.running = False
        
        # Lock to protect the wait list 
        self.lock = threading.Lock()
        
        # Event to wake up thread function
        self.event = threading.Event()
        
        # A list of SFP to be waited. Key is SFP index, value is the expire time.
        self._wait_dict = {}
        
        # The queue to store those SFPs who finish loading firmware.
        self._ready_set = set()
        
    def stop(self):
        """Stop the task, only used in unit test
        """
        self.running = False
        self.event.set()
        
    def schedule_wait(self, sfp_index):
        """Add a SFP to the wait list

        Args:
            sfp_index (int): the index of the SFP object
        """
        logger.log_debug(f'SFP {sfp_index} is scheduled for waiting reset done')
        with self.lock:
            is_empty = len(self._wait_dict) == 0
  
            # The item will be expired in 3 seconds
            self._wait_dict[sfp_index] = time.monotonic() + self.WAIT_TIME

        if is_empty:
            logger.log_debug('An item arrives, wake up WaitSfpReadyTask')
            # wake up the thread
            self.event.set()
    
    def cancel_wait(self, sfp_index):
        """Cancel a SFP from the wait list

        Args:
            sfp_index (int): the index of the SFP object
        """
        logger.log_debug(f'SFP {sfp_index} is canceled for waiting reset done')
        with self.lock:
            if sfp_index in self._wait_dict:
                self._wait_dict.pop(sfp_index)
            if sfp_index in self._ready_set:
                self._ready_set.pop(sfp_index)
                
    def get_ready_set(self):
        """Get ready set and clear it

        Returns:
            set: a deep copy of self._ready_set
        """
        with self.lock:
            if not self._ready_set:
                return EMPTY_SET
            ready_set = copy.deepcopy(self._ready_set)
            self._ready_set.clear()
        return ready_set
            
    def empty(self):
        """Indicate if wait_dict is empty

        Returns:
            bool: True if wait_dict is empty
        """
        with self.lock:
            return len(self._wait_dict) == 0

    def run(self):
        """Thread function
        """
        self.running = True
        pending_remove_set = set()
        is_empty = True
        while self.running:
            if is_empty:
                logger.log_debug(f'WaitSfpReadyTask is waiting for task...')
                # If wait_dict is empty, hold the thread until an item coming
                self.event.wait()
                self.event.clear()

            now = time.monotonic()
            with self.lock:
                logger.log_debug(f'Processing wait SFP dict: {self._wait_dict}, now={now}')
                for sfp_index, expire_time in self._wait_dict.items():
                    # If now time is greater than the expire time, remove
                    # the item from wait_dict
                    if now >= expire_time:
                        pending_remove_set.add(sfp_index)

                for sfp_index in pending_remove_set:
                    self._wait_dict.pop(sfp_index)
                    self._ready_set.add(sfp_index)
                    
                is_empty = (len(self._wait_dict) == 0)
                    
            pending_remove_set.clear()
            time.sleep(1)
