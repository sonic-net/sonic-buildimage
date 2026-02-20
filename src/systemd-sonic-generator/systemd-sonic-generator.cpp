#include <stdio.h>
#include <string_view>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <json-c/json.h>
#include <string>
#include <sstream>
#include <filesystem>
#include <format>
#include <unordered_set>
#include <fstream>
#include <unordered_map>
#include <regex>
#include <fcntl.h>
#include <stdarg.h>

// Utility function for logging to /dev/kmsg
void log_to_kmsg(const char* format, ...) {
    static int kmsg_fd = -1;
    
    // Open /dev/kmsg if not already opened
    if (kmsg_fd == -1) {
        kmsg_fd = open("/dev/kmsg", O_WRONLY);
        if (kmsg_fd == -1) {
            // Fallback to stderr if /dev/kmsg is not available
            va_list args;
            va_start(args, format);
            vfprintf(stderr, format, args);
            va_end(args);
            return;
        }
    }
    
    va_list args;
    va_start(args, format);
    
    // Format the message
    char kmsg_buffer[1024];
    char *buffer_p;
    buffer_p = stpncpy(kmsg_buffer, "<6>systemd-sonic-generator: ", sizeof(kmsg_buffer));
    vsnprintf(buffer_p, sizeof(kmsg_buffer) - (buffer_p - kmsg_buffer), format, args);
    va_end(args);
    kmsg_buffer[1023] = '\0';
    
    // Write to /dev/kmsg
    write(kmsg_fd, kmsg_buffer, strlen(kmsg_buffer));
}

#define MAX_NUM_TARGETS 48
#define MAX_NUM_INSTALL_LINES 48
#define MAX_NUM_UNITS 128
#define MAX_BUF_SIZE 512
#define MAX_PLATFORM_NAME_LEN 64



const char* LIB_SYSTEMD = "/usr/lib/systemd";
const char* ETC_SYSTEMD = "/etc/systemd";
const char* UNIT_FILE_PREFIX = "/usr/lib/systemd/system/";
const char* CONFIG_FILE = "/etc/sonic/generated_services.conf";
const char* MACHINE_CONF_FILE = "/host/machine.conf";
const char* ASIC_CONF_FORMAT = "/usr/share/sonic/device/%s/asic.conf";
const char* PLATFORM_FILE_FORMAT = "/usr/share/sonic/device/%s/platform.json";
const char* PLATFORM_CONF_FORMAT = "/usr/share/sonic/device/%s/services.conf";
const char* DPU_PREFIX = "dpu";


const char* g_lib_systemd = NULL;
const char* get_lib_systemd () {
    return (g_lib_systemd) ? g_lib_systemd : LIB_SYSTEMD;
}

const char* g_etc_systemd = NULL;
const char* get_etc_systemd () {
    return (g_etc_systemd) ? g_etc_systemd : ETC_SYSTEMD;
}

const char* g_unit_file_prefix = NULL;
const char* get_unit_file_prefix() {
    return (g_unit_file_prefix) ? g_unit_file_prefix : UNIT_FILE_PREFIX;
}

const char* g_config_file = NULL;
const char* get_config_file() {
    return (g_config_file) ? g_config_file : CONFIG_FILE;
}

const char* g_machine_config_file = NULL;
const char* get_machine_config_file() {
    return (g_machine_config_file) ? g_machine_config_file : MACHINE_CONF_FILE;
}

const char* g_asic_conf_format = NULL;
const char* get_asic_conf_format() {
    return (g_asic_conf_format) ? g_asic_conf_format : ASIC_CONF_FORMAT;
}

const char* g_platform_file_format = NULL;
const char* get_platform_file_format() {
    return (g_platform_file_format) ? g_platform_file_format : PLATFORM_FILE_FORMAT;
}

const char* g_platform_conf_format = NULL;
const char* get_platform_conf_format() {
    return (g_platform_conf_format) ? g_platform_conf_format : PLATFORM_CONF_FORMAT;
}

const char* get_platform();

static int num_asics;
static char** multi_instance_services;
static int num_multi_inst;
static bool smart_switch_npu;
static bool smart_switch_dpu;
static bool smart_switch;
static int num_dpus;
static char* platform = NULL;
static struct json_object *platform_info = NULL;


