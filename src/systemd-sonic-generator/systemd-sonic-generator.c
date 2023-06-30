#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>

#define MAX_NUM_TARGETS 48
#define MAX_NUM_INSTALL_LINES 48
#define MAX_NUM_UNITS 128
#define MAX_BUF_SIZE 512

const char* UNIT_FILE_PREFIX = "/usr/lib/systemd/system/";
const char* CONFIG_FILE = "/etc/sonic/generated_services.conf";
const char* MACHINE_CONF_FILE = "/host/machine.conf";
const char* ASIC_CONF_FORMAT = "/usr/share/sonic/device/%s/asic.conf";

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

static int num_asics;
static char** multi_instance_services;
static int num_multi_inst;

void strip_trailing_newline(char* str) {
    /***
    Strips trailing newline from a string if it exists
    ***/

    size_t l = strlen(str);
    if (l > 0 && str[l-1] == '\n')
        str[l-1] = '\0';
}


static int get_target_lines(char* unit_file, char* target_lines[]) {
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
        fprintf(stderr, "Failed to open file %s\n", unit_file);
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
                fprintf(stderr, "Number of lines in [Install] section of %s exceeds MAX_NUM_INSTALL_LINES\n", unit_file);
                fputs("Extra [Install] lines will be ignored\n", stderr);
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

static bool is_multi_instance_service(char *service_name){
    int i;
    for(i=0; i < num_multi_inst; i++){
        /*
         * The service name may contain @.service or .service. Remove these
         * postfixes and extract service name. Compare service name for absolute
         * match in multi_instance_services[].
         * This is to prevent services like database-chassis and systemd-timesyncd marked
         * as multi instance services as they contain strings 'database' and 'syncd' respectively
         * which are multi instance services in multi_instance_services[].
         */
        char *saveptr;
        char *token = strtok_r(service_name, "@", &saveptr);
        if (token) {
            if (strstr(token, ".service") != NULL) {
                /* If we are here, service_name did not have '@' delimiter but contains '.service' */
                token = strtok_r(service_name, ".", &saveptr);
            }
        }
        if (strncmp(service_name, multi_instance_services[i], strlen(service_name)) == 0) {
            return true;
        }
    }
    return false;

}

static int get_install_targets_from_line(char* target_string, char* install_type, char* targets[], int existing_targets) {
    /***
    Helper fuction for get_install_targets

    Given a space delimited string of target directories and a suffix,
    puts each target directory plus the suffix into the targets array
    ***/
    char* token;
    char* target;
    char* saveptr;
    char final_target[PATH_MAX];
    int num_targets = 0;

    while ((token = strtok_r(target_string, " ", &target_string))) {
        if (num_targets + existing_targets >= MAX_NUM_TARGETS) {
            fputs("Number of targets found exceeds MAX_NUM_TARGETS\n", stderr);
            fputs("Additional targets will be ignored \n", stderr);
            return num_targets;
        }

        target = strdup(token);
        strip_trailing_newline(target);

        if (strstr(target, "%") != NULL) {
            char* prefix = strtok_r(target, ".", &saveptr);
            char* suffix = strtok_r(NULL, ".", &saveptr);
            int prefix_len = strlen(prefix);

            strncpy(final_target, prefix, prefix_len - 2);
            final_target[prefix_len - 2] = '\0';
            strcat(final_target, ".");
            strcat(final_target, suffix);
        }
        else {
            strcpy(final_target, target);
        }
        strcat(final_target, install_type);

        free(target);

        targets[num_targets + existing_targets] = strdup(final_target);
        num_targets++;
    }
    return num_targets;
}

static void replace_multi_inst_dep(char *src) {
    FILE *fp_src;
    FILE *fp_tmp;
    char buf[MAX_BUF_SIZE];
    char* line = NULL;
    int i;
    ssize_t len;
    char *token;
    char *word;
    char *line_copy;
    char *service_name;
    char *type;
    char *save_ptr1 = NULL;
    char *save_ptr2 = NULL;
    ssize_t nread;
    bool section_done = false;
    char tmp_file_path[PATH_MAX];

    /* Assumes that the service files has 3 sections,
     * in the order: Unit, Service and Install.
     * Assumes that the timer file has 3 sections,
     * in the order: Unit, Timer and Install.
     * Read service dependency from Unit and Install
     * sections, replace if dependent on multi instance
     * service.
     */
    fp_src = fopen(src, "r");
    snprintf(tmp_file_path, PATH_MAX, "%s.tmp", src);
    fp_tmp = fopen(tmp_file_path, "w");

    while ((nread = getline(&line, &len, fp_src)) != -1 ) {
        if ((strstr(line, "[Service]") != NULL) ||
            (strstr(line, "[Timer]") != NULL)) {
            section_done = true;
            fputs(line,fp_tmp);
        } else if (strstr(line, "[Install]") != NULL) {
            section_done = false;
            fputs(line,fp_tmp);
        } else if ((strstr(line, "[Unit]") != NULL) ||
           (strstr(line, "Description") != NULL) ||
           (section_done == true)) {
            fputs(line,fp_tmp);
        } else {
            line_copy = strdup(line);
            token = strtok_r(line_copy, "=", &save_ptr1);
            while ((word = strtok_r(NULL, " ", &save_ptr1))) {
                if((strchr(word, '.') == NULL) ||
                   (strchr(word, '@') != NULL)) {
                    snprintf(buf, MAX_BUF_SIZE,"%s=%s\n",token, word);
                    fputs(buf,fp_tmp);
                } else {
                    service_name = strdup(word);
                    service_name = strtok_r(service_name, ".", &save_ptr2);
                    type = strtok_r(NULL, "\n", &save_ptr2);
                    if (is_multi_instance_service(word)) {
                        for(i = 0; i < num_asics; i++) {
                            snprintf(buf, MAX_BUF_SIZE, "%s=%s@%d.%s\n",
                                    token, service_name, i, type);
                            fputs(buf,fp_tmp);
                        }
                    } else {
                        snprintf(buf, MAX_BUF_SIZE,"%s=%s.%s\n",token, service_name, type);
                        fputs(buf, fp_tmp);
                    }
                    free(service_name);
                }
            }
            free(line_copy);
        }
    }
    fclose(fp_src);
    fclose(fp_tmp);
    free(line);
    /* remove the .service file, rename the .service.tmp file
     * as .service.
     */
    remove(src);
    rename(tmp_file_path, src);
}

int get_install_targets(char* unit_file, char* targets[]) {
    /***
    Returns install targets for a unit file

    Parses the information in the [Install] section of a given
    unit file to determine which directories to install the unit in
    ***/
    char file_path[PATH_MAX];
    char *target_lines[MAX_NUM_INSTALL_LINES];
    int num_target_lines;
    int num_targets;
    int found_targets;
    char* token;
    char* line = NULL;
    bool first;
    char* target_suffix;
    char *instance_name;
    char *dot_ptr;

    strcpy(file_path, get_unit_file_prefix());
    strcat(file_path, unit_file);

    instance_name = strdup(unit_file);
    dot_ptr = strchr(instance_name, '.');
    *dot_ptr = '\0';

    if((num_asics > 1) && (!is_multi_instance_service(instance_name))) {
        replace_multi_inst_dep(file_path);
    }
    free(instance_name);

    num_target_lines = get_target_lines(file_path, target_lines);
    if (num_target_lines < 0) {
        fprintf(stderr, "Error parsing targets for %s\n", unit_file);
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


int get_unit_files(char* unit_files[]) {
    /***
    Reads a list of unit files to be installed from /etc/sonic/generated_services.conf
    ***/
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char *pos;
    const char* config_file = get_config_file();

    fp = fopen(config_file, "r");

    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s\n", config_file);
        exit(EXIT_FAILURE);
    }

    int num_unit_files = 0;
    num_multi_inst = 0;

    multi_instance_services = calloc(MAX_NUM_UNITS, sizeof(char *));

    while ((read = getline(&line, &len, fp)) != -1) {
        if (num_unit_files >= MAX_NUM_UNITS) {
            fprintf(stderr, "Maximum number of units exceeded, ignoring extras\n");
            break;
        }
        strip_trailing_newline(line);

        /* Get the multi-instance services */
        pos = strchr(line, '@');
        if (pos != NULL) {
            multi_instance_services[num_multi_inst] = calloc(strlen(line), sizeof(char));
            strncpy(multi_instance_services[num_multi_inst], line, pos-line);
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


char* insert_instance_number(char* unit_file, int instance) {
    /***
    Adds an instance number to a systemd template name

    E.g. given unit_file='example@.service', instance=3,
    returns a pointer to 'example@3.service'
    ***/
    char* instance_name;
    int   ret;
    int   prefix_len;
    const char *suffix = strchr(unit_file, '@');
    if (!suffix) {
        fprintf(stderr, "Invalid unit file %s for instance %d\n", unit_file, instance);
        return NULL;
    }

    /***
    suffix is "@.service", set suffix=".service"
    prefix_len is length of "example@"
    ***/
    prefix_len = ++suffix - unit_file;
    ret = asprintf(&instance_name, "%.*s%d%s", prefix_len, unit_file, instance, suffix);
    if (ret == -1) {
        fprintf(stderr, "Error creating instance %d of %s\n", instance, unit_file);
        return NULL;
    }

    return instance_name;
}


static int create_symlink(char* unit, char* target, char* install_dir, int instance) {
    struct stat st;
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];
    char final_install_dir[PATH_MAX];
    char* unit_instance;
    int r;

    strcpy(src_path, get_unit_file_prefix());
    strcat(src_path, unit);

    if (instance < 0) {
        unit_instance = strdup(unit);
    }
    else {
        unit_instance = insert_instance_number(unit, instance);
    }

    strcpy(final_install_dir, install_dir);
    strcat(final_install_dir, target);
    strcpy(dest_path, final_install_dir);
    strcat(dest_path, "/");
    strcat(dest_path, unit_instance);

    free(unit_instance);

    if (stat(final_install_dir, &st) == -1) {
        // If doesn't exist, create
        r = mkdir(final_install_dir, 0755);
        if (r == -1) {
            fprintf(stderr, "Unable to create target directory %s\n", final_install_dir);
            return -1;
        }
    }
    else if (S_ISREG(st.st_mode)) {
        // If is regular file, remove and create
        r = remove(final_install_dir);
        if (r == -1) {
            fprintf(stderr, "Unable to remove file with same name as target directory %s\n", final_install_dir);
            return -1;
        }

        r = mkdir(final_install_dir, 0755);
        if (r == -1) {
            fprintf(stderr, "Unable to create target directory %s\n", final_install_dir);
            return -1;
        }
    }
    else if (S_ISDIR(st.st_mode)) {
        // If directory, verify correct permissions
        r = chmod(final_install_dir, 0755);
        if (r == -1) {
            fprintf(stderr, "Unable to change permissions of existing target directory %s\n", final_install_dir);
            return -1;
        }
    }

    r = symlink(src_path, dest_path);

    if (r < 0) {
        if (errno == EEXIST)
            return 0;
        fprintf(stderr, "Error creating symlink %s from source %s\n", dest_path, src_path);
        return -1;
    }

    return 0;

}


static int install_unit_file(char* unit_file, char* target, char* install_dir) {
    /***
    Creates a symlink for a unit file installation

    For a given unit file and target directory,
    create the appropriate symlink in the target directory
    to enable the unit and have it started by Systemd

    If a multi ASIC platform is detected, enables multi-instance
    services as well
    ***/
    char* target_instance;
    char* prefix;
    char* suffix;
    int r;

    assert(unit_file);
    assert(target);


    if ((num_asics > 1) && strstr(unit_file, "@") != NULL) {

        for (int i = 0; i < num_asics; i++) {

            if (strstr(target, "@") != NULL) {
                target_instance = insert_instance_number(target, i);
            }
            else {
                target_instance = strdup(target);
            }

            r = create_symlink(unit_file, target_instance, install_dir, i);
            if (r < 0)
                fprintf(stderr, "Error installing %s for target %s\n", unit_file, target_instance);

            free(target_instance);

        }
    }
    else {
        r = create_symlink(unit_file, target, install_dir, -1);
        if (r < 0)
            fprintf(stderr, "Error installing %s for target %s\n", unit_file, target);
    }

    return 0;
}


int get_num_of_asic() {
    /***
    Determines if the current platform is single or multi-ASIC
    ***/
    FILE *fp;
    char *line = NULL;
    char* token;
    char* platform = NULL;
    char* saveptr;
    size_t len = 0;
    ssize_t nread;
    bool ans;
    char asic_file[512];
    char* str_num_asic;
    int num_asic = 1;
    const char* machine_config_file = get_machine_config_file();

    fp = fopen(machine_config_file, "r");

    if (fp == NULL) {
        fprintf(stderr, "Failed to open %s\n", machine_config_file);
        exit(EXIT_FAILURE);
    }

    while ((nread = getline(&line, &len, fp)) != -1) {
        if ((strstr(line, "onie_platform") != NULL) ||
            (strstr(line, "aboot_platform") != NULL)) {
            token = strtok_r(line, "=", &saveptr);
            platform = strtok_r(NULL, "=", &saveptr);
            strip_trailing_newline(platform);
            break;
        }
    }
    fclose(fp);
    if(platform != NULL) {
        snprintf(asic_file, 512, get_asic_conf_format(), platform);
        fp = fopen(asic_file, "r");
        if (fp != NULL) {
            while ((nread = getline(&line, &len, fp)) != -1) {
                if (strstr(line, "NUM_ASIC") != NULL) {
                    token = strtok_r(line, "=", &saveptr);
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

int ssg_main(int argc, char **argv) {
    char* unit_files[MAX_NUM_UNITS];
    char install_dir[PATH_MAX];
    char* targets[MAX_NUM_TARGETS];
    char* unit_instance;
    char* prefix;
    char* suffix;
    char* saveptr;
    int num_unit_files;
    int num_targets;
    int r;

    if (argc <= 1) {
        fputs("Installation directory required as argument\n", stderr);
        return 1;
    }

    num_asics = get_num_of_asic();
    strcpy(install_dir, argv[1]);
    strcat(install_dir, "/");
    num_unit_files = get_unit_files(unit_files);

    // For each unit file, get the installation targets and install the unit
    for (int i = 0; i < num_unit_files; i++) {
        unit_instance = strdup(unit_files[i]);
        if ((num_asics == 1) && strstr(unit_instance, "@") != NULL) {
            prefix = strdup(strtok_r(unit_instance, "@", &saveptr));
            suffix = strdup(strtok_r(NULL, "@", &saveptr));

            strcpy(unit_instance, prefix);
            strcat(unit_instance, suffix);

            free(prefix);
            free(suffix);
        }

        num_targets = get_install_targets(unit_instance, targets);
        if (num_targets < 0) {
            fprintf(stderr, "Error parsing %s\n", unit_instance);
            free(unit_instance);
            free(unit_files[i]);
            continue;
        }

        for (int j = 0; j < num_targets; j++) {
            if (install_unit_file(unit_instance, targets[j], install_dir) != 0)
                fprintf(stderr, "Error installing %s to target directory %s\n", unit_instance, targets[j]);

            free(targets[j]);
        }

        free(unit_instance);
        free(unit_files[i]);
    }

    for (int i = 0; i < num_multi_inst; i++) {
        free(multi_instance_services[i]);
    }
    free(multi_instance_services);

    return 0;
}


#ifndef _SSG_UNITTEST
int main(int argc, char **argv) {
   return ssg_main(argc, argv);
}
#endif
