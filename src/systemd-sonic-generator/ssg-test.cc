/*------------------------------------------------------------------
 * ssg-test.cc - systemd-sonic-generator Unit Test
 *
 * Initial: Apr 2021
 *
 * Copyright (c) 2021 by Cisco Systems, Inc.
 *------------------------------------------------------------------
 */
#include <fstream>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <linux/limits.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <nlohmann/json.hpp>
#include "systemd-sonic-generator.h"

namespace fs = boost::filesystem;

namespace SSGTest {
#define IS_MULTI_ASIC(x)  ((x) > 1)
#define IS_SINGLE_ASIC(x) ((x) <= 1)
#define NUM_UNIT_FILES 9

/*
 * This test class uses following directory hierarchy for input and output
 * data for systemd-sonic-generator.
 *
 *    tests/ssg-test/   --- Test data directory
 *             |
 *             |---generated_services.conf
 *             |---machine.conf (systemd-sonic-generator fetch platform from here)
 *             |---systemd/
 *             |       |--- *.service (Test unit files are copied from
 *             |                       tests/testfiles/ to here)
 *             |----test_platform/  (test platform)
 *             |       |---asic.conf
 *             |
 *             |----generator/   (Output Directory)
 *
 */
const std::string TEST_ROOT_DIR = "tests/ssg-test/";
const std::string TEST_UNIT_FILE_PREFIX = TEST_ROOT_DIR + "systemd/";
const std::string TEST_LIB_NETWORK = TEST_UNIT_FILE_PREFIX + "network/";
const std::string TEST_ASIC_CONF_FORMAT = TEST_ROOT_DIR + "%s/asic.conf";
const std::string TEST_PLATFORM_FILE_FORMAT = TEST_ROOT_DIR + "%s/platform.json";
const std::string TEST_MACHINE_CONF = TEST_ROOT_DIR + "machine.conf";
const std::string TEST_PLATFORM_CONF_FORMAT = TEST_ROOT_DIR + "%s/services.conf";

const std::string TEST_PLATFORM_DIR = TEST_ROOT_DIR + "test_platform/";
const std::string TEST_ASIC_CONF = TEST_PLATFORM_DIR + "asic.conf";
const std::string TEST_PLATFORM_CONF = TEST_PLATFORM_DIR + "platform.json";

const std::string TEST_OUTPUT_DIR = TEST_ROOT_DIR + "generator/";
const std::string TEST_ETC_NETWORK = TEST_OUTPUT_DIR + "network/";
const std::string TEST_ETC_SYSTEM = TEST_OUTPUT_DIR + "system/";

const std::string TEST_CONFIG_FILE = TEST_ROOT_DIR + "generated_services.conf";

const std::string TEST_UNIT_FILES = "tests/testfiles/";

const std::string TEST_PLATFORM_CONFIG = TEST_PLATFORM_DIR + "services.conf";

/* Input data for generated_services.conf */
const std::vector<std::string> generated_services = {
    "multi_inst_a.service",         /* Single instance of a  multi asic service a */
    "multi_inst_a@.service",        /* Multi-instance of a multi asic service a */
    "multi_inst_b@.service",        /* Multi-instance of a multi asic service b */
    "single_inst.service",          /* A single instance service */
    "test.service",                 /* A single instance test service
                                       to test dependency creation */
    "test.timer",                   /* A timer service */
    "database.service",             /* A database service*/
    "database@.service",            /* A database service for multi instances */
};

static std::mutex g_ssg_test_mutex;

class SystemdSonicGeneratorFixture : public testing::Test {
  protected:
    /* Save global variables before running tests */
    virtual void SetUp() {
        /* one test runs at a time */
        g_ssg_test_mutex.lock();

        unit_file_prefix_ = g_unit_file_prefix;
        config_file_ = g_config_file;
        machine_config_file_ = g_machine_config_file;
        asic_conf_format_ = g_asic_conf_format;
        platform_conf_format_ = g_platform_conf_format;
    }

