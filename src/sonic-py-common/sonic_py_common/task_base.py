import multiprocessing
import os
import signal
import threading


#
# ProcessTaskBase =====================================================================
#
class ProcessTaskBase(object):
    """
    Base class for creating an object that gets spawned as a separate process

    Child class needs to implement the task_worker method, which should be
    designed to return if task_stopping_event is set

    """
    def __init__(self, stop_timeout_secs=1):
        """
        Initializer

        Args:
            stop_timeout_secs (int): Number of seconds to wait for process to exit
                upon calling task_stop(). If the process fails to stop before the
                specified timeout, it will attemp to kill the process via brute
                force. If you would like to wait indefinitely, pass in `None`.
        """
        self._stop_timeout_secs = stop_timeout_secs
        self._task_process = None
        self.task_stopping_event = multiprocessing.Event()

    def task_worker(self):
        raise NotImplementedError

    def task_run(self):
        if self.task_stopping_event.is_set():
            return

        self._task_process = multiprocessing.Process(target=self.task_worker)
        self._task_process.start()

    def task_stop(self):
        # Signal the process to stop
        self.task_stopping_event.set()

        # Wait for the process to exit
        self._task_process.join(self._stop_timeout_secs)

        # If the process didn't exit, attempt to kill it
        if self._task_process.is_alive():
            os.kill(self._task_process.pid, signal.SIGKILL)

        if self._task_process.is_alive():
            return False

        return True


#
# ThreadTaskBase =====================================================================
#
class ThreadTaskBase(object):
    """
    Base class for creating an object that gets spawned as a separate thread

    Child class needs to implement the task_worker method, which should be
    designed to return if task_stopping_event is set
    """
    def __init__(self, stop_timeout_secs=None):
        """
        Initializer

        Args:
            stop_timeout_secs (int): Number of seconds to wait for thread to exit
                upon calling task_stop(). If you would like to wait indefinitely,
                pass in None.
        """
        self._stop_timeout_secs = stop_timeout_secs
        self._task_thread = None
        self.task_stopping_event = threading.Event()

    def task_worker(self):
        raise NotImplementedError

    def task_run(self):
        if self.task_stopping_event.is_set():
            return

        self._task_thread = threading.Thread(target=self.task_worker)
        self._task_thread.start()

    def task_stop(self):
        # Signal the thread to stop
        self.task_stopping_event.set()

        # Wait for the thread to exit
        self._task_thread.join(self._stop_timeout_secs)

        if self._task_thread.is_alive():
            return False

        return True
