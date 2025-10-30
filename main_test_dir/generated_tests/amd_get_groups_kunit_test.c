#include <kunit/test.h>
#include <linux/io.h>
#include <linux/gpio/driver.h>
#include "gpio-amdpt.c"

#define MOCK_BASE_ADDR 0x1000
#define INTERNAL_GPIO0_DEBOUNCE 0x2
#define DB_TYPE_REMOVE_GLITCH 0x1
#define DB_CNTRL_OFF 28
#define DB_TMR_OUT_MASK 0xFF
#define DB_TMR_LARGE_OFF 9
#define DB_CNTRL_MASK 0x7
#define WAKE_INT_MASTER_REG 0x10

// Mocking the hardware access functions to avoid linking errors
static void __iomem *mock_base;

static inline u32 readl_mock(const volatile void __iomem *addr)
{
    uintptr_t offset = (uintptr_t)addr - (uintptr_t)mock_base;
    return *(volatile u32 *)(mock_base + offset);
}

static inline void writel_mock(u32 value, volatile void __iomem *addr)
{
    uintptr_t offset = (uintptr_t)addr - (uintptr_t)mock_base;
    *(volatile u32 *)(mock_base + offset) = value;
}

#define readl readl_mock
#define writel writel_mock

static struct pt_gpio *gpio_dev;
static char debounce_test_buffer[8192];

static int gpio_amdpt_test_init(struct kunit *test)
{
    gpio_dev = kunit_kzalloc(test, sizeof(*gpio_dev), GFP_KERNEL);
    if (!gpio_dev)
        return -ENOMEM;

    mock_base = debounce_test_buffer;
    gpio_dev->base = mock_base;

    return 0;
}