    /* Restore global vars */
    virtual void TearDown() {
        g_unit_file_prefix = unit_file_prefix_;
        g_config_file = config_file_;
        g_machine_config_file = machine_config_file_;
        g_asic_conf_format = asic_conf_format_;
        g_platform_conf_format = platform_conf_format_;

        g_ssg_test_mutex.unlock();
    }

  private:
    const char* unit_file_prefix_;
    const char* config_file_;
    const char* machine_config_file_;
    const char* asic_conf_format_;
    const char* platform_conf_format_;
};

/*
 * class SsgFunctionTest
 * Implements functions to execute functional level tests.
 */
class SsgFunctionTest : public SystemdSonicGeneratorFixture {
  protected:
    /* This function generates the generated_services.conf file */
    void generate_generated_services_conf() {
        FILE* fp = fopen(TEST_CONFIG_FILE.c_str(), "w");
        ASSERT_NE(fp, nullptr);
        for (std::string str : generated_services) {
            fputs(str.c_str(), fp);
            fputs("\n", fp);
        }
        fclose(fp);
    }

    /* copy files from src_dir to dest_dir */
    void copyfiles(const char* src_dir, const char* dest_dir) {
        // Iterate through the source directory
        for (fs::directory_iterator file(src_dir);
                 file != fs::directory_iterator(); ++file) {
            try {
                fs::path current(file->path());
                if(!fs::is_directory(current)) {
                    std::string ext = boost::filesystem::extension(current);
                    fs::path dest_path = dest_dir;
                    if (ext == ".netdev" || ext == ".network" || ext == ".link") {
                        dest_path = dest_path / "network";
                    }
                    /* Copy file */
                    fs::copy_file( current, dest_path / current.filename());
                }
            }
            catch(fs::filesystem_error const & e) {
                std:: cerr << e.what() << '\n';
            }
        }
    }

    /* Save global variables before running tests */
    virtual void SetUp() {
        FILE* fp;
        SystemdSonicGeneratorFixture::SetUp();

        /* Setup Input and Output directories and files */
        fs::path path{TEST_UNIT_FILE_PREFIX.c_str()};
        fs::create_directories(path);
        path = fs::path(TEST_OUTPUT_DIR.c_str());
        fs::create_directories(path);
        path = fs::path(TEST_PLATFORM_DIR.c_str());
        fs::create_directories(path);
        path = fs::path(TEST_LIB_NETWORK.c_str());
        fs::create_directories(path);
        path = fs::path(TEST_ETC_NETWORK.c_str());
        fs::create_directories(path);
        path = fs::path(TEST_ETC_SYSTEM.c_str());
        fs::create_directories(path);
        fp = fopen(TEST_MACHINE_CONF.c_str(), "w");
        ASSERT_NE(fp, nullptr);
        fputs("onie_platform=test_platform", fp);
        fclose(fp);

        fp = fopen(TEST_PLATFORM_CONFIG.c_str(), "w");
        ASSERT_NE(fp, nullptr);
        fputs("platform_specific.service\n", fp);
        fclose(fp);
        generate_generated_services_conf();
        copyfiles(TEST_UNIT_FILES.c_str(), TEST_UNIT_FILE_PREFIX.c_str());
    }

    /* Restore global vars */
    virtual void TearDown() {
        /* Delete ssg_test directory */
        EXPECT_TRUE(fs::exists(TEST_ROOT_DIR.c_str()));
        fs::path path{TEST_ROOT_DIR.c_str()};
        fs::remove_all(path);

        SystemdSonicGeneratorFixture::TearDown();
    }