#ifdef _SSG_UNITTEST
/**
 * @brief Cleans up the cache by resetting cache pointers.
 */
void clean_up_cache() {
    platform = NULL;
    platform_info = NULL;
}
#endif

/**
 * Sets the value of a pointer to an invalid memory address.
 *
 * @param pointer A pointer to a pointer variable.
 */
static void set_invalid_pointer(void **pointer) {
    *pointer = (void *)-1;
}


/**
 * @brief Checks if a pointer is valid.
 *
 * This function checks if a pointer is valid by verifying that it is not NULL and not equal to (void *)-1.
 *
 * @param pointer The pointer to be checked.
 * @return true if the pointer is valid, false otherwise.
 */
static bool is_valid_pointer(void *pointer) {
    return pointer != NULL && pointer != (void *)-1;
}


/**
 * Checks if a pointer is initialized.
 *
 * @param pointer The pointer to check.
 * @return true if the pointer is not NULL, false otherwise.
 */
static bool is_initialized_pointer(void *pointer) {
    return pointer != NULL;
}

static void strip_trailing_newline(char* str) {
    /***
    Strips trailing newline from a string if it exists
    ***/

    if (str == NULL) {
        return;
    }
    size_t l = strlen(str);
    if (l > 0 && str[l-1] == '\n')
        str[l-1] = '\0';
}

static void strip_trailing_newline(std::string& str) {
    /***
    Strips trailing newline from a string if it exists
    ***/
    if (!str.empty() && str.back() == '\n') {
        str.pop_back();
    }
}


/**
 * Checks if the given path is "/dev/null".
 *
 * @param path The path to check.
 * @return true if the path is "/dev/null", false otherwise.
 */
static bool is_devnull(const char* path)
{
    char resolved_path[PATH_MAX];
    if (realpath(path, resolved_path) == NULL) {
        return false;
    }
    return strcmp(resolved_path, "/dev/null") == 0;
}


static int get_target_lines(const char* unit_file, char* target_lines[]) {
    /***
    Gets installation information for a given unit file

    Returns lines in the [Install] section of a unit file
    ***/
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    bool found_install;
    int num_target_lines;


    fp = fopen(unit_file, "r");

    if (fp == NULL) {
        log_to_kmsg("Failed to open file %s\n", unit_file);
        return -1;
    }

    found_install = false;
    num_target_lines = 0;

    while ((nread = getline(&line, &len, fp)) != -1 ) {
        // Assumes that [Install] is the last section of the unit file
        if (strstr(line, "[Install]") != NULL) {
             found_install = true;
        }
        else if (found_install) {
            if (num_target_lines >= MAX_NUM_INSTALL_LINES) {
                log_to_kmsg("Number of lines in [Install] section of %s exceeds MAX_NUM_INSTALL_LINES\n", unit_file);
                log_to_kmsg("Extra [Install] lines will be ignored\n");
                break;
            }
            target_lines[num_target_lines] = strdup(line);
            num_target_lines++;
        }
    }

    free(line);

    fclose(fp);

    return num_target_lines;
}

static bool is_multi_instance_service(std::string service_file, std::unordered_set<std::string> service_list=std::unordered_set<std::string>()){
    /*
        * The service name may contain @.service or .service. Remove these
        * postfixes and extract service name. Compare service name for absolute
        * match in multi_instance_services[].
        * This is to prevent services like database-chassis and systemd-timesyncd marked
        * as multi instance services as they contain strings 'database' and 'syncd' respectively
        * which are multi instance services in multi_instance_services[].
        */    
    std::string delimiter;
    if (service_file.find("@") != std::string::npos) {
        delimiter = "@";
    } else {
        delimiter = ".";
    }
    std::string service_name = service_file.substr(0, service_file.find(delimiter));

    if (service_list.empty()) {
        for(int i=0; i < num_multi_inst; i++){

            if (service_name == multi_instance_services[i]) {
                return true;
            }
        }
    } else {
        if (service_list.count(service_name) > 0) {
            return true;
        }
    }
    return false;

}


/**
 * Checks if a service is a multi-instance service for DPU.
 *
 * @param service_name The name of the service to check.
 * @return true if the service is a multi-instance service for DPU, false otherwise.
 */
