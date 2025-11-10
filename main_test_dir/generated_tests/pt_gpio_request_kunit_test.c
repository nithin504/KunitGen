#include <kunit/test.h>
#include <linux/io.h>
#include <linux/gfp.h>
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

	ret = amd_pt_gpio_direction_input(gc, 100); // Invalid pin
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

	ret = amd_pt_gpio_direction_input(gc, 63); // Edge case pin
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_direction_input_zero_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_direction_input(gc, 0); // Zero pin
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_get_valid(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_get(gc, 10);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_get_invalid_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_get(gc, 100); // Invalid pin
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_pt_gpio_get_edge_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_get(gc, 63); // Edge case pin
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_get_zero_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_get(gc, 0); // Zero pin
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

	ret = amd_pt_gpio_direction_output(gc, 100, 1); // Invalid pin
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

	ret = amd_pt_gpio_direction_output(gc, 63, 0); // Edge case pin
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_direction_output_zero_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_direction_output(gc, 0, 1); // Zero pin
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_set_valid(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_set(gc, 10, 1);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_set_invalid_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_set(gc, 100, 1); // Invalid pin
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_pt_gpio_set_edge_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_set(gc, 63, 0); // Edge case pin
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_pt_gpio_set_zero_pin(struct kunit *test)
{
	struct gpio_chip *gc;
	int ret;

	gc = kunit_kzalloc(test, sizeof(*gc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, gc);

	gc->ngpio = 64;
	gc->base = 0;

	ret = amd_pt_gpio_set(gc, 0, 1); // Zero pin
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_pt_gpio_test_cases[] = {
	KUNIT_CASE(test_amd_pt_gpio_direction_input_valid),
	KUNIT_CASE(test_amd_pt_gpio_direction_input_invalid_pin),
	KUNIT_CASE(test_amd_pt_gpio_direction_input_edge_pin),
	KUNIT_CASE(test_amd_pt_gpio_direction_input_zero_pin),
	KUNIT_CASE(test_amd_pt_gpio_get_valid),
	KUNIT_CASE(test_amd_pt_gpio_get_invalid_pin),
	KUNIT_CASE(test_amd_pt_gpio_get_edge_pin),
	KUNIT_CASE(test_amd_pt_gpio_get_zero_pin),
	KUNIT_CASE(test_amd_pt_gpio_direction_output_valid),
	KUNIT_CASE(test_amd_pt_gpio_direction_output_invalid_pin),
	KUNIT_CASE(test_amd_pt_gpio_direction_output_edge_pin),
	KUNIT_CASE(test_amd_pt_gpio_direction_output_zero_pin),
	KUNIT_CASE(test_amd_pt_gpio_set_valid),
	KUNIT_CASE(test_amd_pt_gpio_set_invalid_pin),
	KUNIT_CASE(test_amd_pt_gpio_set_edge_pin),
	KUNIT_CASE(test_amd_pt_gpio_set_zero_pin),
	{}
};

static struct kunit_suite amd_pt_gpio_test_suite = {
	.name = "amd_pt_gpio_test",
	.test_cases = amd_pt_gpio_test_cases,
};

kunit_test_suite(amd_pt_gpio_test_suite);