  private:
};


struct SsgMainConfig {
    int num_asics = 0;
    bool is_smart_switch_npu = false;
    bool is_smart_switch_dpu = false;
    int num_dpus = 0;
};


/*
 * class SsgMainTest
 * Implements functions to test ssg_main routine.
 */
class SsgMainTest : public SsgFunctionTest {
  protected:
    /* Retrun true if string belongs to a multi instance service */
    bool is_multi_instance(const std::string str) {
        return (str.find("@") != std::string::npos) ? true : false;
    }

    /* Returns true if it is a timer service */
    bool is_timer_service(const std::string str) {
        return (str.find(".timer") != std::string::npos) ? true : false;
    }

    /* Find a string in a file */
    bool find_string_in_file(std::string str,
                             std::string file_name) {
        bool found = false;
        std::string line;

        std::ifstream file(TEST_OUTPUT_DIR + file_name);
        while (getline(file, line) && !found) {
            if (str == line) {
                found = true;
                break;
            }
        }
        return found;
    }

    /* This function validates if a given dependency list for an unit file
     * exists in the unit file as per expected_result. The items in the list
     * should exist if expected_result is true.
     */
    void validate_output_dependency_list(std::vector<std::string> strs,
                              std::string target,
                              bool expected_result,
                              int num_asics) {
        for (std::string str : strs) {
            bool finished = false;
            for (int i = 0 ; i < num_asics && !finished; ++i) {
                auto str_t = str;
                if (is_multi_instance(str)) {
                    /* insert instance id in string */
                    str_t  = (boost::format{str} % i).str();
                } else {
                    /* Run once for single instance */
                    finished = true;
                }
                EXPECT_EQ(find_string_in_file(str_t, target),
                        expected_result)
                        << "Error validating " + str_t + " in " + target;
            }
        }
    }

    void validate_output_dependency_list_ignore_multi_instance(
            std::vector<std::string> strs,
            std::string target,
            bool expected_result) {
        for (std::string str : strs) {
            bool finished = false;
            EXPECT_EQ(find_string_in_file(str, target),
                    expected_result)
                    << "Error validating " + str + " in " + target;
        }
    }

    /* This function validates if unit file paths in the provided
     * list strs exists or not as per expected_result. The unit files
     * should exist if expected_result is true.
     */
    void validate_output_unit_files(std::vector<std::string> strs,
                              std::string target,
                              bool expected_result,
                              int num_instances,
                              bool dev_null_as_inexistent = true) {
        for (std::string str : strs) {
            bool finished = false;
            for (int i = 0 ; i < num_instances && !finished; ++i) {
                auto str_t = str;
                if (is_multi_instance(str)) {
                    /* insert instance id in string */
                    str_t  = (boost::format{str} % i).str();
                } else {
                    /* Run once for single instance */
                    finished = true;
                }
                fs::path path{TEST_OUTPUT_DIR + target + "/" + str_t};
                bool exist = fs::exists(path);
                if (exist) {
                    char resolved_path[PATH_MAX] = { 0 };
                    realpath(path.c_str(), resolved_path);
                    if (strcmp(resolved_path, "/dev/null") == 0) {
                        exist = !dev_null_as_inexistent;
                    }
                }
                EXPECT_EQ(exist, expected_result)
                    << "Failed validation: " << path;
            }
        }
    }

    /*
     * This function validates the generated dependencies in a Unit File.
     */
    void validate_depedency_in_unit_file(const SsgMainConfig &cfg) {
        std::string test_service = "test.service.d/multi-asic-dependencies.conf";

        if (IS_SINGLE_ASIC(cfg.num_asics) && cfg.num_dpus == 0) {
            /* Nothing in this section will apply to single asic, as the file
             * won't be created at all.
             */
            validate_output_dependency_list(common_dependency_list,
                test_service, false, cfg.num_asics);
            return;
        }

        /* Validate Unit file dependency creation for multi instance
         * services. These entries should be present for multi asic
         * system but not present for single asic system.
         */
        validate_output_dependency_list(multi_asic_dependency_list,
            test_service, IS_MULTI_ASIC(cfg.num_asics), cfg.num_asics);

        /* This section handles a tricky scenario.
         * When the number of DPUs (Data Processing Units) is greater than 0,
         * the dependency list will be split. Otherwise, it remains in one line.
         * Despite the split, the final result remains equivalent.
         */
        if (cfg.num_dpus > 0) {
            for (int i = 0; i < cfg.num_dpus; i++) {
                validate_output_dependency_list_ignore_multi_instance(npu_dependency_list,
                    "database@dpu" + std::to_string(i) + ".service.d/ordering.conf", true);
            }
        }

        /* Validate Unit file dependency creation for single instance
         * common services. These entries should not be present for multi
         * and single asic system.
         */
        validate_output_dependency_list(common_dependency_list,
            test_service, true, cfg.num_asics);
    }

