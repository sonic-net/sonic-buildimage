/*------------------------------------------------------------------
 * ssg-test.cc - systemd-sonic-generator Unit Test
 *
 * Initial: Apr 2021
 *
 * Copyright (c) 2021 by Cisco Systems, Inc.
 *------------------------------------------------------------------
 */
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include "systemd-sonic-generator.h"

namespace fs = boost::filesystem;

namespace SSGTest {
#define IS_MULTI_ASIC(x)  ((x) > 1)
#define IS_SINGLE_ASIC(x) ((x) <= 1)
#define NUM_UNIT_FILES 6

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
const std::string TEST_ASIC_CONF_FORMAT = TEST_ROOT_DIR + "%s/asic.conf";
const std::string TEST_MACHINE_CONF = TEST_ROOT_DIR + "machine.conf";

const std::string TEST_PLATFORM_DIR = TEST_ROOT_DIR + "test_platform/";
const std::string TEST_ASIC_CONF = TEST_PLATFORM_DIR + "asic.conf";

const std::string TEST_OUTPUT_DIR = TEST_ROOT_DIR + "generator/";

const std::string TEST_CONFIG_FILE = TEST_ROOT_DIR + "generated_services.conf";

const std::string TEST_UNIT_FILES = "tests/testfiles/";

/* Input data for generated_services.conf */
const std::vector<std::string> generated_services = {
    "multi_inst_a.service",     /* Single instance of a  multi asic service a */
    "multi_inst_a@.service",    /* Multi-instance of a multi asic service a */
    "multi_inst_b@.service",    /* Multi-instance of a multi asic service b */
    "single_inst.service",      /* A single instance service */
    "test.service",             /* A single instance test service
                                   to test dependency creation */
    "test.timer",               /* A timer service */
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
    }

    /* Restore global vars */
    virtual void TearDown() {
        g_unit_file_prefix = unit_file_prefix_;
        g_config_file = config_file_;
        g_machine_config_file = machine_config_file_;
        g_asic_conf_format = asic_conf_format_;

        g_ssg_test_mutex.unlock();
    }

