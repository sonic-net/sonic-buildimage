#include "gtest/gtest.h"
#include <iostream>

class FibEnvironment : public ::testing::Environment {
public:
    // Override this to define how to set up the environment.
    void SetUp() override {
    }
};

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    FibEnvironment* const env = new FibEnvironment;
    testing::AddGlobalTestEnvironment(env);
    return RUN_ALL_TESTS();
}
