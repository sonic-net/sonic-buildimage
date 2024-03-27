// #include <assert.h>
// #include <stdio.h>
// #include <unistd.h>
// #include <errno.h>
// #include <string.h>
// #include <stdlib.h>
// #include <stdbool.h>
// #include <sys/types.h>
// #include <linux/limits.h>


#include <sys/stat.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <unistd.h>

// #define MAX_NUM_TARGETS 48
// #define MAX_NUM_INSTALL_LINES 48
// #define MAX_NUM_UNITS 128
// #define MAX_BUF_SIZE 512

const std::string UNIT_FILE_PREFIX = "/usr/lib/systemd/system/";
const std::string CONFIG_FILE = "/etc/sonic/generated_services.conf";
const std::string MACHINE_CONF_FILE = "/host/machine.conf";
const std::vector<std::string> ASIC_CONF_FORMAT = {"/usr/share/sonic/device/", "/asic.conf"};

std::string g_unit_file_prefix;
std::string g_config_file;
std::string g_machine_config_file;
std::vector<std::string> g_asic_conf_format;

std::string get_unit_file_prefix() {
    if (g_unit_file_prefix.empty()) {
        return UNIT_FILE_PREFIX;
    }
    return g_unit_file_prefix;
}

std::string get_config_file() {
    if (g_config_file.empty()) {
        return CONFIG_FILE;
    }
    return g_config_file;
}

std::string get_machine_config_file() {
    if (g_machine_config_file.empty()) {
        return MACHINE_CONF_FILE;
    }
    return g_machine_config_file;
}

std::vector<std::string> get_asic_conf_format() {
    if (g_asic_conf_format.empty()) {
        return ASIC_CONF_FORMAT;
    }
    return g_asic_conf_format;
}

static int num_asics;
static std::unordered_set<std::string> multi_instance_services;

std::string strip_trailing_newline(std::string str) {
    /***
    Strips trailing newline from a string if it exists
    ***/
   if (!str.empty() && str.back() == '\n') {
       str.pop_back();
   }
   return str;
}


static std::vector<std::string> get_target_lines(const std::string& unit_file_name) {
    /***
    Gets installation information for a given unit file

    Returns lines in the [Install] section of a unit file
    ***/
    bool found_install = false;
    std::vector<std::string> target_lines;   
    std::string line;
    std::ifstream unit_file(unit_file_name);
    if (!unit_file.is_open()) {
        std::cerr << "Failed to open file " << unit_file_name << std::endl;
        return target_lines;
    }

    while (std::getline(unit_file, line)) {
        // Assume that the [Install] section is the last section in the unit file
        if (line.find("[Install]") != std::string::npos) {
            found_install = true;
        }
        else if (found_install) {
            target_lines.push_back(line);
        }
    }

    return target_lines;
}

static bool is_multi_instance_service(const std::string& service_file){
    /*
        * The service file may match the form `<service-name>.service`
        * or `<service-name>@.service`. Remove these postfixes and
        * extract service name. Compare service name for absolute
        * match in multi_instance_services[].
        * This is to prevent services like database-chassis and
        * systemd-timesyncd from being marked as multi-instance
        * services as they contain strings 'database' and 'syncd'
        * respectively which are multi instance services in
        * multi_instance_services[].
        */
    std::string delimiter;
    if (service_file.find("@") == std::string::npos) {
        delimiter = "@";
    } else {
        delimiter = ".";
    }
    std::string service_name = service_file.substr(0, service_file.find(delimiter));

    if (multi_instance_services.count(service_name) > 0) {
        return true;
    }
    return false;
}

