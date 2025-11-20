#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/device.h>

// Mocking required structures
struct amd_pmf_dev {
	struct device *dev;
	// Add other fields if needed for full functionality
};

// Assume these are defined elsewhere or will be mocked appropriately
bool is_apmf_func_supported(struct amd_pmf_dev *dev, int func);
void amd_pmf_set_sps_power_limits(struct amd_pmf_dev *dev);
void dev_dbg(const struct device *dev, const char *fmt, ...);

#define APMF_FUNC_STATIC_SLIDER_GRANULAR 0x1234 // Placeholder value

static int mock_is_apmf_func_supported = 0;
static int mock_amd_pmf_set_sps_power_limits_called = 0;

bool is_apmf_func_supported(struct amd_pmf_dev *dev, int func)
{
	return mock_is_apmf_func_supported;
}

void amd_pmf_set_sps_power_limits(struct amd_pmf_dev *dev)
{
	mock_amd_pmf_set_sps_power_limits_called++;
}

void dev_dbg(const struct device *dev, const char *fmt, ...)
{
	// Debug logging can be ignored in tests
}

// Include the function under test
int amd_pmf_reset_amt(struct amd_pmf_dev *dev)
{
	if (is_apmf_func_supported(dev, APMF_FUNC_STATIC_SLIDER_GRANULAR)) {
		dev_dbg(dev->dev, "resetting AMT thermals\n");
		amd_pmf_set_sps_power_limits(dev);
	}
	return 0;
}

// Test cases
static void test_amd_pmf_reset_amt_func_supported(struct kunit *test)
{
	struct amd_pmf_dev *dev = kunit_kzalloc(test, sizeof(*dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);

	mock_is_apmf_func_supported = 1;
	mock_amd_pmf_set_sps_power_limits_called = 0;

	int ret = amd_pmf_reset_amt(dev);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_amd_pmf_set_sps_power_limits_called, 1);
}

static void test_amd_pmf_reset_amt_func_not_supported(struct kunit *test)
{
	struct amd_pmf_dev *dev = kunit_kzalloc(test, sizeof(*dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);

	mock_is_apmf_func_supported = 0;
	mock_amd_pmf_set_sps_power_limits_called = 0;

	int ret = amd_pmf_reset_amt(dev);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, mock_amd_pmf_set_sps_power_limits_called, 0);
}

static struct kunit_case amd_pmf_reset_amt_test_cases[] = {
	KUNIT_CASE(test_amd_pmf_reset_amt_func_supported),
	KUNIT_CASE(test_amd_pmf_reset_amt_func_not_supported),
	{}
};

static struct kunit_suite amd_pmf_reset_amt_test_suite = {
	.name = "amd_pmf_reset_amt_test",
	.test_cases = amd_pmf_reset_amt_test_cases,
};

kunit_test_suite(amd_pmf_reset_amt_test_suite);