static bool is_multi_instance_service_for_dpu(const std::string& service_name) {
    if (!smart_switch_npu) {
        return false;
    }

    std::unordered_set<std::string> multi_instance_services_for_dpu = {"database", "dash-ha"};
    return is_multi_instance_service(service_name, multi_instance_services_for_dpu);
}


static int get_install_targets_from_line(std::string target_string, std::string install_type, char* targets[], int existing_targets) {
    /***
    Helper fuction for get_install_targets

    Given a space delimited string of target directories and a suffix,
    puts each target directory plus the suffix into the targets array
    ***/
    std::string target;
    int num_targets = 0;

    if (target_string.empty() || install_type.empty()) {
        log_to_kmsg("Invalid target string or install type\n");
        exit(EXIT_FAILURE);
    }

    std::stringstream ss(target_string);

    while (ss >> target) {
        if (num_targets + existing_targets >= MAX_NUM_TARGETS) {
            log_to_kmsg("Number of targets exceeds MAX_NUM_TARGETS\n");
            log_to_kmsg("Additional targets will be ignored\n");
            break;
        }
        // handle install targets using the '%i' systemd specifier
        if (target.find("%") != std::string::npos) {
            target = target.substr(0, target.find("%")) + target.substr(target.find("."));
        }
        strip_trailing_newline(target);
        target += install_type;
        targets[num_targets + existing_targets] = (char*) calloc(target.length() + 1, sizeof(char));
        snprintf(targets[num_targets + existing_targets], PATH_MAX, "%s", target.c_str());
        num_targets++;
    }
    return num_targets;
}

static void replace_multi_inst_dep(const std::filesystem::path& install_dir, const std::string& unit_file) {
    std::ifstream fp_src;
    std::ofstream fp_tmp;
    std::string line;
    int i;
    bool section_done = false;

    /* Assumes that the service files has 3 sections,
     * in the order: Unit, Service and Install.
     * Assumes that the timer file has 3 sections,
     * in the order: Unit, Timer and Install.
     * Read service dependency from Unit and Install
     * sections, replace if dependent on multi instance
     * service.
     */
    {
        fp_src = std::ifstream(get_unit_file_prefix() + unit_file);
        fp_tmp = std::ofstream(install_dir / unit_file);

        while (std::getline(fp_src, line)) {
            const auto kv_split = line.find("=");
            std::string_view line_view = line;
            auto key = line_view.substr(0, kv_split);
            // Ignore all dependency-related keys for later handling
            if (key != "After" && key != "Before"
                    && key != "Requires" && key != "Wants"
                    && key != "Requisite" && key != "WantedBy") {
                fp_tmp << line << "\n";
            }
        }
    }

    std::filesystem::create_directory(install_dir / (unit_file + ".d"));

    {
        fp_src = std::ifstream(get_unit_file_prefix() + unit_file);
        fp_tmp = std::ofstream(install_dir / (unit_file + ".d") / "multi-asic-dependencies.conf");
        fp_tmp << "[Unit]\n";

        while (std::getline(fp_src, line)) {
            if (line.find("[Service]") != std::string::npos ||
                    line.find("[Timer]") != std::string::npos) {
                section_done = true;
            } else if (line.find("[Install]") != std::string::npos) {
                section_done = false;
                fp_tmp << "[Install]\n";
                fp_tmp << "WantedBy=\n";
            } else if (line.find("[Unit]") != std::string::npos ||
                    line.find("Description") != std::string::npos ||
                    (section_done == true)) {
            } else {
                const auto kv_split = line.find("=");
                std::string_view line_view = line;
                auto key = line_view.substr(0, kv_split);
                if (key != "After" && key != "Before"
                        && key != "Requires" && key != "Wants"
                        && key != "Requisite" && key != "WantedBy") {
                    // Ignore keys that aren't going to need special handling for multi-instance
                    // services.
                    continue;
                }
                auto values = line_view.substr(kv_split + 1);
                std::string_view value;
                std::string_view::size_type value_idx = values.find(" ");
                std::string_view::size_type old_value_idx = 0;
                value = values.substr(0, value_idx);
                do {
                    if (value.length() == 0) {
                        old_value_idx = value_idx;
                        value_idx = values.find(" ", old_value_idx + 1);
                        value = values.substr(old_value_idx + 1, value_idx - old_value_idx - 1);
                        continue;
                    }
                    if((value.find('.') == std::string_view::npos) ||
                            (value.find('@') != std::string_view::npos)) {
                        // If the value doesn't have a suffix, or it is already an instantiation
                        // of a unit, ignore it
                        fp_tmp << key << "=" << value << "\n";
                    } else {
                        auto extension_idx = value.find(".");
                        auto service_name = value.substr(0, extension_idx);
                        auto type = value.substr(extension_idx + 1);
                        if (num_asics > 1 && is_multi_instance_service(std::string(value))) {
                            for(i = 0; i < num_asics; i++) {
                                fp_tmp << key << "=" << service_name << "@" << i << "." << type << "\n";
                            }
                        } else if (smart_switch_npu && is_multi_instance_service_for_dpu(std::string(value))) {
                            for(i = 0; i < num_dpus; i++) {
                                fp_tmp << key << "=" << service_name << "@" << DPU_PREFIX << i << "." << type << "\n";
                            }
                        } else {
                            fp_tmp << key << "=" << service_name << "." << type << "\n";
                        }
                    }
                    old_value_idx = value_idx;
                    if (value_idx != std::string_view::npos) {
                        value_idx = values.find(" ", old_value_idx + 1);
                        value = values.substr(old_value_idx + 1, value_idx - old_value_idx - 1);
                    }
                } while (old_value_idx != std::string_view::npos);
            }
        }
    }
}

