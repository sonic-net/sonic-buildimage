#include "gtest/gtest.h"
#include "dbconnector.h"
#include <iostream>

using namespace std;
using namespace swss;

string existing_file = "./tests/redis_multi_db_ut_config/database_config.json";
string nonexisting_file = "./tests/redis_multi_db_ut_config/database_config_nonexisting.json";
string global_existing_file = "./tests/redis_multi_db_ut_config/database_global.json";

#define TEST_DB  "APPL_DB"
#define TEST_NAMESPACE  "asic0"
#define INVALID_NAMESPACE  "invalid"

bool g_is_redis_available = false;

class SwsscommonEnvironment : public ::testing::Environment {
public:
    // Override this to define how to set up the environment
    void SetUp() override {
        // by default , init should be false
        cout<<"Default : isInit = "<<SonicDBConfig::isInit()<<endl;
        EXPECT_FALSE(SonicDBConfig::isInit());

        // load nonexisting file, should throw exception with NO file existing
        try
        {
            cout<<"INIT: loading nonexisting db config file"<<endl;
            SonicDBConfig::initialize(nonexisting_file);
        }
        catch (exception &e)
        {
            EXPECT_TRUE(strstr(e.what(), "Sonic database config file doesn't exist"));
        }
        EXPECT_FALSE(SonicDBConfig::isInit());

        // load local config file, init should be true
        SonicDBConfig::initialize(existing_file);
        cout<<"INIT: load local db config file, isInit = "<<SonicDBConfig::isInit()<<endl;
        EXPECT_TRUE(SonicDBConfig::isInit());

        // Test the database_global.json file
        // by default , global_init should be false
        cout<<"Default : isGlobalInit = "<<SonicDBConfig::isGlobalInit()<<endl;
        EXPECT_FALSE(SonicDBConfig::isGlobalInit());

        // Call an API which actually needs the data populated by SonicDBConfig::initializeGlobalConfig
        try
        {
            cout<<"INIT: Invoking SonicDBConfig::getDbId(APPL_DB, asic0)"<<endl;
            SonicDBConfig::getDbId(TEST_DB, TEST_NAMESPACE);
        }
        catch (exception &e)
        {
            EXPECT_TRUE(strstr(e.what(), "Initialize global DB config using API SonicDBConfig::initializeGlobalConfig"));
        }

        // load local global file, init should be true
        SonicDBConfig::initializeGlobalConfig(global_existing_file);
        cout<<"INIT: load global db config file, isInit = "<<SonicDBConfig::isGlobalInit()<<endl;
        EXPECT_TRUE(SonicDBConfig::isGlobalInit());

        // Call an API with wrong namespace passed
        try
        {
            cout<<"INIT: Invoking SonicDBConfig::getDbId(APPL_DB, invalid)"<<endl;
            SonicDBConfig::getDbId(TEST_DB, INVALID_NAMESPACE);
        }
        catch (exception &e)
        {
            EXPECT_TRUE(strstr(e.what(), "Namespace invalid is not a valid namespace name in config file"));
        }

        // Get this info handy
        try
        {
            DBConnector db("TEST_DB", 0, true);
            g_is_redis_available = true;
        }
        catch (exception &e)
        {
            printf("Unable to get DB Connector, e=(%s)\n", e.what());
        }
    }
};


int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    // Registers a global test environment, and verifies that the
    // registration function returns its argument.
    SwsscommonEnvironment* const env = new SwsscommonEnvironment;
    testing::AddGlobalTestEnvironment(env);
    return RUN_ALL_TESTS();
}