static std::vector<std::string> get_install_targets_from_line(const std::string& target_string, const std::string& install_type) {
    /***
    Helper fuction for get_install_targets

    Given a space delimited string of target directories and a suffix,
    puts each target directory plus the suffix into the targets vector
    ***/
    std::string target;
    std::string prefix;
    std::string suffix; 
    std::vector<std::string> targets;

    if (target_string.empty() || install_type.empty()) {
        std::cerr << "Invalid target string or install type" << std::endl;
        return targets;
    }

    std::stringstream ss(target_string);
    while (ss >> target) {
        // handle install targets using the '%i' systemd specifier
        if (target.find("%") != std::string::npos) {
            target = target.substr(0, target.find("%")) + target.substr(target.find("."));
        }
        targets.push_back(strip_trailing_newline(target) + install_type);
    }
    return targets;
}

static std::string get_multi_inst_line(const std::string& property_name, const std::string& property_val) {
    std::stringstream new_line;
    if (property_val.find(".") == std::string::npos || property_val.find("@") != std::string::npos) {
        new_line << property_name << "=" << property_val << std::endl;
    } else {
        std::string service_name = property_val.substr(0, property_val.find("."));
        std::string type = property_val.substr(property_val.find(".") + 1);
        if (is_multi_instance_service(service_name)) {
            for (int i = 0; i < num_asics; i++) {
                new_line << property_name << "=" << service_name << "@" << i << "." << type << std::endl;
            }
        } else {
            new_line << property_name << "=" << service_name << "." << type << std::endl;
        }
    }
    return new_line.str();
}

static void replace_multi_inst_dep(const std::string& orig_unit_file) {
    /* Assumes that the service files has 3 sections,
     * in the order: Unit, Service and Install.
     * Assumes that the timer file has 3 sections,
     * in the order: Unit, Timer and Install.
     * Read service dependency from Unit and Install
     * sections, replace if dependent on multi instance
     * service.
     */
    std::vector<std::string> target_lines;   
    std::string line;
    std::string property;
    std::string property_name;
    std::string property_values;
    std::string tmp_file_name = orig_unit_file + ".tmp";
    std::ifstream unit_file(orig_unit_file);
    std::ofstream tmp_file(tmp_file_name);
    std::stringstream new_line;
    if (!unit_file.is_open()) {
        std::cerr << "Failed to open file " << orig_unit_file << std::endl;
        return;
    }
    if (!tmp_file.is_open()) {
        std::cerr << "Failed to open file " << tmp_file_name << std::endl;
        return;
    }

    std::vector<std::string> keep_lines = {"[Service]", "[Timer]", "[Install]", "[Unit]", "Description"};
    while (std::getline(unit_file, line)) {
        bool copy_orig_line = std::any_of(keep_lines.begin(), keep_lines.end(), [&line](const std::string& s) { return line.find(s) != std::string::npos; });
        if (copy_orig_line) {
            tmp_file << line << std::endl;
        } else {
            property_name = line.substr(0, line.find("="));
            property_values= line.substr(line.find("=") + 1);
            std::stringstream ss(property_values);
            while (ss >> property) {
                tmp_file << get_multi_inst_line(property_name, property) << std::endl;
            }
        }
    }
    /* remove the .service file, rename the .service.tmp file
     * as .service.
     */
    remove(orig_unit_file.c_str());
    rename(tmp_file_name.c_str(), orig_unit_file.c_str());
}

std::vector<std::string> get_install_targets(const std::string& unit_file) {
    /***
    Returns install targets for a unit file

    Parses the information in the [Install] section of a given
    unit file to determine which directories to install the unit in
    ***/
    std::string target_suffix;
    std::string type;
    std::vector<std::string> targets;

    std::string file_path = get_unit_file_prefix() + unit_file;
    std::string instance_name = unit_file.substr(0, unit_file.find("."));

    if((num_asics > 1) && (!is_multi_instance_service(instance_name))) {
        replace_multi_inst_dep(file_path);
    }

    std::vector<std::string> target_lines = get_target_lines(file_path);
    if (target_lines.empty()) {
        std::cerr << "Error parsing targets for " << unit_file << std::endl;
        return targets;
    }

    for (std::string line : target_lines) {
        size_t equal_pos = line.find("=");
        type = line.substr(0, equal_pos);

        if (type.find("WantedBy") != std::string::npos) {
            target_suffix = ".wants";
        }
        else if (type.find("RequiredBy") != std::string::npos) {
            target_suffix = ".requires";
        }
        else {
            std::cerr << "Invalid target type " << type << std::endl;
            continue;
        }

        targets = get_install_targets_from_line(line.substr(equal_pos + 1), target_suffix);
    }
    return targets;
}


