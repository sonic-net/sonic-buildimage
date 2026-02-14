from collections import defaultdict

from .log import log_err


class Directory(object):
    """ This class stores values and notifies callbacks which were registered to be executed as soon
        as some value is changed. This class works as DB cache mostly """
    def __init__(self):
        self.data = defaultdict(dict)  # storage. A key is a slot name, a value is a dictionary with data
        self.notify = defaultdict(lambda: defaultdict(list))  # registered callbacks: slot -> path -> handlers[]

    @staticmethod
    def get_slot_name(db, table):
        """ Convert db, table pair into a slot name """
        return db + "__" + table

    def path_traverse(self, slot, path):
        """
        Traverse a path in the storage.
        If the path is an empty string, it returns a value as it is.
        If the path is not an empty string, the method will traverse through the dictionary value.
        Example:
            self.data["key_1"] = { "abc": { "cde": { "fgh": "val_1", "ijk": "val_2" } } }
            self.path_traverse("key_1", "abc/cde") will return True, { "fgh": "val_1", "ijk": "val_2" }
        :param slot: storage key
        :param path: storage path as a string where each internal key is separated by '/'
        :return: a pair: True if the path was found, object if it was found
        """
        if slot not in self.data:
            return False, None
        elif path == '':
            return True, self.data[slot]
        d = self.data[slot]
        for p in path.split("/"):
            if p not in d:
                return False, None
            d = d[p]
        return True, d

    def path_exist(self, db, table, path):
        """
        Check if the path exists in the storage
        :param db: db name
        :param table: table name
        :param path: requested path
        :return: True if the path is available, False otherwise
        """
        slot = self.get_slot_name(db, table)
        return self.path_traverse(slot, path)[0]

    def get_path(self, db, table, path):
        """
        Return the requested path from the storage
        :param db: db name
        :param table: table name
        :param path: requested path
        :return: object if the path was found, None otherwise
        """
        slot = self.get_slot_name(db, table)
        return self.path_traverse(slot, path)[1]

    def put(self, db, table, key, value):
        """
        Put information into the storage. Notify handlers which are dependant to the information
        :param db: db name
        :param table: table name
        :param key: key to change
        :param value: value to put
        :return:
        """
        slot = self.get_slot_name(db, table)
        self.data[slot][key] = value
        if slot in self.notify:
            handlers_to_run = []

            for path in self.notify[slot].keys():
                if self.path_exist(db, table, path):
                    handlers_to_run += self.notify[slot][path]

            for handler in handlers_to_run:
                handler()

    def get(self, db, table, key):
        """
        Get a value from the storage
        :param db: db name
        :param table: table name
        :param key: ket to get
        :return: value for the key
        """
        slot = self.get_slot_name(db, table)
        return self.data[slot][key]

    def get_slot(self, db, table):
        """
        Get an object from the storage
        :param db: db name
        :param table: table name
        :return: object for the slot
        """
        slot = self.get_slot_name(db, table)
        return self.data[slot]

    def remove(self, db, table, key):
        """
        Remove a value from the storage
        :param db: db name
        :param table: table name
        :param key: key to remove
        """
        slot = self.get_slot_name(db, table)
        if slot in self.data:
            if key in self.data[slot]:
                del self.data[slot][key]
            else:
                log_err("Directory: Can't remove key '%s' from slot '%s'. The key doesn't exist" % (key, slot))
        else:
            log_err("Directory: Can't remove key '%s' from slot '%s'. The slot doesn't exist" % (key, slot))

    def remove_slot(self, db, table):
        """
        Remove an object from the storage
        :param db: db name
        :param table: table name
        """
        slot = self.get_slot_name(db, table)
        if slot in self.data:
            del self.data[slot]
        else:
            log_err("Directory: Can't remove slot '%s'. The slot doesn't exist" % slot)

    def available(self, db, table):
        """
        Check if the table is available
        :param db: db name
        :param table: table name
        :return: True if the slot is available, False if not
        """
        slot = self.get_slot_name(db, table)
        return slot in self.data

    def available_deps(self, deps):
        """
        Check if all items from the deps list is available in the storage
        :param deps: list of dependencies
        :return: True if all dependencies are presented, False otherwise
        """
        res = True
        for db, table, path in deps:
            res = res and self.path_exist(db, table, path)
        return res

    def subscribe(self, deps, handler):
        """
        Subscribe the handler to be run as soon as all dependencies are presented
        :param deps:
        :param handler:
        :return:
        """
        for db, table, path in deps:
            slot = self.get_slot_name(db, table)
            self.notify[slot][path].append(handler)

    def unsubscribe(self, deps):
        for db, table, path in deps:
            slot = self.get_slot_name(db, table)
            if slot in self.notify:
                if path in self.notify[slot]:
                    del self.notify[slot][path]