static void update_environment(const std::filesystem::path& install_dir, const std::string &unit_file_name)
{
    if (!unit_file_name.ends_with(".service")) {
        return;
    }

    auto unit_override_dir = install_dir / (unit_file_name + ".d");
    std::filesystem::create_directory(unit_override_dir);

    auto unit_environment_file_path = unit_override_dir / "environment.conf";
    std::ofstream unit_environment_file(unit_environment_file_path);

    std::unordered_map<std::string, std::string> env_vars;
    env_vars["IS_DPU_DEVICE"] = (smart_switch_dpu ? "true" : "false");
    env_vars["NUM_DPU"] = std::to_string(num_dpus);

    unit_environment_file << "[Service]\n";

    for (const auto& [key, value] : env_vars) {
        unit_environment_file << "Environment=\"" << key << "=" << value << "\"" << std::endl;
    }

    unit_environment_file.close();
}

int get_install_targets(std::string unit_file, char* targets[]) {
    /***
    Returns install targets for a unit file

    Parses the information in the [Install] section of a given
    unit file to determine which directories to install the unit in
    ***/
    std::string file_path;
    char *target_lines[MAX_NUM_INSTALL_LINES];
    int num_target_lines;
    int num_targets;
    int found_targets;
    char* token;
    char* line = NULL;
    bool first;
    std::string target_suffix;
    std::string instance_name;

    file_path = get_unit_file_prefix() + unit_file;

    instance_name = unit_file.substr(0, unit_file.find('.'));

    num_target_lines = get_target_lines(file_path.c_str(), target_lines);
    if (num_target_lines < 0) {
        log_to_kmsg("Error parsing targets for %s\n", unit_file.c_str());
        return -1;
    }

    num_targets = 0;

    for (int i = 0; i < num_target_lines; i++) {
        line = target_lines[i];
        first = true;

        while ((token = strtok_r(line, "=", &line))) {
            if (first) {
                first = false;

                if (strstr(token, "RequiredBy") != NULL) {
                    target_suffix = ".requires";
                }
                else if (strstr(token, "WantedBy") != NULL) {
                    target_suffix = ".wants";
                } else {
                    break;
                }
            }
            else {
                found_targets = get_install_targets_from_line(token, target_suffix, targets, num_targets);
                num_targets += found_targets;
            }
        }
        free(target_lines[i]);
    }
    return num_targets;
}