std::vector<std::string> get_unit_files() {
    /***
    Reads a list of unit files to be installed from /etc/sonic/generated_services.conf
    ***/
    std::string line;
    std::vector<std::string> unit_files;
    std::string config_file = get_config_file();
    std::ifstream file(config_file);

    if (!file.is_open()) {
        std::cerr << "Failed to open " << config_file << std::endl;
        return unit_files;
    }

    while (std::getline(file, line)) {
        size_t at_pos = line.find("@");
        if (at_pos != std::string::npos) {
            multi_instance_services.insert(line.substr(0, at_pos));
        }

        if (line.find("topology.service") != std::string::npos && num_asics == 1) {
            continue;
        }
        unit_files.push_back(strip_trailing_newline(line));
    }
    return unit_files;
}


std::string insert_instance_number(const std::string& unit_file, int instance) {
    /***
    Adds an instance number to a systemd template name

    E.g. given unit_file='example@.service', instance=3,
    returns a pointer to 'example@3.service'
    ***/
    size_t at_pos = unit_file.find("@");
    if (at_pos == std::string::npos) {
        std::cerr << "Invalid unit file " << unit_file << " for instance " << instance << std::endl;
        return "";
    }

    return unit_file.substr(0, at_pos + 1) + std::to_string(instance) + unit_file.substr(at_pos + 2);
}


static int create_symlink(const std::string& unit, const std::string& target, const std::string& install_dir, int instance) {
    struct stat st;
    int r;
    std::string unit_instance;
    std::string final_install_dir;
    std::string dest_path;
    std::string src_path = get_unit_file_prefix() + unit;

    if (instance < 0) {
        unit_instance = unit;
    }
    else {
        unit_instance = insert_instance_number(unit, instance);
    }

    final_install_dir = install_dir + target;
    dest_path = final_install_dir + "/" + unit_instance;

    if (stat(final_install_dir.c_str(), &st) == -1) {
        // If doesn't exist, create
        r = mkdir(final_install_dir.c_str(), 0755);
        if (r == -1) {
            std::cerr << "Unable to create target directory " << final_install_dir << std::endl;
            return -1;
        }
    }
    else if (S_ISREG(st.st_mode)) {
        // If is regular file, remove and create
        r = remove(final_install_dir.c_str());
        if (r == -1) {
            std::cerr << "Unable to remove file with same name as target directory " << final_install_dir << std::endl;
            return -1;
        }

        r = mkdir(final_install_dir.c_str(), 0755);
        if (r == -1) {
            std::cerr << "Unable to create target directory " << final_install_dir << std::endl;
            return -1;
        }
    }
    else if (S_ISDIR(st.st_mode)) {
        // If directory, verify correct permissions
        r = chmod(final_install_dir.c_str(), 0755);
        if (r == -1) {
            std::cerr << "Unable to change permissions of existing target directory " << final_install_dir << std::endl;
            return -1;
        }
    }

    r = symlink(src_path.c_str(), dest_path.c_str());

    if (r < 0) {
        if (errno == EEXIST)
            return 0;
        std::cerr << "Error creating symlink " << dest_path << " from source " << src_path << std::endl;
        return -1;
    }
    return 0;
}


