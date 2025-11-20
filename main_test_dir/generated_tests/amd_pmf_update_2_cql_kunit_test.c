#include <kunit/test.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

// Mock the AMD PMF device structure and related definitions
struct amd_pmf_dev {
	struct device *dev;
};

enum amd_pmf_auto_mode {
	AUTO_PERFORMANCE = 0,
	AUTO_PERFORMANCE_ON_LAP,
};

#define AUTO_TRANSITION_TO_PERFORMANCE 0

struct config_store {
	int current_mode;
	struct {
		int target_mode;
	} transition[1]; // Only using index 0
};

static struct config_store config_store;

// Mock function declaration
static int amd_pmf_set_automode_call_count;
static int last_mode_passed_to_set_automode;

static void amd_pmf_set_automode(struct amd_pmf_dev *dev, int mode, void *unused)
{
	amd_pmf_set_automode_call_count++;
	last_mode_passed_to_set_automode = mode;
}

// Include the function under test
void amd_pmf_update_2_cql(struct amd_pmf_dev *dev, bool is_cql_event)
{
	int mode = config_store.current_mode;

	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode =
				   is_cql_event ? AUTO_PERFORMANCE_ON_LAP : AUTO_PERFORMANCE;

	if ((mode == AUTO_PERFORMANCE || mode == AUTO_PERFORMANCE_ON_LAP) &&
	    mode != config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode) {
		mode = config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode;
		amd_pmf_set_automode(dev, mode, NULL);
	}
	dev_dbg(dev->dev, "updated CQL thermals\n");
}

// Test cases
static void test_amd_pmf_update_2_cql_cql_event_true_from_auto_performance(struct kunit *test)
{
	struct amd_pmf_dev dev = { .dev = NULL };
	config_store.current_mode = AUTO_PERFORMANCE;
	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode = AUTO_PERFORMANCE; // Initial value
	amd_pmf_set_automode_call_count = 0;
	last_mode_passed_to_set_automode = -1;

	amd_pmf_update_2_cql(&dev, true);

	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode, AUTO_PERFORMANCE_ON_LAP);
	KUNIT_EXPECT_EQ(test, amd_pmf_set_automode_call_count, 1);
	KUNIT_EXPECT_EQ(test, last_mode_passed_to_set_automode, AUTO_PERFORMANCE_ON_LAP);
}

static void test_amd_pmf_update_2_cql_cql_event_false_from_auto_performance_on_lap(struct kunit *test)
{
	struct amd_pmf_dev dev = { .dev = NULL };
	config_store.current_mode = AUTO_PERFORMANCE_ON_LAP;
	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode = AUTO_PERFORMANCE_ON_LAP; // Initial value
	amd_pmf_set_automode_call_count = 0;
	last_mode_passed_to_set_automode = -1;

	amd_pmf_update_2_cql(&dev, false);

	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode, AUTO_PERFORMANCE);
	KUNIT_EXPECT_EQ(test, amd_pmf_set_automode_call_count, 1);
	KUNIT_EXPECT_EQ(test, last_mode_passed_to_set_automode, AUTO_PERFORMANCE);
}

static void test_amd_pmf_update_2_cql_no_change_needed(struct kunit *test)
{
	struct amd_pmf_dev dev = { .dev = NULL };
	config_store.current_mode = AUTO_PERFORMANCE;
	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode = AUTO_PERFORMANCE; // Same as current
	amd_pmf_set_automode_call_count = 0;

	amd_pmf_update_2_cql(&dev, false);

	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode, AUTO_PERFORMANCE);
	KUNIT_EXPECT_EQ(test, amd_pmf_set_automode_call_count, 0);
}

static void test_amd_pmf_update_2_cql_different_initial_target_mode(struct kunit *test)
{
	struct amd_pmf_dev dev = { .dev = NULL };
	config_store.current_mode = AUTO_PERFORMANCE_ON_LAP;
	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode = AUTO_PERFORMANCE; // Different initially
	amd_pmf_set_automode_call_count = 0;
	last_mode_passed_to_set_automode = -1;

	amd_pmf_update_2_cql(&dev, true);

	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode, AUTO_PERFORMANCE_ON_LAP);
	KUNIT_EXPECT_EQ(test, amd_pmf_set_automode_call_count, 1);
	KUNIT_EXPECT_EQ(test, last_mode_passed_to_set_automode, AUTO_PERFORMANCE_ON_LAP);
}

static void test_amd_pmf_update_2_cql_non_matching_current_mode(struct kunit *test)
{
	struct amd_pmf_dev dev = { .dev = NULL };
	config_store.current_mode = 99; // Not AUTO_PERFORMANCE or AUTO_PERFORMANCE_ON_LAP
	config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode = AUTO_PERFORMANCE;
	amd_pmf_set_automode_call_count = 0;

	amd_pmf_update_2_cql(&dev, true);

	KUNIT_EXPECT_EQ(test, config_store.transition[AUTO_TRANSITION_TO_PERFORMANCE].target_mode, AUTO_PERFORMANCE_ON_LAP);
	KUNIT_EXPECT_EQ(test, amd_pmf_set_automode_call_count, 0);
}

static struct kunit_case amd_pmf_update_2_cql_test_cases[] = {
	KUNIT_CASE(test_amd_pmf_update_2_cql_cql_event_true_from_auto_performance),
	KUNIT_CASE(test_amd_pmf_update_2_cql_cql_event_false_from_auto_performance_on_lap),
	KUNIT_CASE(test_amd_pmf_update_2_cql_no_change_needed),
	KUNIT_CASE(test_amd_pmf_update_2_cql_different_initial_target_mode),
	KUNIT_CASE(test_amd_pmf_update_2_cql_non_matching_current_mode),
	{}
};

static struct kunit_suite amd_pmf_update_2_cql_test_suite = {
	.name = "amd_pmf_update_2_cql_test",
	.test_cases = amd_pmf_update_2_cql_test_cases,
};

kunit_test_suite(amd_pmf_update_2_cql_test_suite);

MODULE_LICENSE("GPL");