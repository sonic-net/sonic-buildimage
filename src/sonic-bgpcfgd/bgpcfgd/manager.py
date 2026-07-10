from swsscommon import swsscommon

from .log import log_debug, log_err


class Manager(object):
    """ This class represents a SONiC DB table """
    def __init__(self, common_objs, deps, database, table_name, wait_for_all_deps=True):
        """
        Initialize class
        :param common_objs: common object dictionary
        :param deps: dependencies list
        :param database: database name
        :param table_name: table name
        """
        self.directory = common_objs['directory']
        self.cfg_mgr = common_objs['cfg_mgr']
        self.constants = common_objs['constants']
        self.deps = deps
        self.db_name = database
        self.table_name = table_name
        self.wait_for_all_deps = wait_for_all_deps  # control whether the manager should wait for all dependencies to be set before processing any 'SET' command
        self.set_queue = []
        self.directory.subscribe(deps, self.on_deps_change)  # subscribe this class method on directory changes

    def get_database(self):
        """ Return associated database """
        return self.db_name

    def get_table_name(self):
        """ Return associated table name"""
        return self.table_name

    def handler(self, key, op, data):
        """
        This method is executed on each add/remove event on the table.
        :param key: key of the table entry
        :param op: operation on the table entry. Could be either 'SET' or 'DEL'
        :param data: associated data of the event. Empty for 'DEL' operation.
        """
        if op == swsscommon.SET_COMMAND:
            if (not self.wait_for_all_deps) or self.directory.available_deps(self.deps):  # all required dependencies are set in the Directory?
                res = self.set_handler(key, data)
                if not res:  # set handler returned False, which means it is not ready to process is. Save it for later.
                    log_debug("'SET' handler returned NOT_READY for the Manager: %s" % self.__class__)
                    self.set_queue.append((key, data))
            else:
                log_debug("Not all dependencies are met for the Manager: %s" % self.__class__)
                self.set_queue.append((key, data))
        elif op == swsscommon.DEL_COMMAND:
            self.del_handler(key)
        else:
            log_err("Invalid operation '%s' for key '%s'" % (op, key))

    def on_deps_change(self):
        """ This method is being executed on every dependency change """
        if self.wait_for_all_deps and not self.directory.available_deps(self.deps):
            return
        new_queue = []
        for key, data in self.set_queue:
            res = self.set_handler(key, data)
            if not res:
                new_queue.append((key, data))
        self.set_queue = new_queue

    def set_handler(self, key, data):
        """ Placeholder for 'SET' command """
        log_err("set_handler() wasn't implemented for %s" % self.__class__.__name__)

    def del_handler(self, key):
        """ Placeholder for 'DEL' command """
        log_err("del_handler wasn't implemented for %s" % self.__class__.__name__)