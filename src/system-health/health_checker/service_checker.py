import docker
import os
import pickle
import re

from swsscommon import swsscommon
from sonic_py_common import multi_asic, device_info
from sonic_py_common.logger import Logger
from .health_checker import HealthChecker
from . import utils

SYSLOG_IDENTIFIER = 'service_checker'
logger = Logger(log_identifier=SYSLOG_IDENTIFIER)

EVENTS_PUBLISHER_SOURCE = "sonic-events-host"
EVENTS_PUBLISHER_TAG = "process-not-running"

def check_docker_image(image_name):
    """
    @summary: This function will check if docker image exists.
    @return:  True if the image exists, otherwise False.
    """
    try:
        DOCKER_CLIENT = docker.DockerClient(base_url='unix://var/run/docker.sock')
        DOCKER_CLIENT.images.get(image_name)
        return True
    except (docker.errors.ImageNotFound, docker.errors.APIError) as err:
        logger.log_warning("Failed to get image '{}'. Error: '{}'".format(image_name, err))
        return False

class ServiceChecker(HealthChecker):
    """
    Checker that checks critical system service status via monit service.
    """

    # Cache file to save container_critical_processes
    CRITICAL_PROCESS_CACHE = '/tmp/critical_process_cache'

    CRITICAL_PROCESSES_PATH = 'etc/supervisor/critical_processes'

    # Command to get merged directory of a container
    GET_CONTAINER_FOLDER_CMD = 'docker inspect {} --format "{{{{.GraphDriver.Data.MergedDir}}}}"'

    # Command to query the status of monit service.
    CHECK_MONIT_SERVICE_CMD = 'systemctl is-active monit.service'

    # Command to get summary of critical system service.
    CHECK_CMD = 'monit summary -B'
    MIN_CHECK_CMD_LINES = 3

    # Expect status for all system service categories.
    # Monit 5.34.3+ (Debian 13) uses 'OK' for all service types
    EXPECTED_STATUS = 'OK'

    def __init__(self):
        HealthChecker.__init__(self)
        self.container_critical_processes = {}
        # Containers that has invalid critical_processes file
        self.bad_containers = set()

        self.container_feature_dict = {}

        self.need_save_cache = False

        self.config_db = None

        self.load_critical_process_cache()

    def get_expected_running_containers(self, feature_table):
        """Get a set of containers that are expected to running on SONiC

        Args:
            feature_table (object): FEATURE table in CONFIG_DB

        Returns:
            expected_running_containers: A set of container names that are expected running
            container_feature_dict: A dictionary {<container_name>:<feature_name>}
        """
        expected_running_containers = set()
        container_feature_dict = {}

        # Get current asic presence list. For multi_asic system, multi instance containers
        # should be checked only for asics present.
        asics_id_presence = multi_asic.get_asic_presence_list()

        # Some services may run all the instances irrespective of asic presence.
        # Add those to exception list.
        # database service: Currently services have dependency on all database services to
        # be up irrespective of asic presence.
        # bgp service: Currently bgp runs all instances. Once this is fixed to be config driven,
        # it will be removed from exception list.
        run_all_instance_list = ['database', 'bgp']

        container_list = []
        for container_name in feature_table.keys():
            # skip frr_bmp since it's not container just bmp option used by bgpd
            if container_name == "frr_bmp":
                continue
            # slim image does not have telemetry container and corresponding docker image
            if container_name == "telemetry":
                ret = check_docker_image("docker-sonic-telemetry")
                if not ret:
                    # If telemetry container image is not present, check gnmi container image
                    # If gnmi container image is not present, ignore telemetry container check
                    # if gnmi container image is present, check gnmi container instead of telemetry
                    ret = check_docker_image("docker-sonic-gnmi")
                    if not ret:
                        logger.log_debug("Ignoring telemetry container check on image which has no corresponding docker image")
                    else:
                        container_list.append("gnmi")
                    continue
            container_list.append(container_name)

        for container_name in container_list:
            feature_entry = feature_table[container_name]
            if feature_entry["state"] not in ["disabled", "always_disabled"]:
                if multi_asic.is_multi_asic():
                    if feature_entry.get("has_global_scope", "True") == "True":
                        expected_running_containers.add(container_name)
                        container_feature_dict[container_name] = container_name
                    if feature_entry.get("has_per_asic_scope", "False") == "True":
                        num_asics = multi_asic.get_num_asics()
                        for asic_id in range(num_asics):
                            if asic_id in asics_id_presence or container_name in run_all_instance_list:
                                expected_running_containers.add(container_name + str(asic_id))
                                container_feature_dict[container_name + str(asic_id)] = container_name
                else:
                    expected_running_containers.add(container_name)
                    container_feature_dict[container_name] = container_name
                    
        if device_info.is_supervisor() or device_info.is_disaggregated_chassis():
            expected_running_containers.add("database-chassis")
            container_feature_dict["database-chassis"] = "database"
        return expected_running_containers, container_feature_dict

    def get_current_running_containers(self):
        """Get current running containers, if the running container is not in self.container_critical_processes,
           try get the critical process list

        Returns:
            running_containers: A set of running container names
        """
        DOCKER_CLIENT = docker.DockerClient(base_url='unix://var/run/docker.sock')
        running_containers = set()
        ctrs = DOCKER_CLIENT.containers
        try:
            lst = ctrs.list(filters={"status": "running"})

            for ctr in lst:
                running_containers.add(ctr.name)
                if ctr.name not in self.container_critical_processes:
                    self.fill_critical_process_by_container(ctr.name)
        except docker.errors.APIError as err:
            logger.log_error("Failed to retrieve the running container list. Error: '{}'".format(err))

        return running_containers

    def get_critical_process_list_from_file(self, container, critical_processes_file):
        """Read critical process name list from critical processes file

        Args:
            container (str): contianer name
            critical_processes_file (str): critical processes file path

        Returns:
            critical_process_list: A list of critical process names
        """
        critical_process_list = []

        with open(critical_processes_file, 'r') as file:
            for line in file:
                # Try to match a line like "program:<process_name>"
                match = re.match(r"^\s*((.+):(.*))*\s*$", line)
                if match is None:
                    if container not in self.bad_containers:
                        self.bad_containers.add(container)
                        logger.log_error('Invalid syntax in critical_processes file of {}'.format(container))
                    continue
                if match.group(1) is not None:
                    identifier_key = match.group(2).strip()
                    identifier_value = match.group(3).strip()
                    if identifier_key == "program" and identifier_value:
                        critical_process_list.append(identifier_value)

        return critical_process_list

    def fill_critical_process_by_container(self, container):
        """Get critical process for a given container

        Args:
            container (str): container name
        """
        # Get container volumn folder
        container_folder = self._get_container_folder(container)
        if not container_folder:
            logger.log_warning('Could not find MergedDir of container {}, was container stopped?'.format(container))
            return

        if not os.path.exists(container_folder):
            logger.log_warning('MergedDir {} of container {} not found in filesystem, was container stopped?'.format(container_folder, container))
            return

        # Get critical_processes file path
        critical_processes_file = os.path.join(container_folder, ServiceChecker.CRITICAL_PROCESSES_PATH)
        if not os.path.isfile(critical_processes_file):
            # Critical process file does not exist, the container has no critical processes.
            logger.log_debug('Failed to get critical process file for {}, {} does not exist'.format(container, critical_processes_file))
            self._update_container_critical_processes(container, [])
            return

        # Get critical process list from critical_processes
        critical_process_list = self.get_critical_process_list_from_file(container, critical_processes_file)
        self._update_container_critical_processes(container, critical_process_list)

    def _update_container_critical_processes(self, container, critical_process_list):
        self.container_critical_processes[container] = critical_process_list
        self.need_save_cache = True

    def _get_container_folder(self, container):
        container_folder = utils.run_command(ServiceChecker.GET_CONTAINER_FOLDER_CMD.format(container))
        if container_folder is None:
            return container_folder

        return container_folder.strip()

    def save_critical_process_cache(self):
        """Save self.container_critical_processes to a cache file
        """
        if not self.need_save_cache:
            return

        self.need_save_cache = False
        if not self.container_critical_processes:
            # if container_critical_processes is empty, don't save it
            return

        if os.path.exists(ServiceChecker.CRITICAL_PROCESS_CACHE):
            # if cache file exists, remove it
            os.remove(ServiceChecker.CRITICAL_PROCESS_CACHE)

        with open(ServiceChecker.CRITICAL_PROCESS_CACHE, 'wb+') as f:
            pickle.dump(self.container_critical_processes, f)

    def load_critical_process_cache(self):
        if not os.path.isfile(ServiceChecker.CRITICAL_PROCESS_CACHE):
            # cache file does not exist
            return

        with open(ServiceChecker.CRITICAL_PROCESS_CACHE, 'rb') as f:
            self.container_critical_processes = pickle.load(f)

    def reset(self):
        self._info = {}

    def get_category(self):
        return 'Services'

    def check_by_monit(self, config):
        """
        et and analyze the output of $CHECK_CMD, collect status for file system or customize checker if any.
        :param config: Health checker configuration.
        :return:
        """
        output = utils.run_command(ServiceChecker.CHECK_MONIT_SERVICE_CMD)
        if not output or output.strip() != 'active':
            self.set_object_not_ok('Service', 'monit', 'monit service is not running')
            return

        output = utils.run_command(ServiceChecker.CHECK_CMD)
        lines = output.splitlines()
        if not lines or len(lines) < ServiceChecker.MIN_CHECK_CMD_LINES:
            self.set_object_not_ok('Service', 'monit', 'monit service is not ready')
            return

        status_begin = lines[1].find('Status')
        type_begin = lines[1].find('Type')
        if status_begin < 0 or type_begin < 0:
            self.set_object_not_ok('Service', 'monit', 'output of \"monit summary -B\" is invalid or incompatible')
            return

        for line in lines[2:]:
            name = line[0:status_begin].strip()
            if config and config.ignore_services and name in config.ignore_services:
                continue
            status = line[status_begin:type_begin].strip()
            service_type = line[type_begin:].strip()
            if status != ServiceChecker.EXPECTED_STATUS:
                self.set_object_not_ok(service_type, name, '{} status is {}, expected {}'.format(name, status, ServiceChecker.EXPECTED_STATUS))
            else:
                self.set_object_ok(service_type, name)
        return

    def check_services(self, config):
        """Check status of critical services and critical processes

        Args:
            config (config.Config): Health checker configuration.
        """
        if not self.config_db:
            self.config_db = swsscommon.ConfigDBConnector(use_unix_socket_path=True)
            self.config_db.connect()
        feature_table = self.config_db.get_table("FEATURE")
        expected_running_containers, self.container_feature_dict = self.get_expected_running_containers(feature_table)
        current_running_containers = self.get_current_running_containers()

        newly_disabled_containers = set(self.container_critical_processes.keys()).difference(expected_running_containers)
        for newly_disabled_container in newly_disabled_containers:
            self.container_critical_processes.pop(newly_disabled_container)

        self.save_critical_process_cache()

        not_running_containers = expected_running_containers.difference(current_running_containers)
        for container in not_running_containers:
            self.set_object_not_ok('Service', container, "Container '{}' is not running".format(container))

        if not self.container_critical_processes:
            # Critical process is empty, not expect
            self.set_object_not_ok('Service', 'system', 'no critical process found')
            return

        for container, critical_process_list in self.container_critical_processes.items():
            self.check_process_existence(container, critical_process_list, config, feature_table)

        for bad_container in self.bad_containers:
            self.set_object_not_ok('Service', bad_container, 'Syntax of critical_processes file is incorrect')

    def check(self, config):
        """Check critical system service status.

        Args:
            config (object): Health checker configuration.
        """
        self.reset()
        self.check_by_monit(config)
        self.check_services(config)

    def _parse_supervisorctl_status(self, process_status):
        """Expected input:
            arp_update                       RUNNING   pid 67, uptime 1:03:56
            buffermgrd                       RUNNING   pid 81, uptime 1:03:56

        Args:
            process_status (list): List of process status
        """
        data = {}
        for line in process_status:
            line = line.strip()
            if not line:
                continue
            items = line.split()
            if len(items) < 2:
                continue
            data[items[0].strip()] = items[1].strip()
        return data

    def publish_events(self, container_name, critical_process_list):
        params = swsscommon.FieldValueMap()
        params["ctr_name"] = container_name
        events_handle = swsscommon.events_init_publisher(EVENTS_PUBLISHER_SOURCE)
        for process_name in critical_process_list:
            params["process_name"] = process_name
            swsscommon.event_publish(events_handle, EVENTS_PUBLISHER_TAG, params)
        swsscommon.events_deinit_publisher(events_handle)

    def check_process_existence(self, container_name, critical_process_list, config, feature_table):
        """Check whether the process in the specified container is running or not.

        Args:
            container_name (str): Container name
            critical_process_list (list): Critical processes
            config (object): Health checker configuration.
            feature_table (object): Feature table
        """
        feature_name = self.container_feature_dict[container_name]
        if feature_name in feature_table:
            # We look into the 'FEATURE' table to verify whether the container is disabled or not.
            # If the container is diabled, we exit.
            if ("state" in feature_table[feature_name]
                    and feature_table[feature_name]["state"] not in ["disabled", "always_disabled"]):

                # We are using supervisorctl status to check the critical process status. We cannot leverage psutil here because
                # it not always possible to get process cmdline in supervisor.conf. E.g, cmdline of orchagent is "/usr/bin/orchagent",
                # however, in supervisor.conf it is "/usr/bin/orchagent.sh"
                cmd = 'docker exec {} bash -c "supervisorctl status"'.format(container_name)
                process_status = utils.run_command(cmd)
                if process_status is None:
                    for process_name in critical_process_list:
                        self.set_object_not_ok('Process', '{}:{}'.format(container_name, process_name), "Process '{}' in container '{}' is not running".format(process_name, container_name))
                    self.publish_events(container_name, critical_process_list)
                    return

                process_status = self._parse_supervisorctl_status(process_status.strip().splitlines())
                for process_name in critical_process_list:
                    if config and config.ignore_services and process_name in config.ignore_services:
                        continue

                    # Sometimes process_name is in critical_processes file, but it is not in supervisor.conf, such process will not run in container.
                    # and it is safe to ignore such process. E.g, radv. So here we only check those processes which are in process_status.
                    if process_name in process_status:
                        if process_status[process_name] != 'RUNNING':
                            self.set_object_not_ok('Process', '{}:{}'.format(container_name, process_name), "Process '{}' in container '{}' is not running".format(process_name, container_name))
                        else:
                            self.set_object_ok('Process', '{}:{}'.format(container_name, process_name))