int get_unit_files(const char* config_file, char* unit_files[], int unit_files_size) {
    /***
    Reads a list of unit files to be installed from config_file
    ***/
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char *pos;

    fp = fopen(config_file, "r");

    if (fp == NULL) {
        log_to_kmsg("Failed to open %s\n", config_file);
        exit(EXIT_FAILURE);
    }

    int num_unit_files = 0;

    if (!multi_instance_services) {
        num_multi_inst = 0;
        multi_instance_services = (char**) calloc(MAX_NUM_UNITS, sizeof(char *));
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        if (num_unit_files >= unit_files_size) {
            log_to_kmsg("Maximum number of units exceeded, ignoring extras\n");
            break;
        }
        strip_trailing_newline(line);

        /* Get the multi-instance services */
        pos = strchr(line, '@');
        if (pos != NULL) {
            multi_instance_services[num_multi_inst] = (char*) calloc(pos-line+1, sizeof(char));
            snprintf(multi_instance_services[num_multi_inst], pos-line+1, "%s", line);
            num_multi_inst++;
        }

        /* topology service to be started only for multiasic VS platform */
        if ((strcmp(line, "topology.service") == 0) &&
                        (num_asics == 1)) {
            continue;
        }

        unit_files[num_unit_files] = strdup(line);
        num_unit_files++;
    }

    free(line);

    fclose(fp);

    return num_unit_files;
}

int get_platform_unit_files(char* unit_files[], int unit_files_size)
{
    const char* platform = get_platform();
    if (!platform) {
        return 0;
    }

    char config_file[PATH_MAX];
    snprintf(config_file, PATH_MAX, get_platform_conf_format(), platform);

    if (access(config_file, R_OK) != 0) {
        return 0;
    }

    return get_unit_files(config_file, unit_files, unit_files_size);
}


std::string insert_instance_number(const std::string& unit_file, int instance, const std::string& instance_prefix) {
    /***
    Adds an instance number to a systemd template name

    E.g. given unit_file='example@.service', instance=3,
    returns a pointer to 'example@3.service'
    ***/
    size_t at_pos = unit_file.find("@");
    if (at_pos == std::string::npos) {
        log_to_kmsg("Invalid unit file %s for instance %d\n", unit_file.c_str(), instance);
        return "";
    }

    return unit_file.substr(0, at_pos + 1) + instance_prefix + std::to_string(instance) + unit_file.substr(at_pos + 1);
}


static int create_symlink(const std::string& unit, const std::string& target, const std::string& install_dir, int instance,  const std::string& instance_prefix) {
    struct stat st;
    std::string src_path;
    std::string dest_path;
    std::string final_install_dir;
    std::string unit_instance;
    int r;

    src_path = get_unit_file_prefix() + unit;

    if (instance < 0) {
        unit_instance = unit;
    }
    else {
        unit_instance = insert_instance_number(unit, instance, instance_prefix);
    }

    final_install_dir = install_dir + std::string(target);
    dest_path = final_install_dir + "/" + unit_instance;

    if (stat(final_install_dir.c_str(), &st) == -1) {
        // If doesn't exist, create
        r = mkdir(final_install_dir.c_str(), 0755);
        if (r == -1) {
            log_to_kmsg("Unable to create target directory %s\n", final_install_dir.c_str());
            return -1;
        }
    }
    else if (S_ISREG(st.st_mode)) {
        // If is regular file, remove and create
        r = remove(final_install_dir.c_str());
        if (r == -1) {
            log_to_kmsg("Unable to remove file with same name as target directory %s\n", final_install_dir.c_str());
            return -1;
        }

        r = mkdir(final_install_dir.c_str(), 0755);
        if (r == -1) {
            log_to_kmsg("Unable to create target directory %s\n", final_install_dir.c_str());
            return -1;
        }
    }
    else if (S_ISDIR(st.st_mode)) {
        // If directory, verify correct permissions
        r = chmod(final_install_dir.c_str(), 0755);
        if (r == -1) {
            log_to_kmsg("Unable to change permissions of existing target directory %s\n", final_install_dir.c_str());
            return -1;
        }
    }

    if (is_devnull(dest_path.c_str())) {
        if (remove(dest_path.c_str()) != 0) {
            log_to_kmsg("Unable to remove existing symlink %s\n", dest_path.c_str());
            return -1;
        }
    }

    r = symlink(src_path.c_str(), dest_path.c_str());

    if (r < 0) {
        if (errno == EEXIST)
            return 0;
        log_to_kmsg("Error creating symlink %s from source %s\n", dest_path.c_str(), src_path.c_str());
        return -1;
    }

    return 0;

}