    /*
     * This function validates the list of generated Service Unit Files.
     */
    void validate_service_file_generated_list(const SsgMainConfig &cfg) {
        std::string test_target = "multi-user.target.wants";
        validate_output_unit_files(multi_asic_service_list,
            test_target, IS_MULTI_ASIC(cfg.num_asics), cfg.num_asics);
        validate_output_unit_files(single_asic_service_list,
            test_target, IS_SINGLE_ASIC(cfg.num_asics), cfg.num_asics);
        validate_output_unit_files(common_service_list,
            test_target, true, cfg.num_asics);
        validate_output_unit_files(npu_service_list,
            test_target, cfg.is_smart_switch_npu, cfg.num_dpus);
        validate_output_unit_files(dpu_service_list,
            test_target, cfg.is_smart_switch_dpu, cfg.num_dpus);
        validate_output_unit_files(dpu_network_service_list,
            "network", cfg.is_smart_switch_dpu, cfg.num_dpus);
        validate_output_unit_files(non_smart_switch_service_list,
            "system", !cfg.is_smart_switch_npu && !cfg.is_smart_switch_dpu, cfg.num_dpus, false);
    }

    void validate_environment_variable(const SsgMainConfig &cfg) {
        std::unordered_map<std::string, std::string> env_vars;
        env_vars["IS_DPU_DEVICE"] = (cfg.is_smart_switch_dpu ? "true" : "false");
        env_vars["NUM_DPU"] = std::to_string(cfg.num_dpus);

        std::vector<std::string> checked_service_list;

        checked_service_list.insert(checked_service_list.end(), common_service_list.begin(), common_service_list.end());
        if (cfg.num_dpus > 0) {
            checked_service_list.insert(checked_service_list.end(), npu_service_list_for_environment_variables.begin(), npu_service_list_for_environment_variables.end());
        }
        if (cfg.num_asics > 1) {
            checked_service_list.insert(checked_service_list.end(), multi_asic_service_list.begin(), multi_asic_service_list.end());
        }

        for (const auto &target: checked_service_list) {
            if (!target.ends_with(".service")) {
                continue;
            }

            for (const auto& item : env_vars) {
                std::string str = "Environment=\"" + item.first + "=" + item.second + "\"";
                auto target_unit = target;
                if (is_multi_instance(target)) {
                    /* insert instance id in string */
                    target_unit = (boost::format{target_unit} % "").str();
                }
                EXPECT_EQ(find_string_in_file(str, target_unit + ".d/environment.conf"), true)
                    << "Error validating " + str + " in " + target_unit;
            }
        }
    }

