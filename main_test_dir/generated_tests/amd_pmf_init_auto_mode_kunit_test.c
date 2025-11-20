#include <kunit/test.h>
#include <linux/module.h>
#include <linux/kernel.h>

// Assuming the existence of these functions based on context
extern void amd_pmf_load_defaults_auto_mode(struct amd_pmf_dev *dev);
extern void amd_pmf_init_metrics_table(struct amd_pmf_dev *dev);

// Mock structure for amd_pmf_dev
struct amd_pmf_dev {
    // Add fields as needed for testing
    int dummy_field;
};

// Mock implementations of dependencies
static bool load_defaults_called = false;
static bool init_metrics_table_called = false;

void amd_pmf_load_defaults_auto_mode(struct amd_pmf_dev *dev)
{
    load_defaults_called = true;
}

void amd_pmf_init_metrics_table(struct amd_pmf_dev *dev)
{
    init_metrics_table_called = true;
}

// Include the function under test
void amd_pmf_init_auto_mode(struct amd_pmf_dev *dev)
{
    amd_pmf_load_defaults_auto_mode(dev);
    amd_pmf_init_metrics_table(dev);
}

// Test cases
static void test_amd_pmf_init_auto_mode_calls_load_defaults(struct kunit *test)
{
    struct amd_pmf_dev *dev = kunit_kzalloc(test, sizeof(*dev), GFP_KERNEL);
    KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);

    load_defaults_called = false;
    init_metrics_table_called = false;

    amd_pmf_init_auto_mode(dev);

    KUNIT_EXPECT_TRUE(test, load_defaults_called);
}

static void test_amd_pmf_init_auto_mode_calls_init_metrics_table(struct kunit *test)
{
    struct amd_pmf_dev *dev = kunit_kzalloc(test, sizeof(*dev), GFP_KERNEL);
    KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);

    load_defaults_called = false;
    init_metrics_table_called = false;

    amd_pmf_init_auto_mode(dev);

    KUNIT_EXPECT_TRUE(test, init_metrics_table_called);
}

static void test_amd_pmf_init_auto_mode_calls_both_functions(struct kunit *test)
{
    struct amd_pmf_dev *dev = kunit_kzalloc(test, sizeof(*dev), GFP_KERNEL);
    KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);

    load_defaults_called = false;
    init_metrics_table_called = false;

    amd_pmf_init_auto_mode(dev);

    KUNIT_EXPECT_TRUE(test, load_defaults_called);
    KUNIT_EXPECT_TRUE(test, init_metrics_table_called);
}

static struct kunit_case amd_pmf_init_auto_mode_test_cases[] = {
    KUNIT_CASE(test_amd_pmf_init_auto_mode_calls_load_defaults),
    KUNIT_CASE(test_amd_pmf_init_auto_mode_calls_init_metrics_table),
    KUNIT_CASE(test_amd_pmf_init_auto_mode_calls_both_functions),
    {}
};

static struct kunit_suite amd_pmf_init_auto_mode_test_suite = {
    .name = "amd_pmf_init_auto_mode",
    .test_cases = amd_pmf_init_auto_mode_test_cases,
};

kunit_test_suite(amd_pmf_init_auto_mode_test_suite);

MODULE_LICENSE("GPL");