static void test_pt_gpio_request_valid_pin(struct kunit *test)
{
    int ret;
    
    // Test valid pin request
    ret = pt_gpio_request(&gpio_dev->chip, 5);
    KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_pt_gpio_request_invalid_pin(struct kunit *test)
{
    int ret;
    
    // Test invalid pin request (out of range)
    ret = pt_gpio_request(&gpio_dev->chip, 64);
    KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_pt_gpio_free_valid_pin(struct kunit *test)
{
    // First request a pin
    int ret = pt_gpio_request(&gpio_dev->chip, 3);
    KUNIT_EXPECT_EQ(test, ret, 0);
    
    // Then free it
    pt_gpio_free(&gpio_dev->chip, 3);
    // No return value to check, but shouldn't crash
}

static void test_pt_gpio_get_valid_pin(struct kunit *test)
{
    int ret;
    
    // Set up initial state
    *((u32 *)(debounce_test_buffer + 0x40)) = 0; // GPIO_DATA_REG for pin 0
    
    ret = pt_gpio_get(&gpio_dev->chip, 0);
    KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_pt_gpio_get_inverted_pin(struct kunit *test)
{
    int ret;
    
    // Set up inverted state
    gpio_dev->chip.bgpio_pdata.flags = BGPIOF_INVERTED;
    *((u32 *)(debounce_test_buffer + 0x40)) = 0; // GPIO_DATA_REG for pin 0
    
    ret = pt_gpio_get(&gpio_dev->chip, 0);
    KUNIT_EXPECT_EQ(test, ret, 1);
    
    gpio_dev->chip.bgpio_pdata.flags = 0; // Reset flag
}

static void test_pt_gpio_set_valid_pin_high(struct kunit *test)
{
    // Set pin direction as output first
    pt_gpio_direction_output(&gpio_dev->chip, 2, 1);
    
    pt_gpio_set(&gpio_dev->chip, 2, 1);
    // Verify the value was set correctly
    u32 reg_val = *((u32 *)(debounce_test_buffer + 0x40 + (2 * 4))); // GPIO_DATA_REG for pin 2
    KUNIT_EXPECT_EQ(test, (reg_val & BIT(2)) ? 1 : 0, 1);
}

static void test_pt_gpio_set_valid_pin_low(struct kunit *test)
{
    // Set pin direction as output first
    pt_gpio_direction_output(&gpio_dev->chip, 1, 0);
    
    pt_gpio_set(&gpio_dev->chip, 1, 0);
    // Verify the value was set correctly
    u32 reg_val = *((u32 *)(debounce_test_buffer + 0x40 + (1 * 4))); // GPIO_DATA_REG for pin 1
    KUNIT_EXPECT_EQ(test, (reg_val & BIT(1)) ? 1 : 0, 0);
}

static void test_pt_gpio_set_inverted_pin(struct kunit *test)
{
    // Set up inverted state
    gpio_dev->chip.bgpio_pdata.flags = BGPIOF_INVERTED;
    
    // Set pin direction as output first
    pt_gpio_direction_output(&gpio_dev->chip, 4, 1);
    
    pt_gpio_set(&gpio_dev->chip, 4, 1);
    // With inversion, setting 1 should write 0
    u32 reg_val = *((u32 *)(debounce_test_buffer + 0x40 + (4 * 4))); // GPIO_DATA_REG for pin 4
    KUNIT_EXPECT_EQ(test, (reg_val & BIT(4)) ? 1 : 0, 0);
    
    gpio_dev->chip.bgpio_pdata.flags = 0; // Reset flag
}

static void test_pt_gpio_direction_input(struct kunit *test)
{
    int ret;
    
    ret = pt_gpio_direction_input(&gpio_dev->chip, 3);
    KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_pt_gpio_direction_output_high(struct kunit *test)
{
    int ret;
    
    ret = pt_gpio_direction_output(&gpio_dev->chip, 2, 1);
    KUNIT_EXPECT_EQ(test, ret, 0);
    
    // Check that the data register was set correctly
    u32 reg_val = *((u32 *)(debounce_test_buffer + 0x40 + (2 * 4)));
    KUNIT_EXPECT_NE(test, reg_val & BIT(2), 0U);
}

static void test_pt_gpio_direction_output_low(struct kunit *test)
{
    int ret;
    
    ret = pt_gpio_direction_output(&gpio_dev->chip, 1, 0);
    KUNIT_EXPECT_EQ(test, ret, 0);
    
    // Check that the data register was set correctly
    u32 reg_val = *((u32 *)(debounce_test_buffer + 0x40 + (1 * 4)));
    KUNIT_EXPECT_EQ(test, reg_val & BIT(1), 0U);
}

static struct kunit_case gpio_amdpt_test_cases[] = {
    KUNIT_CASE_INITIALIZER(test_pt_gpio_request_valid_pin, gpio_amdpt_test_init),
    KUNIT_CASE_INITIALIZER(test_pt_gpio_request_invalid_pin, gpio_amdpt_test_init),
    KUNIT_CASE_INITIALIZER(test_pt_gpio_free_valid_pin, gpio_amdpt_test_init),
    KUNIT_CASE_INITIALIZER(test_pt_gpio_get_valid_pin, gpio_amdpt_test_init),
    KUNIT_CASE_INITIALIZER(test_pt_gpio_get_inverted_pin, gpio_amdpt_test_init),
    KUNIT_CASE_INITIALIZER(test_pt_gpio_set_valid_pin_high, gpio_amdpt_test_init),
    KUNIT_CASE_INITIALIZER(test_pt_gpio_set_valid_pin_low, gpio_amdpt_test_init),
    KUNIT_CASE_INITIALIZER(test_pt_gpio_set_inverted_pin, gpio_amdpt_test_init),
    KUNIT_CASE_INITIALIZER(test_pt_gpio_direction_input, gpio_amdpt_test_init),
    KUNIT_CASE_INITIALIZER(test_pt_gpio_direction_output_high, gpio_amdpt_test_init),
    KUNIT_CASE_INITIALIZER(test_pt_gpio_direction_output_low, gpio_amdpt_test_init),
    {}
};

static struct kunit_suite gpio_amdpt_test_suite = {
    .name = "gpio_amdpt_test",
    .test_cases = gpio_amdpt_test_cases,
};

kunit_test_suite(gpio_amdpt_test_suite);