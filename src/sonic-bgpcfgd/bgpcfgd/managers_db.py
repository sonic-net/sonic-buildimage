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

        if self.__set_handler_validate(key, data) == True:
            cmd_list = []
            bgp_asn = data["bgp_asn"]
            state = data["suppress-fib-pending"]
            cmd_list.append("router bgp %s" % bgp_asn)
            if state == "disabled":
                cmd_list.append("no bgp suppress-fib-pending")
            else:
                cmd_list.append("bgp suppress-fib-pending")
            cmd_list.append("exit")

            self.cfg_mgr.push_list(cmd_list)

        self.directory.put(self.db_name, self.table_name, key, data)

        return True

    def del_handler(self, key):
        """ Implementation of 'DEL' command for this class """
        self.directory.remove(self.db_name, self.table_name, key)

    def __set_handler_validate(self, key, data):
        if data:
            if ((key == "localhost") and ("bgp_asn" in data) and ("suppress-fib-pending" in data and data["suppress-fib-pending"] in ["enabled","disabled"])):
                 return True
        return False