  private:
    const char* unit_file_prefix_;
    const char* config_file_;
    const char* machine_config_file_;
    const char* asic_conf_format_;
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
                    /* Copy file */
                    fs::copy_file( current, dest_dir / current.filename());
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
        fp = fopen(TEST_MACHINE_CONF.c_str(), "w");
        ASSERT_NE(fp, nullptr);
        fputs("onie_platform=test_platform", fp);
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
                             std::string file_name,
                             int num_asics) {
        bool found = false;
            std::string line;

            std::ifstream file(TEST_UNIT_FILE_PREFIX + file_name);
            while (getline(file, line) && !found) {
                if (str == line) {
                    found = true;
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
                EXPECT_EQ(find_string_in_file(str_t, target, num_asics),
                        expected_result)
                        << "Error validating " + str_t + " in " + target;
            }
        }
    }

    /* This function validates if unit file paths in the provided
     * list strs exists or not as per expected_result. The unit files
     * should exist if expected_result is true.
     */
    void validate_output_unit_files(std::vector<std::string> strs,
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
                fs::path path{TEST_OUTPUT_DIR + target + "/" + str_t};
                EXPECT_EQ(fs::exists(path), expected_result)
                    << "Failed validation: " << path;
            }
        }
    }

    /*
     * This function validates the generated dependencies in a Unit File.
     */
    void validate_depedency_in_unit_file(int num_asics) {
        std::string test_service = "test.service";

        /* Validate Unit file dependency creation for multi instance
         * services. These entries should be present for multi asic
         * system but not present for single asic system.
         */
        validate_output_dependency_list(multi_asic_dependency_list,
            test_service, IS_MULTI_ASIC(num_asics), num_asics);

        /* Validate Unit file dependency creation for single instance
         * services. These entries should not be present for multi asic
         * system but present for single asic system.
         */
        validate_output_dependency_list(single_asic_dependency_list,
            test_service, IS_SINGLE_ASIC(num_asics), num_asics);

        /* Validate Unit file dependency creation for single instance
         * common services. These entries should not be present for multi
         * and single asic system.
         */
        validate_output_dependency_list(common_dependency_list,
            test_service, true, num_asics);
    }

    /*
     * This function validates the list of generated Service Unit Files.
     */
    void validate_service_file_generated_list(int num_asics) {
        std::string test_target = "multi-user.target.wants";
        validate_output_unit_files(multi_asic_service_list,
            test_target, IS_MULTI_ASIC(num_asics), num_asics);
        validate_output_unit_files(single_asic_service_list,
            test_target, IS_SINGLE_ASIC(num_asics), num_asics);
        validate_output_unit_files(common_service_list,
            test_target, true, num_asics);
    }

    /* ssg_main test routine.
     * input: num_asics    number of asics
     */
    void ssg_main_test(int num_asics) {
        FILE* fp;
        std::vector<char*> argv_;
        std::vector<std::string> arguments = {
                    "ssg_main",
                    TEST_OUTPUT_DIR.c_str()
                };
        std::string num_asic_str = "NUM_ASIC=" + std::to_string(num_asics);

        std::string unit_file_path = fs::current_path().string() + "/" +TEST_UNIT_FILE_PREFIX;
        g_unit_file_prefix = unit_file_path.c_str();
        g_config_file = TEST_CONFIG_FILE.c_str();
        g_machine_config_file = TEST_MACHINE_CONF.c_str();
        g_asic_conf_format = TEST_ASIC_CONF_FORMAT.c_str();

        /* Set NUM_ASIC value in asic.conf */
        fp = fopen(TEST_ASIC_CONF.c_str(), "w");
        ASSERT_NE(fp, nullptr);
        fputs(num_asic_str.c_str(), fp);
        fclose(fp);

        /* Create argv list for ssg_main. */
        for (const auto& arg : arguments) {
            argv_.push_back((char*)arg.data());
        }
        argv_.push_back(nullptr);

        /* Call ssg_main */
        EXPECT_EQ(ssg_main(argv_.size(), argv_.data()), 0);

        /* Validate systemd service template creation. */
        validate_service_file_generated_list(num_asics);

        /* Validate Test Unit file for dependency creation. */
        validate_depedency_in_unit_file(num_asics);
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
    static const std::vector<std::string> single_asic_dependency_list;
    static const std::vector<std::string> multi_asic_dependency_list;
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
};

/* Common Systemd service Unit file list for single and multi asic system. */
const std::vector<std::string>
SsgMainTest::common_service_list = {
    "multi_inst_a.service",
    "single_inst.service",
    "test.service",

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
}

/* TEST insert_instance_number() */
TEST_F(SsgFunctionTest, insert_instance_number) {
    char input[] = "test@.service";
    for (int i = 0; i <= 100; ++i) {
        std::string out = "test@" + std::to_string(i) + ".service";
        char* ret = insert_instance_number(input, i);
        ASSERT_NE(ret, nullptr);
        EXPECT_STREQ(ret, out.c_str());
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
    g_config_file = TEST_CONFIG_FILE.c_str();
    char* unit_files[NUM_UNIT_FILES];
    int num_unit_files = get_unit_files(unit_files);
    EXPECT_EQ(num_unit_files, NUM_UNIT_FILES);
    for (std::string service : generated_services) {
        bool found = false;
        for (auto& unit_file : unit_files) {
            if(unit_file == service) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "unit file not found: " << service;
    }
}

/* TEST ssg_main() argv error */
TEST_F(SsgMainTest, ssg_main_argv) {
        FILE* fp;
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
    ssg_main_test(1);
}

/* TEST ssg_main() multi(10) asic */
TEST_F(SsgMainTest, ssg_main_10_npu) {
    ssg_main_test(10);
}

/* TEST ssg_main() multi(40) asic */
TEST_F(SsgMainTest, ssg_main_40_npu) {
    ssg_main_test(40);
}
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
