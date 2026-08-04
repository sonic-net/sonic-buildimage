from .manager import Manager


class BGPDataBaseMgr(Manager):
    """ This class updates the Directory object when db table is updated """
    def __init__(self, common_objs, db, table):
        """
        Initialize the object
        :param common_objs: common object dictionary
        :param db: name of the db
        :param table: name of the table in the db
        """
        super(BGPDataBaseMgr, self).__init__(
            common_objs,
            [],
            db,
            table,
        )

    def set_handler(self, key, data):
        """ Implementation of 'SET' command for this class """
        self.directory.put(self.db_name, self.table_name, key, data)

        return True

    def del_handler(self, key):
        """ Implementation of 'DEL' command for this class """
        self.directory.remove(self.db_name, self.table_name, key)