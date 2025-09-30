// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/errno.h>

#define PIN_IRQ_PENDING 0x80000000

struct amd_gpio {
	struct pinctrl_dev *pctrl;
	void __iomem *base;
	u32 *saved_regs;
	raw_spinlock_t lock;
};

static int amd_gpio_should_save(struct amd_gpio *gpio_dev, int pin)
{
	return 1;
}

static int amd_gpio_resume(struct device *dev)
{
	struct amd_gpio *gpio_dev = dev_get_drvdata(dev);
	const struct pinctrl_desc *desc = gpio_dev->pctrl->desc;
	unsigned long flags;
	int i;

	for (i = 0; i < desc->npins; i++) {
		int pin = desc->pins[i].number;

		if (!amd_gpio_should_save(gpio_dev, pin))
			continue;

		raw_spin_lock_irqsave(&gpio_dev->lock, flags);
		gpio_dev->saved_regs[i] |= readl(gpio_dev->base + pin * 4) & PIN_IRQ_PENDING;
		writel(gpio_dev->saved_regs[i], gpio_dev->base + pin * 4);
		raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
	}

	return 0;
}

static char test_mmio_buffer[8192];
static struct amd_gpio mock_gpio_dev;
static struct pinctrl_desc mock_pinctrl_desc;
static struct pinctrl_pin_desc mock_pins[3];
static u32 saved_regs[3];
static struct device mock_dev;

static void test_amd_gpio_resume_normal_case(struct kunit *test)
{
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pctrl), GFP_KERNEL);
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.saved_regs = saved_regs;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 3;
	mock_pinctrl_desc.pins = mock_pins;
	
	mock_pins[0].number = 0;
	mock_pins[1].number = 1;
	mock_pins[2].number = 2;
	
	saved_regs[0] = 0x1000;
	saved_regs[1] = 0x2000;
	saved_regs[2] = 0x3000;
	
	writel(0x4000 | PIN_IRQ_PENDING, mock_gpio_dev.base + 0 * 4);
	writel(0x5000, mock_gpio_dev.base + 1 * 4);
	writel(0x6000 | PIN_IRQ_PENDING, mock_gpio_dev.base + 2 * 4);

	int ret = amd_gpio_resume(&mock_dev);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, saved_regs[0], 0x1000 | PIN_IRQ_PENDING);
	KUNIT_EXPECT_EQ(test, saved_regs[1], 0x2000);
	KUNIT_EXPECT_EQ(test, saved_regs[2], 0x3000 | PIN_IRQ_PENDING);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + 0 * 4), 0x1000 | PIN_IRQ_PENDING);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + 1 * 4), 0x2000);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + 2 * 4), 0x3000 | PIN_IRQ_PENDING);
}

static void test_amd_gpio_resume_no_pins(struct kunit *test)
{
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pctrl), GFP_KERNEL);
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.saved_regs = saved_regs;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 0;
	mock_pinctrl_desc.pins = NULL;

	int ret = amd_gpio_resume(&mock_dev);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_resume_should_not_save(struct kunit *test)
{
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = kunit_kzalloc(test, sizeof(*mock_gpio_dev.pctrl), GFP_KERNEL);
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.saved_regs = saved_regs;
	raw_spin_lock_init(&mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 1;
	mock_pinctrl_desc.pins = mock_pins;
	
	mock_pins[0].number = 0;
	saved_regs[0] = 0x1000;
	writel(0x4000 | PIN_IRQ_PENDING, mock_gpio_dev.base + 0 * 4);

	extern int amd_gpio_should_save(struct amd_gpio *gpio_dev, int pin);
	int (*original_should_save)(struct amd_gpio *, int) = amd_gpio_should_save;
	extern int amd_gpio_should_save_return_0;
	int amd_gpio_should_save_return_0 = 1;
	
	int ret = amd_gpio_resume(&mock_dev);
	
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, saved_regs[0], 0x1000);
	KUNIT_EXPECT_EQ(test, readl(mock_gpio_dev.base + 0 * 4), 0x4000 | PIN_IRQ_PENDING);
}

static void test_amd_gpio_resume_null_device(struct kunit *test)
{
	int ret = amd_gpio_resume(NULL);
	KUNIT_EXPECT_LT(test, ret, 0);
}

static void test_amd_gpio_resume_null_pinctrl(struct kunit *test)
{
	struct amd_gpio local_gpio_dev = {0};
	local_gpio_dev.pctrl = NULL;
	
	extern void *dev_get_drvdata_return;
	struct device local_dev;
	void *dev_get_drvdata_return = &local_gpio_dev;
	
	int ret = amd_gpio_resume(&local_dev);
	KUNIT_EXPECT_LT(test, ret, 0);
}

static void test_amd_gpio_resume_null_desc(struct kunit *test)
{
	struct amd_gpio local_gpio_dev = {0};
	local_gpio_dev.pctrl = kunit_kzalloc(test, sizeof(*local_gpio_dev.pctrl), GFP_KERNEL);
	local_gpio_dev.pctrl->desc = NULL;
	
	extern void *dev_get_drvdata_return;
	struct device local_dev;
	void *dev_get_drvdata_return = &local_gpio_dev;
	
	int ret = amd_gpio_resume(&local_dev);
	KUNIT_EXPECT_LT(test, ret, 0);
}

static struct kunit_case amd_gpio_resume_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_resume_normal_case),
	KUNIT_CASE(test_amd_gpio_resume_no_pins),
	KUNIT_CASE(test_amd_gpio_resume_should_not_save),
	KUNIT_CASE(test_amd_gpio_resume_null_device),
	KUNIT_CASE(test_amd_gpio_resume_null_pinctrl),
	KUNIT_CASE(test_amd_gpio_resume_null_desc),
	{}
};

static struct kunit_suite amd_gpio_resume_test_suite = {
	.name = "amd_gpio_resume_test",
	.test_cases = amd_gpio_resume_test_cases,
};

kunit_test_suite(amd_gpio_resume_test_suite);