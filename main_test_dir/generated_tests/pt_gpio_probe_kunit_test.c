#include <kunit/test.h>
#include <linux/io.h>
#include <linux/errno.h>
#include "gpio-amdpt.c"

#define MOCK_BASE_ADDR 0x1000
#define INTERNAL_GPIO0_DEBOUNCE 0x2
#define DB_TYPE_REMOVE_GLITCH 0x1
#define DB_CNTRL_OFF 28
#define DB_TMR_OUT_MASK 0xFF
#define DB_TMR_LARGE_OFF 9
#define DB_CNTRL_MASK 0x7

// Mock register offsets (these would normally come from pinctrl-amd.h)
#define WAKE_INT_MASTER_REG 0x100

static void test_amd_pt_gpio_direction_input_valid(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_direction_input(gc, 10);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_direction_input_invalid_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_direction_input(gc, 70); // Invalid pin
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_pt_gpio_direction_input_null_chip(struct kunit *test)
{
	int ret;

	ret = amd_pt_gpio_direction_input(NULL, 10);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_pt_gpio_direction_input_edge_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	// Test ngpio-1 (valid edge case)
	ret = amd_pt_gpio_direction_input(gc, 63);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Test base (valid edge case)
	ret = amd_pt_gpio_direction_input(gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_direction_output_valid(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_direction_output(gc, 10, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_direction_output_invalid_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_direction_output(gc, 70, 1); // Invalid pin
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_pt_gpio_direction_output_null_chip(struct kunit *test)
{
	int ret;

	ret = amd_pt_gpio_direction_output(NULL, 10, 1);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_pt_gpio_direction_output_edge_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	// Test ngpio-1 (valid edge case)
	ret = amd_pt_gpio_direction_output(gc, 63, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Test base (valid edge case)
	ret = amd_pt_gpio_direction_output(gc, 0, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_get_value_valid(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_get_value(gc, 10);
	KUNIT_EXPECT_EQ(test, ret, 0); // Default return value when no hardware
}

static void test_amd_pt_gpio_get_value_invalid_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_get_value(gc, 70); // Invalid pin
	KUNIT_EXPECT_EQ(test, ret, 0); // Should still return 0 for invalid pins
}

static void test_amd_pt_gpio_get_value_null_chip(struct kunit *test)
{
	int ret;

	ret = amd_pt_gpio_get_value(NULL, 10);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_get_value_edge_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	// Test ngpio-1 (valid edge case)
	ret = amd_pt_gpio_get_value(gc, 63);
	KUNIT_EXPECT_EQ(test, ret, 0);

	// Test base (valid edge case)
	ret = amd_pt_gpio_get_value(gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_set_value_valid(struct kunit *test)
{
	struct gpio_chip *gc;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	// These should not crash or cause issues
	amd_pt_gpio_set_value(gc, 10, 1);
	amd_pt_gpio_set_value(gc, 10, 0);
}

static void test_amd_pt_gpio_set_value_invalid_pin(struct kunit *test)
{
	struct gpio_chip *gc;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	// Should not crash with invalid pin
	amd_pt_gpio_set_value(gc, 70, 1);
}

static void test_amd_pt_gpio_set_value_null_chip(struct kunit *test)
{
	// Should not crash with NULL chip
	amd_pt_gpio_set_value(NULL, 10, 1);
}

static void test_amd_pt_gpio_set_value_edge_pin(struct kunit *test)
{
	struct gpio_chip *gc;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	// Test ngpio-1 (valid edge case)
	amd_pt_gpio_set_value(gc, 63, 1);
	amd_pt_gpio_set_value(gc, 63, 0);

	// Test base (valid edge case)
	amd_pt_gpio_set_value(gc, 0, 1);
	amd_pt_gpio_set_value(gc, 0, 0);
}

static struct kunit_case amd_pt_gpio_test_cases[] = {
	KUNIT_CASE(test_amd_pt_gpio_direction_input_valid),
	KUNIT_CASE(test_amd_pt_gpio_direction_input_invalid_pin),
	KUNIT_CASE(test_amd_pt_gpio_direction_input_null_chip),
	KUNIT_CASE(test_amd_pt_gpio_direction_input_edge_pin),
	KUNIT_CASE(test_amd_pt_gpio_direction_output_valid),
	KUNIT_CASE(test_amd_pt_gpio_direction_output_invalid_pin),
	KUNIT_CASE(test_amd_pt_gpio_direction_output_null_chip),
	KUNIT_CASE(test_amd_pt_gpio_direction_output_edge_pin),
	KUNIT_CASE(test_amd_pt_gpio_get_value_valid),
	KUNIT_CASE(test_amd_pt_gpio_get_value_invalid_pin),
	KUNIT_CASE(test_amd_pt_gpio_get_value_null_chip),
	KUNIT_CASE(test_amd_pt_gpio_get_value_edge_pin),
	KUNIT_CASE(test_amd_pt_gpio_set_value_valid),
	KUNIT_CASE(test_amd_pt_gpio_set_value_invalid_pin),
	KUNIT_CASE(test_amd_pt_gpio_set_value_null_chip),
	KUNIT_CASE(test_amd_pt_gpio_set_value_edge_pin),
	{}
};

static struct kunit_suite amd_pt_gpio_test_suite = {
	.name = "amd_pt_gpio",
	.test_cases = amd_pt_gpio_test_cases,
};

kunit_test_suite(amd_pt_gpio_test_suite);