    /* ssg_main test routine.
     * input: num_asics    number of asics
     */
    void ssg_main_test(const SsgMainConfig &cfg) {
        FILE* fp;
        std::vector<char*> argv_;
        std::vector<std::string> arguments = {
                    "ssg_main",
                    TEST_OUTPUT_DIR.c_str()
                };
        std::string num_asic_str = "NUM_ASIC=" + std::to_string(cfg.num_asics);

        std::string unit_file_path = fs::current_path().string() + "/" +TEST_UNIT_FILE_PREFIX;
        g_unit_file_prefix = unit_file_path.c_str();
        g_config_file = TEST_CONFIG_FILE.c_str();
        g_machine_config_file = TEST_MACHINE_CONF.c_str();
        g_asic_conf_format = TEST_ASIC_CONF_FORMAT.c_str();
        g_platform_file_format = TEST_PLATFORM_FILE_FORMAT.c_str();
        std::string lib_systemd = fs::current_path().string() + "/" + TEST_UNIT_FILE_PREFIX;
        g_lib_systemd = lib_systemd.c_str();
        std::string etc_systemd = fs::current_path().string() + "/" + TEST_OUTPUT_DIR;
        g_etc_systemd = etc_systemd.c_str();


        /* Set NUM_ASIC value in asic.conf */
        fp = fopen(TEST_ASIC_CONF.c_str(), "w");
        ASSERT_NE(fp, nullptr);
        fputs(num_asic_str.c_str(), fp);
        fclose(fp);

        /* Set platform file for smart switch */
        if (cfg.is_smart_switch_dpu || cfg.is_smart_switch_npu) {
            nlohmann::json platform_config;
            if (cfg.is_smart_switch_dpu) {
                ASSERT_EQ(cfg.num_dpus, 0);
                ASSERT_EQ(cfg.is_smart_switch_npu, false);

                platform_config["DPU"] = nlohmann::json::object();
            }
            else if (cfg.is_smart_switch_npu) {
                ASSERT_EQ(cfg.is_smart_switch_dpu, false);
                nlohmann::json dpus;
                for (int i = 0; i < cfg.num_dpus; i++) {
                    dpus["dpu" + std::to_string(i)] = nlohmann::json::object();
                }
                platform_config["DPUS"] = dpus;
            }
            fp = fopen(TEST_PLATFORM_CONF.c_str(), "w");
            ASSERT_NE(fp, nullptr);
            fputs(platform_config.dump().c_str(), fp);
            fclose(fp);
        }

        /* Create argv list for ssg_main. */
        for (const auto& arg : arguments) {
            argv_.push_back((char*)arg.data());
        }
        argv_.push_back(nullptr);

        /* Call ssg_main */
        EXPECT_EQ(ssg_main(argv_.size(), argv_.data()), 0);

        /* Validate systemd service template creation. */
        validate_service_file_generated_list(cfg);

        /* Validate Test Unit file for dependency creation. */
        validate_depedency_in_unit_file(cfg);

        /* Validate environment variables */
        validate_environment_variable(cfg);
    }

    /* Save global variables before running tests */
    virtual void SetUp() {
        SsgFunctionTest::SetUp();
    }

    /* Restore global vars */
    virtual void TearDown() {
        SsgFunctionTest::TearDown();
    }


