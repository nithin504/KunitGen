#include <kunit/test.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

struct amd_pmf_dev {
	struct delayed_work work_buffer;
};

extern void amd_pmf_deinit_auto_mode(struct amd_pmf_dev *dev);

static void test_amd_pmf_deinit_auto_mode_cancel_work(struct kunit *test)
{
	struct amd_pmf_dev *dev;

	dev = kunit_kzalloc(test, sizeof(*dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);

	INIT_DELAYED_WORK(&dev->work_buffer, NULL);

	/* Simulate scheduling the work */
	schedule_delayed_work(&dev->work_buffer, HZ);

	/* Call the function under test */
	amd_pmf_deinit_auto_mode(dev);

	/* Verify that the work is no longer pending */
	KUNIT_EXPECT_FALSE(test, delayed_work_pending(&dev->work_buffer));
}

static void test_amd_pmf_deinit_auto_mode_no_work_scheduled(struct kunit *test)
{
	struct amd_pmf_dev *dev;

	dev = kunit_kzalloc(test, sizeof(*dev), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);

	INIT_DELAYED_WORK(&dev->work_buffer, NULL);

	/* Do not schedule any work */

	/* Call the function under test */
	amd_pmf_deinit_auto_mode(dev);

	/* Verify that the work is still not pending */
	KUNIT_EXPECT_FALSE(test, delayed_work_pending(&dev->work_buffer));
}

static struct kunit_case amd_pmf_deinit_auto_mode_test_cases[] = {
	KUNIT_CASE(test_amd_pmf_deinit_auto_mode_cancel_work),
	KUNIT_CASE(test_amd_pmf_deinit_auto_mode_no_work_scheduled),
	{}
};

static struct kunit_suite amd_pmf_deinit_auto_mode_test_suite = {
	.name = "amd_pmf_deinit_auto_mode",
	.test_cases = amd_pmf_deinit_auto_mode_test_cases,
};

kunit_test_suite(amd_pmf_deinit_auto_mode_test_suite);