static int install_unit_file(std::string unit_file, std::string target, std::string install_dir) {
    /***
    Creates a symlink for a unit file installation

    For a given unit file and target directory,
    create the appropriate symlink in the target directory
    to enable the unit and have it started by Systemd

    If a multi ASIC platform is detected, enables multi-instance
    services as well
    ***/
    std::string target_instance;
    int r;

    if (unit_file.empty() || target.empty() || install_dir.empty()){
        log_to_kmsg("Invalid unit file, target or install directory\n");
        exit(EXIT_FAILURE);
    }

    if ((num_asics > 1) && unit_file.find("@") != std::string::npos) {

        for (int i = 0; i < num_asics; i++) {

            if (target.find("@") != std::string::npos) {
                target_instance = insert_instance_number(target, i, "");
            }
            else {
                target_instance = target;
            }

            r = create_symlink(unit_file, target_instance, install_dir, i, "");
            if (r < 0)
                log_to_kmsg("Error installing %s for target %s\n", unit_file.c_str(), target_instance.c_str());
        }
    } else if (num_dpus > 0 && unit_file.find("@") != std::string::npos) {
        // If multi-instance service for DPU
        // Install each DPU units to the host main instance only,
        // E.g. install database@dpu0.service, database@dpu1.service to multi-user.target.wants
        // We don't have case like to install xxx@dpu0.service to swss@dpu0.service.wants
        for (int i = 0; i < num_dpus; i++) {
            r = create_symlink(unit_file, target, install_dir, i, DPU_PREFIX);
            if (r < 0)
                log_to_kmsg("Error installing %s for target %s\n", unit_file.c_str(), target.c_str());
        }
    } else {
        r = create_symlink(unit_file, target, install_dir, -1, "");
        if (r < 0)
            log_to_kmsg("Error installing %s for target %s\n", unit_file.c_str(), target.c_str());
    }

    return 0;
}


/**
 * Retrieves the platform name from the machine configuration file.
 * If the platform name is already cached, it returns the cached value.
 * If the platform name is not found in the configuration file, it sets the platform pointer to NULL.
 * 
 * @return The platform name if found, otherwise NULL.
 */
const char* get_platform() {
    if (is_initialized_pointer(platform)) {
        if (is_valid_pointer(platform)) {
            return platform;
        } else {
            return NULL;
        }
    }

    FILE* fp;
    char* line = NULL;
    char* saveptr;
    char *tmp_platform = NULL;
    static char platform_buffer[MAX_PLATFORM_NAME_LEN + 1];
    size_t len = 0;
    ssize_t nread;
    const char* machine_config_file = get_machine_config_file();
    fp = fopen(machine_config_file, "r");
    if (fp == NULL) {
        log_to_kmsg("Failed to open %s\n", machine_config_file);
        exit(EXIT_FAILURE);
    }

    while ((nread = getline(&line, &len, fp)) != -1) {
        if ((strstr(line, "onie_platform") != NULL) ||
            (strstr(line, "aboot_platform") != NULL)) {
            strtok_r(line, "=", &saveptr);
            tmp_platform = strtok_r(NULL, "=", &saveptr);
            strip_trailing_newline(tmp_platform);
            break;
        }
    }
    if (tmp_platform == NULL) {
        set_invalid_pointer((void **)&platform);
        fclose(fp);
        free(line);
        return NULL;
    }
    snprintf(platform_buffer, sizeof(platform_buffer), "%s", tmp_platform);
    fclose(fp);
    free(line);

    platform = platform_buffer;
    return platform;
}


int get_num_of_asic() {
    /***
    Determines if the current platform is single or multi-ASIC
    ***/
    FILE *fp;
    char *line = NULL;
    const char* platform = NULL;
    char* saveptr;
    size_t len = 0;
    ssize_t nread;
    char asic_file[512];
    char* str_num_asic;
    int num_asic = 1;

    platform = get_platform();

    if(platform != NULL) {
        snprintf(asic_file, 512, get_asic_conf_format(), platform);
        fp = fopen(asic_file, "r");
        if (fp != NULL) {
            while ((nread = getline(&line, &len, fp)) != -1) {
                if (strstr(line, "NUM_ASIC") != NULL) {
                    strtok_r(line, "=", &saveptr);
                    str_num_asic = strtok_r(NULL, "=", &saveptr);
                    strip_trailing_newline(str_num_asic);
                    if (str_num_asic != NULL){
                        num_asic = strtol(str_num_asic, NULL, 10);
                    }
                    break;
                }
            }
            fclose(fp);
            free(line);
        }
    }
    return num_asic;

}


