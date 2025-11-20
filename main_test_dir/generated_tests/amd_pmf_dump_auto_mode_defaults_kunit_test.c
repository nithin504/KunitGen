#include <kunit/test.h>
#include <linux/kernel.h>
#include <linux/slab.h>

struct auto_mode_mode_config {
    int mode;
    int param1;
    int param2;
};

static void amd_pmf_dump_auto_mode_defaults(struct auto_mode_mode_config *data)
{
    if (!data)
        return;

    data->mode = 0;
    data->param1 = 0;
    data->param2 = 0;
}

static void test_amd_pmf_dump_auto_mode_defaults_null_input(struct kunit *test)
{
    amd_pmf_dump_auto_mode_defaults(NULL);
    KUNIT_EXPECT_PTR_EQ(test, NULL, (void *)NULL);
}

static void test_amd_pmf_dump_auto_mode_defaults_valid_input(struct kunit *test)
{
    struct auto_mode_mode_config *data = kunit_kzalloc(test, sizeof(*data), GFP_KERNEL);
    KUNIT_ASSERT_NOT_ERR_OR_NULL(test, data);

    data->mode = 1;
    data->param1 = 2;
    data->param2 = 3;

    amd_pmf_dump_auto_mode_defaults(data);

    KUNIT_EXPECT_EQ(test, data->mode, 0);
    KUNIT_EXPECT_EQ(test, data->param1, 0);
    KUNIT_EXPECT_EQ(test, data->param2, 0);
}

static struct kunit_case amd_pmf_test_cases[] = {
    KUNIT_CASE(test_amd_pmf_dump_auto_mode_defaults_null_input),
    KUNIT_CASE(test_amd_pmf_dump_auto_mode_defaults_valid_input),
    {}
};

static struct kunit_suite amd_pmf_test_suite = {
    .name = "amd_pmf_dump_auto_mode_defaults_test",
    .test_cases = amd_pmf_test_cases,
};

kunit_test_suite(amd_pmf_test_suite);