static int install_unit_file(const std::string& unit_file, const std::string& target, const std::string& install_dir) {
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
    int ret_val = 0;

    if (unit_file.empty() || target.empty() || install_dir.empty()){
        return -1;
    }

    if (num_asics > 1 && unit_file.find("@") != std::string::npos) {

        for (int i = 0; i < num_asics; i++) {

            if (target.find("%") != std::string::npos) {
                target_instance = insert_instance_number(target, i);
            }
            else {
                target_instance = target;
            }

            r = create_symlink(unit_file, target_instance, install_dir, i);
            if (r < 0) {
                std::cerr << "Error installing " << unit_file << " for target " << target_instance << std::endl;
                ret_val = -1; 
            }


        }
    }
    else {
        r = create_symlink(unit_file, target, install_dir, -1);
        if (r < 0) {
            std::cerr << "Error installing " << unit_file << " for target " << target << std::endl;
            ret_val = -1;
        }
    }

    return ret_val;
}

static int get_num_asic_from_asic_file(const std::string& asic_file) {
    std::string str_num_asic;
    std::string line;
    std::ifstream asic_fstream(asic_file);
    if (!asic_fstream.is_open()) {
        std::cerr << "Failed to open " << asic_file << std::endl;
        exit(EXIT_FAILURE);
    } 
    while (std::getline(asic_fstream, line)) {
        if (line.find("NUM_ASIC") != std::string::npos) {
            str_num_asic = strip_trailing_newline(line.substr(line.find("=") + 1));
            if (!str_num_asic.empty()){
                return std::stoi(str_num_asic);
            }
        }
    }
    return -1;
}

int get_num_of_asic() {
    /***
    Determines if the current platform is single or multi-ASIC
    ***/
    std::string str_num_asic;
    std::string line;
    std::string platform;
    std::string asic_file;
    std::string machine_config_file = get_machine_config_file();
    std::ifstream file(machine_config_file);
    std::ifstream asic_fstream;

    if (!file.is_open()) {
        std::cerr << "Failed to open " << machine_config_file << std::endl;
        exit(EXIT_FAILURE);
    }

    while (std::getline(file, line)) {
        if (line.find("onie_platform") != std::string::npos || line.find("aboot_platform") != std::string::npos) {
            platform = strip_trailing_newline(line.substr(line.find("=") + 1));
            break;
        }
    }

    if(platform.empty()) {
        std::cerr << "Failed to get platform from " << machine_config_file << std::endl;
        exit(EXIT_FAILURE);
    }
    asic_file = get_asic_conf_format()[0] + platform + get_asic_conf_format()[1]; 
    return get_num_asic_from_asic_file(asic_file);
}


int ssg_main(int argc, char **argv) {
    std::vector<std::string> unit_files;
    std::vector<std::string> targets;
    std::string unit_instance;
    std::string prefix;
    std::string suffix;
    std::string install_dir;

    if (argc <= 1) {
        std::cerr << "Installation directory required as argument" << std::endl;
        return 1;
    }

    num_asics = get_num_of_asic();
    if (num_asics < 0) {
        std::cerr << "Failed to get number of ASICs" << std::endl;
        exit(EXIT_FAILURE);
    }
    install_dir = argv[1] + std::string("/");
    unit_files = get_unit_files();

    // For each unit file, get the installation targets and install the unit
    for (std::string file: unit_files) {
        size_t at_pos = file.find("@");
        if (num_asics == 1 && at_pos != std::string::npos) {
            prefix = file.substr(0, at_pos);
            suffix = file.substr(at_pos + 1);
            unit_instance = prefix + suffix;
        } else {
            unit_instance = file;
        }

        targets = get_install_targets(unit_instance);
        if (targets.empty()) {
            std::cerr << "Error parsing " << unit_instance << std::endl;
            continue;
        }

        for (std::string target: targets) {
            if (install_unit_file(unit_instance, target, install_dir) != 0) {
                std::cerr << "Error installing " << unit_instance << " to target directory " << target << std::endl;
            }

        }
    }
    return 0;
}


#ifndef _SSG_UNITTEST
int main(int argc, char **argv) {
   return ssg_main(argc, argv);
}
#endif
