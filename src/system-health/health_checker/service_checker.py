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
        return False

class ServiceChecker(HealthChecker):
    """
    Checker that checks critical system service status via monit service.
    """

    # Cache file to save container_critical_processes
    CRITICAL_PROCESS_CACHE = '/tmp/critical_process_cache'

    CRITICAL_PROCESSES_PATH = 'etc/supervisor/critical_processes'
    SUPERVISOR_CONF_DIR = 'etc/supervisor/conf.d'

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

    # Whitelist of containers which are managed by KubeSonic to bypass health checking entirely.
    # These containers will be excluded from both expected and running container sets.
    CONTAINER_K8S_WHITELIST = {'telemetry', 'acms', 'restapi'}

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
            # Skip containers in the whitelist
            if container_name in ServiceChecker.CONTAINER_K8S_WHITELIST:
                logger.log_debug("Skipping whitelisted kubesonic managed container '{}' from expected running check".format(container_name))
                continue
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
            # Some platforms may not include the OTEL container; skip expecting it when image absent
            if container_name == "otel":
                if not check_docker_image("docker-sonic-otel"):
                    logger.log_debug("Ignoring otel container check on image which has no corresponding docker image")
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
                # Check if this is a Kubernetes-managed container
                labels = ctr.labels or {}
                ns = labels.get("io.kubernetes.pod.namespace")
                if ns == "sonic":
                    continue
                # Skip kubesonic managed containers in the whitelist
                if ctr.name in ServiceChecker.CONTAINER_K8S_WHITELIST:
                    continue
                running_containers.add(ctr.name)
                if ctr.name not in self.container_critical_processes:
                    self.fill_critical_process_by_container(ctr.name)
        except docker.errors.APIError as err:
            logger.log_error("Failed to retrieve the running container list. Error: '{}'".format(err))

        return running_containers

    def _set_container_parse_error(self, container, message):
        """Log parse/config errors once per container to avoid repeated noise.

        Args:
            container (str): Container name.
            message (str): Error message to log.
        """
        if container not in self.bad_containers:
            self.bad_containers.add(container)
            logger.log_error(message)

    def _get_group_programs(self, supervisor_conf_dir, group_name):
        """Return unique program names for a supervisor group.

        Scan all .conf files under the container's supervisord conf.d directory,
        locate [group:<name>] sections, and parse their programs= entries.

        Args:
            supervisor_conf_dir (str): Absolute path to etc/supervisor/conf.d
                under the container merged directory.
            group_name (str): Supervisor group name to resolve.

        Returns:
            list[str]: Ordered unique program names from the requested group. Empty if
                group is not found or conf directory is unavailable.
        """
        if not supervisor_conf_dir or not os.path.isdir(supervisor_conf_dir):
            return []

        content = ''
        for file_name in sorted(os.listdir(supervisor_conf_dir)):
            if not file_name.endswith('.conf'):
                continue

            supervisor_conf_file = os.path.join(supervisor_conf_dir, file_name)
            if not os.path.isfile(supervisor_conf_file):
                continue

            with open(supervisor_conf_file, 'r') as file:
                content += file.read() + '\n'

        if not content:
            return []

        current_group = None
        group_programs = []
        for line in content.splitlines():
            stripped_line = line.strip()
            if not stripped_line or stripped_line.startswith('#') or stripped_line.startswith(';'):
                continue

            group_match = re.match(r'^\[group:(.+)\]$', stripped_line)
            if group_match is not None:
                current_group = group_match.group(1).strip()
                continue

            if stripped_line.startswith('['):
                current_group = None
                continue

            if current_group != group_name:
                continue

            programs_match = re.match(r'^programs\s*=\s*(.*)$', stripped_line)
            if programs_match is None:
                continue

            programs = [program.strip() for program in programs_match.group(1).split(',') if program.strip()]
            for program in programs:
                if program not in group_programs:
                    group_programs.append(program)

        return group_programs

    def get_critical_process_list_from_file(self, container, critical_processes_file, supervisor_conf_dir=None):
        """Read critical process name list from critical processes file

        Args:
            container (str): contianer name
            critical_processes_file (str): critical processes file path
            supervisor_conf_dir (str): supervisord conf.d directory path in merged container fs

        Returns:
            critical_process_list: A list of critical process names
        """
        critical_process_list = []

        with open(critical_processes_file, 'r') as file:
            for line in file:
                # Try to match a line like "program:<process_name>"
                match = re.match(r"^\s*((.+):(.*))*\s*$", line)
                if match is None:
                    self._set_container_parse_error(container, 'Invalid syntax in critical_processes file of {}'.format(container))
                    continue
                if match.group(1) is not None:
                    identifier_key = match.group(2).strip()
                    identifier_value = match.group(3).strip()
                    if identifier_key == "program" and identifier_value:
                        if identifier_value not in critical_process_list:
                            critical_process_list.append(identifier_value)
                    elif identifier_key == "group" and identifier_value:
                        programs = self._get_group_programs(supervisor_conf_dir, identifier_value)
                        if not programs:
                            self._set_container_parse_error(container, 'Group {} in critical_processes file of {} does not exist in supervisor config'.format(identifier_value, container))
                            continue
                        for program in programs:
                            if program not in critical_process_list:
                                critical_process_list.append(program)

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

        supervisor_conf_dir = os.path.join(container_folder, ServiceChecker.SUPERVISOR_CONF_DIR)

        # Get critical process list from critical_processes
        critical_process_list = self.get_critical_process_list_from_file(container, critical_processes_file, supervisor_conf_dir)
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

    def _get_matching_process_states(self, process_name, process_status):
        """Get all matching process states for a process name.

        Match both direct supervisor keys (<program>) and grouped keys
        (<group>:<program>) by suffix.

        Args:
            process_name (str): Process name from critical_processes.
            process_status (dict[str, str]): Parsed supervisorctl map
                {process_key: state}.

        Returns:
            list[str]: Matching states found in supervisorctl output.
        """
        matching_process_states = []

        if process_name in process_status:
            matching_process_states.append(process_status[process_name])

        grouped_process_suffix = ':{}'.format(process_name)
        for name, status in process_status.items():
            if name.endswith(grouped_process_suffix):
                matching_process_states.append(status)

        return matching_process_states

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
                process_status = utils.run_command(cmd, timeout=15)
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
                    # and it is safe to ignore such process. E.g, radv.
                    # Group entries can appear in supervisorctl as <group>:<program>.
                    matching_process_states = self._get_matching_process_states(process_name, process_status)
                    if matching_process_states:
                        if any(state != 'RUNNING' for state in matching_process_states):
                            self.set_object_not_ok('Process', '{}:{}'.format(container_name, process_name), "Process '{}' in container '{}' is not running".format(process_name, container_name))
                        else:
                            self.set_object_ok('Process', '{}:{}'.format(container_name, process_name))
