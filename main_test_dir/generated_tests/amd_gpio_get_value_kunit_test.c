```c
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/io.h>
#include <linux/spinlock.h>

#define PIN_STS_OFF 0

static inline u32 readl(const volatile void __iomem *addr)
{
	return *(const volatile u32 __force *)addr;
}

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
};

static struct amd_gpio mock_gpio_dev;
static char mock_mmio_region[4096];

static int amd_gpio_get_value(struct gpio_chip *gc, unsigned offset)
{
	u32 pin_reg;
	unsigned long flags;
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + offset * 4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return !!(pin_reg & BIT(PIN_STS_OFF));
}

static void *mock_gpiochip_get_data(struct gpio_chip *gc)
{
	return &mock_gpio_dev;
}

#define gpiochip_get_data mock_gpiochip_get_data

static void test_amd_gpio_get_value_pin_low(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mock_mmio_region + 4 * 10);
	*reg_addr = 0x0;

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_gpio_get_value(&gc, 10);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_get_value_pin_high(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)(mock_mmio_region + 4 * 5);
	*reg_addr = BIT(PIN_STS_OFF);

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_gpio_get_value(&gc, 5);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static void test_amd_gpio_get_value_offset_zero(struct kunit *test)
{
	struct gpio_chip gc;
	u32 *reg_addr = (u32 *)mock_mmio_region;
	*reg_addr = BIT(PIN_STS_OFF);

	mock_gpio_dev.base = mock_mmio_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_gpio_get_value(&gc, 0);
	KUNIT_EXPECT_EQ(test, ret, 1);
}

static void test_amd_gpio_get_value_large_offset(struct kunit *test)
{
	struct gpio_chip gc;
	const unsigned offset = 1000;
	char large_mock_region[8192];
	u32 *reg_addr = (u32 *)(large_mock_region + offset * 4);
	*reg_addr = 0x0;

	mock_gpio_dev.base = large_mock_region;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);

	int ret = amd_gpio_get_value(&gc, offset);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_gpio_get_value_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_get_value_pin_low),
	KUNIT_CASE(test_amd_gpio_get_value_pin_high),
	KUNIT_CASE(test_amd_gpio_get_value_offset_zero),
	KUNIT_CASE(test_amd_gpio_get_value_large_offset),
	{}
};

static struct kunit_suite amd_gpio_get_value_test_suite = {
	.name = "amd_gpio_get_value_test",
	.test_cases = amd_gpio_get_value_test_cases,
};

kunit_test_suite(amd_gpio_get_value_test_suite);
```