  private:
    static const std::vector<std::string> single_asic_service_list;
    static const std::vector<std::string> multi_asic_service_list;
    static const std::vector<std::string> common_service_list;
    static const std::vector<std::string> non_smart_switch_service_list;
    static const std::vector<std::string> npu_service_list;
    static const std::vector<std::string> npu_service_list_for_environment_variables;
    static const std::vector<std::string> dpu_service_list;
    static const std::vector<std::string> dpu_network_service_list;
    static const std::vector<std::string> single_asic_dependency_list;
    static const std::vector<std::string> single_asic_dependency_list_split;
    static const std::vector<std::string> multi_asic_dependency_list;
    static const std::vector<std::string> npu_dependency_list;
    static const std::vector<std::string> common_dependency_list;
};

/*
 * The following list defines the Service unit files symlinks generated by
 * Systemd sonic generator for single and multi asic systems. The test case
 * use these lists to check for presence/absence of unit files based on
 * num_asics value.
 */

/* Systemd service Unit file list for single asic only system */
const std::vector<std::string>
SsgMainTest::single_asic_service_list = {
    "multi_inst_b.service",
};

/* Systemd service Unit file list for multi asic only system.
 * %1% is formatter for boost::format API and replaced by asic num.
 */
const std::vector<std::string>
SsgMainTest::multi_asic_service_list = {
    "multi_inst_a@%1%.service",
    "multi_inst_b@%1%.service",
    "database@%1%.service",
};

/* Common Systemd service Unit file list for single and multi asic system. */
const std::vector<std::string>
SsgMainTest::common_service_list = {
    "multi_inst_a.service",
    "single_inst.service",
    "test.service",
    "database.service",
};

/* Systemd service list for non Smart Switch */
const std::vector<std::string>
SsgMainTest::non_smart_switch_service_list = {
    "systemd-networkd.service"
};


/* Systemd service Unit file list for Smart Switch NPU. */
const std::vector<std::string>
SsgMainTest::npu_service_list = {
    "database@dpu%1%.service",
};

const std::vector<std::string>
SsgMainTest::npu_service_list_for_environment_variables = {
    "database@%1%.service",
};

/* Systemd service Unit file list for Smart Switch DPU. */
const std::vector<std::string>
SsgMainTest::dpu_service_list = {
    "midplane-network-dpu.service",
};

/* Systemd service Unit file list for Smart Switch DPU. */
const std::vector<std::string>
SsgMainTest::dpu_network_service_list = {
    "midplane-network-dpu.network",
};

/*
 * The following list defines the systemd dependencies in a unit file to be
 * varified for single and multi asic systems. Based on num_asics and type of
 * service listed as dependency in Unit file, systemd sonic generator modifies
 * the original unit file, if required, for multi asic system.
 * For example: if test.service file defines a dependency "After=multi_inst_a.service",
 *              as multi_inst_a.service is a multi instance service,
 *              for a system with 2 asics, systemd sonic generator shall modify
 *              test.service to include following dependency strings:
 *              "After=multi_inst_a@0.service"
 *              After=multi_inst_a@1.service"
 */

/* Systemd service Unit file dependency entries for Single asic system. */
const std::vector<std::string>
SsgMainTest::single_asic_dependency_list = {
    "After=multi_inst_a.service multi_inst_b.service",
};

/* Systemd service Unit file dependency entries for Single asic system. */
const std::vector<std::string>
SsgMainTest::single_asic_dependency_list_split = {
    "After=multi_inst_a.service",
    "After=multi_inst_b.service",
};

/* Systemd service Unit file dependency entries for multi asic system. */
const std::vector<std::string>
SsgMainTest::multi_asic_dependency_list = {
    "After=multi_inst_a@%1%.service",
    "After=multi_inst_b@%1%.service",
};

/* Common Systemd service Unit file dependency entries for single and multi asic
 * systems.
 */
const std::vector<std::string>
SsgMainTest::common_dependency_list = {
    "Before=single_inst.service",
};

const std::vector<std::string>
SsgMainTest::npu_dependency_list = {
    "Requires=systemd-networkd-wait-online@bridge-midplane.service",
    "After=systemd-networkd-wait-online@bridge-midplane.service",
};

/* Test get functions for global vasr*/
TEST_F(SystemdSonicGeneratorFixture, get_global_vars) {
    EXPECT_EQ(g_unit_file_prefix, nullptr);
    EXPECT_STREQ(get_unit_file_prefix(), UNIT_FILE_PREFIX);
    g_unit_file_prefix = TEST_UNIT_FILE_PREFIX.c_str();
    EXPECT_STREQ(get_unit_file_prefix(), TEST_UNIT_FILE_PREFIX.c_str());

    EXPECT_EQ(g_config_file, nullptr);
    EXPECT_STREQ(get_config_file(), CONFIG_FILE);
    g_config_file = TEST_CONFIG_FILE.c_str();
    EXPECT_STREQ(get_config_file(), TEST_CONFIG_FILE.c_str());

    EXPECT_EQ(g_machine_config_file, nullptr);
    EXPECT_STREQ(get_machine_config_file(), MACHINE_CONF_FILE);
    g_machine_config_file = TEST_MACHINE_CONF.c_str();
    EXPECT_STREQ(get_machine_config_file(), TEST_MACHINE_CONF.c_str());

    EXPECT_EQ(g_asic_conf_format, nullptr);
    EXPECT_STREQ(get_asic_conf_format(), ASIC_CONF_FORMAT);
    g_asic_conf_format = TEST_ASIC_CONF_FORMAT.c_str();
    EXPECT_STREQ(get_asic_conf_format(), TEST_ASIC_CONF_FORMAT.c_str());

    EXPECT_EQ(g_platform_conf_format, nullptr);
    EXPECT_STREQ(get_platform_conf_format(), PLATFORM_CONF_FORMAT);
    g_platform_conf_format = TEST_PLATFORM_CONF_FORMAT.c_str();
    EXPECT_STREQ(get_platform_conf_format(), TEST_PLATFORM_CONF_FORMAT.c_str());
}

TEST_F(SystemdSonicGeneratorFixture, global_vars) {
    EXPECT_EQ(g_unit_file_prefix, nullptr);
    EXPECT_STREQ(get_unit_file_prefix(), UNIT_FILE_PREFIX);

    EXPECT_EQ(g_config_file, nullptr);
    EXPECT_STREQ(get_config_file(), CONFIG_FILE);

    EXPECT_EQ(g_machine_config_file, nullptr);
    EXPECT_STREQ(get_machine_config_file(), MACHINE_CONF_FILE);
}

/* TEST machine/unit/config if file is missing */
TEST_F(SsgFunctionTest, missing_file) {
    EXPECT_TRUE(fs::exists(TEST_MACHINE_CONF.c_str()));
    EXPECT_TRUE(fs::exists(TEST_UNIT_FILE_PREFIX.c_str()));
    EXPECT_TRUE(fs::exists(TEST_OUTPUT_DIR.c_str()));
    EXPECT_TRUE(fs::exists(TEST_PLATFORM_DIR.c_str()));
    EXPECT_TRUE(fs::exists(TEST_PLATFORM_CONFIG.c_str()));
}

/* TEST insert_instance_number() */
TEST_F(SsgFunctionTest, insert_instance_number) {
    char input[] = "test@.service";
    for (int i = 0; i <= 100; ++i) {
        std::string out = "test@" + std::to_string(i) + ".service";
        std::string ret = insert_instance_number(input, i, "");
        EXPECT_EQ(ret, out);
    }
}

/* TEST get_num_of_asic() */
TEST_F(SsgFunctionTest, get_num_of_asic) {
    FILE* fp;

    g_machine_config_file = TEST_MACHINE_CONF.c_str();
    g_asic_conf_format = TEST_ASIC_CONF_FORMAT.c_str();

    fp = fopen(TEST_ASIC_CONF.c_str(), "w");
    ASSERT_NE(fp, nullptr);
    fputs("NUM_ASIC=1", fp);
    fclose(fp);
    EXPECT_EQ(get_num_of_asic(), 1);

    fp = fopen(TEST_ASIC_CONF.c_str(), "w");
    ASSERT_NE(fp, nullptr);
    fputs("NUM_ASIC=10", fp);
    fclose(fp);
    EXPECT_EQ(get_num_of_asic(), 10);

    fp = fopen(TEST_ASIC_CONF.c_str(), "w");
    ASSERT_NE(fp, nullptr);
    fputs("NUM_ASIC=40", fp);
    fclose(fp);
    EXPECT_EQ(get_num_of_asic(), 40);
}

/* TEST get_unit_files()*/
TEST_F(SsgFunctionTest, get_unit_files) {
    g_unit_file_prefix = TEST_UNIT_FILE_PREFIX.c_str();
    g_lib_systemd = TEST_UNIT_FILE_PREFIX.c_str();
    g_etc_systemd = TEST_OUTPUT_DIR.c_str();
    g_config_file = TEST_CONFIG_FILE.c_str();
    char* unit_files[NUM_UNIT_FILES] = { NULL };
    int num_unit_files = get_unit_files(g_config_file, unit_files, NUM_UNIT_FILES);
    // Exclude the midplane-network-{npu/dpu}.service which is only used for smart switch
    auto non_smart_switch_generated_services = generated_services;
    non_smart_switch_generated_services.erase(
        std::remove(non_smart_switch_generated_services.begin(),
                    non_smart_switch_generated_services.end(),
                    "midplane-network-npu.service"),
        non_smart_switch_generated_services.end());
    non_smart_switch_generated_services.erase(
        std::remove(non_smart_switch_generated_services.begin(),
                    non_smart_switch_generated_services.end(),
                    "midplane-network-dpu.service"),
        non_smart_switch_generated_services.end());
    EXPECT_EQ(num_unit_files, non_smart_switch_generated_services.size());
    for (std::string service : non_smart_switch_generated_services) {
        bool found = false;
        for (auto& unit_file : unit_files) {
            if (unit_file == NULL) {
                continue;
            }
            if(unit_file == service) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "unit file not found: " << service;
    }
}

/* TEST get_platform_unit_files()*/
TEST_F(SsgFunctionTest, get_platform_unit_files) {
    g_unit_file_prefix = TEST_UNIT_FILE_PREFIX.c_str();
    g_config_file = TEST_CONFIG_FILE.c_str();
    g_platform_conf_format = TEST_PLATFORM_CONF_FORMAT.c_str();
    char* unit_files[NUM_UNIT_FILES];
    int num_unit_files = get_platform_unit_files(unit_files, NUM_UNIT_FILES);
    EXPECT_EQ(num_unit_files, 1);
    EXPECT_EQ(std::string(unit_files[0]), "platform_specific.service");
}

/* TEST ssg_main() argv error */
TEST_F(SsgMainTest, ssg_main_argv) {
        std::vector<char*> argv_;
        std::vector<std::string> arguments = {
                    "ssg_main",
                };
        /* Create argv list for ssg_main. */
        for (const auto& arg : arguments) {
            argv_.push_back((char*)arg.data());
        }

        /* Call ssg_main */
        EXPECT_EQ(ssg_main(argv_.size(), argv_.data()), 1);
}

/* TEST ssg_main() single asic */
TEST_F(SsgMainTest, ssg_main_single_npu) {
    SsgMainConfig cfg;
    cfg.num_asics = 1;
    ssg_main_test(cfg);
}

/* TEST ssg_main() multi(10) asic */
TEST_F(SsgMainTest, ssg_main_10_npu) {
    SsgMainConfig cfg;
    cfg.num_asics = 10;
    ssg_main_test(cfg);
}

/* TEST ssg_main() multi(40) asic */
TEST_F(SsgMainTest, ssg_main_40_npu) {
    SsgMainConfig cfg;
    cfg.num_asics = 40;
    ssg_main_test(cfg);
}

TEST_F(SsgMainTest, ssg_main_smart_switch_npu) {
    SsgMainConfig cfg;
    cfg.num_asics = 1;
    cfg.is_smart_switch_npu = true;
    cfg.num_dpus = 8;
    ssg_main_test(cfg);
}

TEST_F(SsgMainTest, ssg_main_smart_switch_dpu) {
    SsgMainConfig cfg;
    cfg.num_asics = 1;
    cfg.is_smart_switch_dpu = true;
    ssg_main_test(cfg);
}

TEST_F(SsgMainTest, ssg_main_smart_switch_double_execution) {
    SsgMainConfig cfg;
    cfg.num_asics = 1;
    cfg.is_smart_switch_npu = true;
    cfg.num_dpus = 8;
    ssg_main_test(cfg);
    ssg_main_test(cfg);
}

}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
