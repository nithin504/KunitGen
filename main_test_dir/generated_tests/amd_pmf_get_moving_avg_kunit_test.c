#include <kunit/test.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define AVG_SAMPLE_SIZE 8

struct amd_pmf_dev {
	int socket_power_history[AVG_SAMPLE_SIZE];
	int socket_power_history_idx;
};

static int amd_pmf_get_moving_avg(struct amd_pmf_dev *pdev, int socket_power);

static void test_amd_pmf_get_moving_avg_initial_fill(struct kunit *test)
{
	struct amd_pmf_dev *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	int result;
	int expected_avg = 100;
	int i;

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);

	pdev->socket_power_history_idx = -1;

	result = amd_pmf_get_moving_avg(pdev, expected_avg);

	KUNIT_EXPECT_EQ(test, result, expected_avg);

	for (i = 0; i < AVG_SAMPLE_SIZE; i++) {
		KUNIT_EXPECT_EQ(test, pdev->socket_power_history[i], expected_avg);
	}
}

static void test_amd_pmf_get_moving_avg_wrap_around(struct kunit *test)
{
	struct amd_pmf_dev *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	int result;
	int i;
	int new_value = 200;

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);

	// Pre-fill the history with 100
	pdev->socket_power_history_idx = -1;
	amd_pmf_get_moving_avg(pdev, 100);

	// Add enough values to cause wrap-around
	for (i = 0; i < AVG_SAMPLE_SIZE - 1; i++) {
		amd_pmf_get_moving_avg(pdev, 100);
	}

	// Add one more value that should overwrite the first entry
	result = amd_pmf_get_moving_avg(pdev, new_value);

	// Check average is correct after wrap-around
	int total = (100 * (AVG_SAMPLE_SIZE - 1)) + new_value;
	int expected_avg = total / AVG_SAMPLE_SIZE;
	KUNIT_EXPECT_EQ(test, result, expected_avg);

	// Verify the new value is at the current index
	int idx = (pdev->socket_power_history_idx) % AVG_SAMPLE_SIZE;
	KUNIT_EXPECT_EQ(test, pdev->socket_power_history[idx], new_value);
}

static void test_amd_pmf_get_moving_avg_sequential_values(struct kunit *test)
{
	struct amd_pmf_dev *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	int result;
	int values[] = {10, 20, 30, 40, 50, 60, 70, 80};
	int i;
	int expected_avg;

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);

	pdev->socket_power_history_idx = -1;

	for (i = 0; i < AVG_SAMPLE_SIZE; i++) {
		result = amd_pmf_get_moving_avg(pdev, values[i]);
	}

	// After filling all samples, check average
	expected_avg = 0;
	for (i = 0; i < AVG_SAMPLE_SIZE; i++) {
		expected_avg += values[i];
	}
	expected_avg /= AVG_SAMPLE_SIZE;

	KUNIT_EXPECT_EQ(test, result, expected_avg);
}

static void test_amd_pmf_get_moving_avg_negative_values(struct kunit *test)
{
	struct amd_pmf_dev *pdev = kunit_kzalloc(test, sizeof(*pdev), GFP_KERNEL);
	int result;
	int values[] = {-10, -20, -30, -40, -50, -60, -70, -80};
	int i;
	int expected_avg;

	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, pdev);

	pdev->socket_power_history_idx = -1;

	for (i = 0; i < AVG_SAMPLE_SIZE; i++) {
		result = amd_pmf_get_moving_avg(pdev, values[i]);
	}

	expected_avg = 0;
	for (i = 0; i < AVG_SAMPLE_SIZE; i++) {
		expected_avg += values[i];
	}
	expected_avg /= AVG_SAMPLE_SIZE;

	KUNIT_EXPECT_EQ(test, result, expected_avg);
}

static struct kunit_case amd_pmf_get_moving_avg_test_cases[] = {
	KUNIT_CASE(test_amd_pmf_get_moving_avg_initial_fill),
	KUNIT_CASE(test_amd_pmf_get_moving_avg_wrap_around),
	KUNIT_CASE(test_amd_pmf_get_moving_avg_sequential_values),
	KUNIT_CASE(test_amd_pmf_get_moving_avg_negative_values),
	{}
};

static struct kunit_suite amd_pmf_get_moving_avg_test_suite = {
	.name = "amd_pmf_get_moving_avg",
	.test_cases = amd_pmf_get_moving_avg_test_cases,
};

kunit_test_suite(amd_pmf_get_moving_avg_test_suite);

MODULE_LICENSE("GPL");

// Implementation of the function under test
static int amd_pmf_get_moving_avg(struct amd_pmf_dev *pdev, int socket_power)
{
	int i, total = 0;

	if (pdev->socket_power_history_idx == -1) {
		for (i = 0; i < AVG_SAMPLE_SIZE; i++)
			pdev->socket_power_history[i] = socket_power;
	}

	pdev->socket_power_history_idx = (pdev->socket_power_history_idx + 1) % AVG_SAMPLE_SIZE;
	pdev->socket_power_history[pdev->socket_power_history_idx] = socket_power;

	for (i = 0; i < AVG_SAMPLE_SIZE; i++)
		total += pdev->socket_power_history[i];

	return total / AVG_SAMPLE_SIZE;
}