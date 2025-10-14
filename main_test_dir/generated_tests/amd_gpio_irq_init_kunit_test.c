// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/errno.h>

static struct pin_desc *pin_desc_get_mock(struct pinctrl_dev *pctldev, unsigned int pin);
#define pin_desc_get pin_desc_get_mock

#include "pinctrl-amd.c"

#define WAKE_CNTRL_OFF_S0I3 0
#define WAKE_CNTRL_OFF_S3 1
#define WAKE_CNTRL_OFF_S4 2

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct pinctrl_dev mock_pctldev;
static struct pinctrl_desc mock_pinctrl_desc;
static struct pin_desc mock_pin_descs[10];
static int pin_desc_get_call_count = 0;
static struct pin_desc *pin_desc_get_return_vals[10];

struct pin_desc *pin_desc_get_mock(struct pinctrl_dev *pctldev, unsigned int pin)
{
	pin_desc_get_call_count++;
	if (pin < 10)
		return pin_desc_get_return_vals[pin];
	return NULL;
}

static void test_amd_gpio_irq_init_normal_case(struct kunit *test)
{
	int i;
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pctldev;
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 3;
	mock_pinctrl_desc.pins = kunit_kzalloc(test, sizeof(struct pinctrl_pin_desc) * 3, GFP_KERNEL);
	
	for (i = 0; i < 3; i++) {
		mock_pinctrl_desc.pins[i].number = i;
		pin_desc_get_return_vals[i] = &mock_pin_descs[i];
		writel(0xFFFFFFFF, mock_gpio_dev.base + i * 4);
	}
	
	pin_desc_get_call_count = 0;
	amd_gpio_irq_init(&mock_gpio_dev);
	
	KUNIT_EXPECT_EQ(test, pin_desc_get_call_count, 3);
	
	for (i = 0; i < 3; i++) {
		u32 val = readl(mock_gpio_dev.base + i * 4);
		u32 mask = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3) | BIT(WAKE_CNTRL_OFF_S4);
		KUNIT_EXPECT_EQ(test, val & mask, 0);
	}
	
	kunit_kfree(test, mock_pinctrl_desc.pins);
}

static void test_amd_gpio_irq_init_null_pin_desc(struct kunit *test)
{
	int i;
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pctldev;
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 3;
	mock_pinctrl_desc.pins = kunit_kzalloc(test, sizeof(struct pinctrl_pin_desc) * 3, GFP_KERNEL);
	
	for (i = 0; i < 3; i++) {
		mock_pinctrl_desc.pins[i].number = i;
		pin_desc_get_return_vals[i] = NULL;
		writel(0xFFFFFFFF, mock_gpio_dev.base + i * 4);
	}
	
	pin_desc_get_call_count = 0;
	amd_gpio_irq_init(&mock_gpio_dev);
	
	KUNIT_EXPECT_EQ(test, pin_desc_get_call_count, 3);
	
	for (i = 0; i < 3; i++) {
		u32 val = readl(mock_gpio_dev.base + i * 4);
		KUNIT_EXPECT_EQ(test, val, 0xFFFFFFFF);
	}
	
	kunit_kfree(test, mock_pinctrl_desc.pins);
}

static void test_amd_gpio_irq_init_zero_pins(struct kunit *test)
{
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pctldev;
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 0;
	mock_pinctrl_desc.pins = NULL;
	
	pin_desc_get_call_count = 0;
	amd_gpio_irq_init(&mock_gpio_dev);
	
	KUNIT_EXPECT_EQ(test, pin_desc_get_call_count, 0);
}

static void test_amd_gpio_irq_init_mixed_pin_descs(struct kunit *test)
{
	int i;
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.pctrl = &mock_pctldev;
	mock_gpio_dev.pctrl->desc = &mock_pinctrl_desc;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	
	mock_pinctrl_desc.npins = 5;
	mock_pinctrl_desc.pins = kunit_kzalloc(test, sizeof(struct pinctrl_pin_desc) * 5, GFP_KERNEL);
	
	for (i = 0; i < 5; i++) {
		mock_pinctrl_desc.pins[i].number = i;
		if (i % 2 == 0) {
			pin_desc_get_return_vals[i] = &mock_pin_descs[i];
		} else {
			pin_desc_get_return_vals[i] = NULL;
		}
		writel(0xFFFFFFFF, mock_gpio_dev.base + i * 4);
	}
	
	pin_desc_get_call_count = 0;
	amd_gpio_irq_init(&mock_gpio_dev);
	
	KUNIT_EXPECT_EQ(test, pin_desc_get_call_count, 5);
	
	for (i = 0; i < 5; i++) {
		u32 val = readl(mock_gpio_dev.base + i * 4);
		u32 mask = BIT(WAKE_CNTRL_OFF_S0I3) | BIT(WAKE_CNTRL_OFF_S3) | BIT(WAKE_CNTRL_OFF_S4);
		
		if (i % 2 == 0) {
			KUNIT_EXPECT_EQ(test, val & mask, 0);
		} else {
			KUNIT_EXPECT_EQ(test, val, 0xFFFFFFFF);
		}
	}
	
	kunit_kfree(test, mock_pinctrl_desc.pins);
}

static struct kunit_case amd_gpio_irq_init_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_init_normal_case),
	KUNIT_CASE(test_amd_gpio_irq_init_null_pin_desc),
	KUNIT_CASE(test_amd_gpio_irq_init_zero_pins),
	KUNIT_CASE(test_amd_gpio_irq_init_mixed_pin_descs),
	{}
};

static struct kunit_suite amd_gpio_irq_init_test_suite = {
	.name = "amd_gpio_irq_init_test",
	.test_cases = amd_gpio_irq_init_test_cases,
};

kunit_test_suite(amd_gpio_irq_init_test_suite);