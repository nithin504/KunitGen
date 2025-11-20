#include <kunit/test.h>
#include <linux/module.h>
#include <linux/kernel.h>

// Mock the config_store structure and related definitions
#define AUTO_TRANSITION_TO_QUIET 0
#define AUTO_TRANSITION_TO_PERFORMANCE 1
#define AUTO_TRANSITION_FROM_QUIET_TO_BALANCE 2
#define AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE 3
#define AUTO_BALANCE 0
#define AUTO_QUIET 1
#define AUTO_PERFORMANCE 2

struct transition_config {
    unsigned int power_threshold;
    unsigned int power_delta;
};

struct mode_set_config {
    unsigned int power_floor;
};

struct global_config_store {
    struct transition_config transition[4];
    struct mode_set_config mode_set[3];
} config_store;

// Include the function under test
static void amd_pmf_get_power_threshold(void)
{
    config_store.transition[AUTO_TRANSITION_TO_QUIET].power_threshold =
                config_store.mode_set[AUTO_BALANCE].power_floor -
                config_store.transition[AUTO_TRANSITION_TO_QUIET].power_delta;

    config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_threshold =
                config_store.mode_set[AUTO_BALANCE].power_floor -
                config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_delta;

    config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_threshold =
            config_store.mode_set[AUTO_QUIET].power_floor -
            config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_delta;

    config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_threshold =
        config_store.mode_set[AUTO_PERFORMANCE].power_floor -
        config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_delta;

#ifdef CONFIG_AMD_PMF_DEBUG
    pr_debug("[AUTO MODE TO_QUIET] pt: %u mW pf: %u mW pd: %u mW\n",
         config_store.transition[AUTO_TRANSITION_TO_QUIET].power_threshold,
         config_store.mode_set[AUTO_BALANCE].power_floor,
         config_store.transition[AUTO_TRANSITION_TO_QUIET].power_delta);

    pr_debug("[AUTO MODE TO_PERFORMANCE] pt: %u mW pf: %u mW pd: %u mW\n",
         config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_threshold,
         config_store.mode_set[AUTO_BALANCE].power_floor,
         config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_delta);

    pr_debug("[AUTO MODE QUIET_TO_BALANCE] pt: %u mW pf: %u mW pd: %u mW\n",
         config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE]
         .power_threshold,
         config_store.mode_set[AUTO_QUIET].power_floor,
         config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_delta);

    pr_debug("[AUTO MODE PERFORMANCE_TO_BALANCE] pt: %u mW pf: %u mW pd: %u mW\n",
         config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE]
         .power_threshold,
         config_store.mode_set[AUTO_PERFORMANCE].power_floor,
         config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_delta);
#endif
}

// Test cases
static void test_amd_pmf_get_power_threshold_basic(struct kunit *test)
{
    // Setup initial values
    config_store.mode_set[AUTO_BALANCE].power_floor = 1000;
    config_store.mode_set[AUTO_QUIET].power_floor = 500;
    config_store.mode_set[AUTO_PERFORMANCE].power_floor = 1500;
    
    config_store.transition[AUTO_TRANSITION_TO_QUIET].power_delta = 100;
    config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_delta = 200;
    config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_delta = 50;
    config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_delta = 150;

    // Call the function
    amd_pmf_get_power_threshold();

    // Verify results
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_QUIET].power_threshold, 900U);
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_threshold, 800U);
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_threshold, 450U);
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_threshold, 1350U);
}

static void test_amd_pmf_get_power_threshold_zero_deltas(struct kunit *test)
{
    // Setup initial values with zero deltas
    config_store.mode_set[AUTO_BALANCE].power_floor = 1000;
    config_store.mode_set[AUTO_QUIET].power_floor = 500;
    config_store.mode_set[AUTO_PERFORMANCE].power_floor = 1500;
    
    config_store.transition[AUTO_TRANSITION_TO_QUIET].power_delta = 0;
    config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_delta = 0;
    config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_delta = 0;
    config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_delta = 0;

    // Call the function
    amd_pmf_get_power_threshold();

    // Verify results
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_QUIET].power_threshold, 1000U);
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_threshold, 1000U);
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_threshold, 500U);
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_threshold, 1500U);
}

static void test_amd_pmf_get_power_threshold_large_values(struct kunit *test)
{
    // Setup large initial values
    config_store.mode_set[AUTO_BALANCE].power_floor = 100000;
    config_store.mode_set[AUTO_QUIET].power_floor = 50000;
    config_store.mode_set[AUTO_PERFORMANCE].power_floor = 150000;
    
    config_store.transition[AUTO_TRANSITION_TO_QUIET].power_delta = 10000;
    config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_delta = 20000;
    config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_delta = 5000;
    config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_delta = 15000;

    // Call the function
    amd_pmf_get_power_threshold();

    // Verify results
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_QUIET].power_threshold, 90000U);
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].power_threshold, 80000U);
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_QUIET_TO_BALANCE].power_threshold, 45000U);
    KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_FROM_PERFORMANCE_TO_BALANCE].power_threshold, 135000U);
}

static struct kunit_case amd_pmf_power_threshold_test_cases[] = {
    KUNIT_CASE(test_amd_pmf_get_power_threshold_basic),
    KUNIT_CASE(test_amd_pmf_get_power_threshold_zero_deltas),
    KUNIT_CASE(test_amd_pmf_get_power_threshold_large_values),
    {}
};

static struct kunit_suite amd_pmf_power_threshold_test_suite = {
    .name = "amd_pmf_power_threshold",
    .test_cases = amd_pmf_power_threshold_test_cases,
};

kunit_test_suite(amd_pmf_power_threshold_test_suite);

MODULE_LICENSE("GPL");