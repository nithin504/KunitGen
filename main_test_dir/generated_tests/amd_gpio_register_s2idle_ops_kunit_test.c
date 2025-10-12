#include <kunit/test.h>

static inline void amd_gpio_register_s2idle_ops(void) {}

static void test_amd_gpio_register_s2idle_ops_normal(struct kunit *test)
{
	amd_gpio_register_s2idle_ops();
	KUNIT_SUCCEED(test);
}

static struct kunit_case amd_gpio_register_s2idle_ops_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_register_s2idle_ops_normal),
	{}
};

static struct kunit_suite amd_gpio_register_s2idle_ops_test_suite = {
	.name = "amd_gpio_register_s2idle_ops_test",
	.test_cases = amd_gpio_register_s2idle_ops_test_cases,
};

kunit_test_suite(amd_gpio_register_s2idle_ops_test_suite);