/**
 * Retrieves the platform information.
 * 
 * This function reads the platform information from a JSON file and returns it as a JSON object.
 * If the platform information has already been retrieved, it returns the cached value.
 * 
 * @return The platform information as a JSON object, or NULL if it fails to retrieve or parse the information.
 */
const struct json_object* get_platform_info() {
    if (is_initialized_pointer(platform_info)) {
        if (is_valid_pointer(platform_info)) {
            return platform_info;
        } else {
            return NULL;
        }
    }

    char platform_file_path[PATH_MAX];
    const char* platform = get_platform();
    if (platform == NULL) {
        set_invalid_pointer((void **)&platform_info);
        return NULL;
    }
    snprintf(platform_file_path, sizeof(platform_file_path), get_platform_file_format(), platform);

    FILE *fp = fopen(platform_file_path, "r");
    if (fp == NULL) {
        log_to_kmsg("Failed to open %s\n", platform_file_path);
        set_invalid_pointer((void **)&platform_info);
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        log_to_kmsg("Failed to seek to end of %s\n", platform_file_path);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    size_t fsize = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) != 0) {
        log_to_kmsg("Failed to seek to beginning of %s\n", platform_file_path);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    char *platform_json = (char*) malloc(fsize + 1);
    if (platform_json == NULL) {
        log_to_kmsg("Failed to allocate memory for %s\n", platform_file_path);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    if (fread(platform_json, fsize, 1, fp) != 1) {
        log_to_kmsg("Failed to read %s\n", platform_file_path);
        free(platform_json);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    fclose(fp);
    platform_json[fsize] = '\0';

    platform_info = json_tokener_parse(platform_json);
    if (platform_info == NULL) {
        log_to_kmsg("Failed to parse %s\n", platform_file_path);
        free(platform_json);
        return NULL;
    }
    free(platform_json);
    return platform_info;
}


/**
 * Checks if the platform is a smart switch with an NPU (Network Processing Unit).
 * 
 * @return true if the platform is a smart switch with an NPU, false otherwise.
 */
static bool is_smart_switch_npu() {
    struct json_object *dpus;
    const struct json_object *platform_info = get_platform_info();
    if (platform_info == NULL) {
        return false;
    }
    return json_object_object_get_ex(platform_info, "DPUS", &dpus);
}


/**
 * Checks if the current platform is a smart switch with a DPU (Data Processing Unit).
 * 
 * @return true if the platform is a smart switch with a DPU, false otherwise.
 */
static bool is_smart_switch_dpu() {
    struct json_object *dpu;
    const struct json_object *platform_info = get_platform_info();
    if (platform_info == NULL) {
        return false;
    }
    return json_object_object_get_ex(platform_info, "DPU", &dpu);
}


/**
 * @brief Retrieves the number of DPUs (Data Processing Units).
 *
 * This function retrieves the number of DPUs by accessing the platform information
 * and extracting the "DPUS" array from it. If the platform information is not available
 * or the "DPUS" array does not exist, the function returns 0.
 *
 * @return The number of DPUs.
 */
static int get_num_of_dpu() {
    struct json_object *dpus;
    const struct json_object *platform_info = get_platform_info();
    if (platform_info == NULL) {
        return 0;
    }
    if (!json_object_object_get_ex(platform_info, "DPUS", &dpus)) {
        return 0;
    }
    size_t num_dpu = 0;
    json_object_object_foreach(dpus, key, val) {
        num_dpu++;
    }
    return num_dpu;
}


static int render_network_service_for_smart_switch(const std::filesystem::path& install_dir) {
    if (!smart_switch_npu) {
        return 0;
    }

    // Render Before instruction for midplane network with database service
    for (int i = 0; i < num_dpus; i++) {
        auto unit_override_dir = install_dir / std::format("database@dpu{}.service.d", i);
        std::filesystem::create_directory(unit_override_dir);

        auto unit_ordering_file_path = unit_override_dir / "ordering.conf";

        std::ofstream unit_ordering_file;
        unit_ordering_file.open(unit_ordering_file_path);
        unit_ordering_file << "[Unit]\n";
        unit_ordering_file << "Requires=systemd-networkd-wait-online@bridge-midplane.service\n";
        unit_ordering_file << "After=systemd-networkd-wait-online@bridge-midplane.service\n";
    }

    return 0;
}


static int mask_networkd_for_non_smart_switch(const std::filesystem::path& install_dir) {
    if (smart_switch) {
        return 0;
    }

    auto service_path = install_dir / "systemd-networkd.service";

    int r = symlink("/dev/null", service_path.c_str());

    if (r < 0) {
        if (errno == EEXIST)
            return 0;
        log_to_kmsg("Error masking %s: %s\n", service_path.c_str(), strerror(errno));
        return -1;
    }

    return 0;
}


int ssg_main(int argc, char **argv) {
    char* unit_files[MAX_NUM_UNITS];
    std::string install_dir;
    char* targets[MAX_NUM_TARGETS];
    std::string unit_instance;
    std::string prefix;
    std::string suffix;
    int num_unit_files;
    int num_targets;

#ifdef _SSG_UNITTEST
    clean_up_cache();
#endif

    if (argc <= 1) {
        log_to_kmsg("Installation directory required as argument\n");
        return 1;
    }

    num_asics = get_num_of_asic();
    smart_switch_npu = is_smart_switch_npu();
    smart_switch_dpu = is_smart_switch_dpu();
    smart_switch = smart_switch_npu || smart_switch_dpu;
    num_dpus = get_num_of_dpu();

    install_dir = std::string(argv[1]) + "/";
    const char* config_file = get_config_file();
    num_unit_files = get_unit_files(config_file, unit_files, MAX_NUM_UNITS);
    num_unit_files += get_platform_unit_files(&unit_files[num_unit_files], MAX_NUM_UNITS - num_unit_files);

    // Install and render midplane network service for smart switch
    if (smart_switch) {
        if (render_network_service_for_smart_switch(install_dir) != 0) {
            return -1;
        }
    }

    // Mask systemd-networkd on non-smart-switch platforms
    if (mask_networkd_for_non_smart_switch(install_dir) != 0) {
        return -1;
    }

    // For each unit file, get the installation targets and install the unit
    for (int i = 0; i < num_unit_files; i++) {
        unit_instance = unit_files[i];
        if ((num_asics == 1 &&
             !is_multi_instance_service_for_dpu(unit_instance)) &&
            unit_instance.find("@") != std::string::npos) {
            prefix = unit_instance.substr(0, unit_instance.find("@"));
            suffix = unit_instance.substr(unit_instance.find("@") + 1);

            unit_instance = prefix + suffix;
        }

        auto instance_name = unit_instance.substr(0, unit_instance.find('.'));

        if ((num_asics > 1) && !is_multi_instance_service(instance_name)) {
            replace_multi_inst_dep(install_dir, unit_instance);
        }

        num_targets = get_install_targets(unit_instance, targets);
        if (num_targets < 0) {
            log_to_kmsg("Error parsing %s\n", unit_instance.c_str());
            free(unit_files[i]);
            continue;
        }

        for (int j = 0; j < num_targets; j++) {
            if (install_unit_file(unit_instance, targets[j], install_dir) != 0)
                log_to_kmsg("Error installing %s to target directory %s\n", unit_instance.c_str(), targets[j]);

            free(targets[j]);
        }

        update_environment(install_dir, unit_instance);

        free(unit_files[i]);
    }

    for (int i = 0; i < num_multi_inst; i++) {
        free(multi_instance_services[i]);
    }
    free(multi_instance_services);
    multi_instance_services = NULL;
    num_multi_inst = 0;

    if (is_valid_pointer(platform_info)) {
        json_object_put(platform_info);
    }

    return 0;
}


#ifndef _SSG_UNITTEST
int main(int argc, char **argv) {
   return ssg_main(argc, argv);
}
#endif
