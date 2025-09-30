#include <kunit/test.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/gpio/driver.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#define LEVEL_TRIG_OFF 1
#define ACTIVE_LEVEL_OFF 3
#define ACTIVE_LEVEL_MASK 0x3
#define ACTIVE_HIGH 0
#define ACTIVE_LOW 1
#define BOTH_EDGES 2
#define LEVEL_TRIGGER 1
#define CLR_INTR_STAT 1
#define INTERRUPT_STS_OFF 5
#define INTERRUPT_ENABLE_OFF 6
#define INTERRUPT_MASK_OFF 7

struct amd_gpio {
	void __iomem *base;
	raw_spinlock_t lock;
	struct platform_device *pdev;
};

static int amd_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
	int ret = 0;
	u32 pin_reg, pin_reg_irq_en, mask;
	unsigned long flags;
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct amd_gpio *gpio_dev = gpiochip_get_data(gc);

	raw_spin_lock_irqsave(&gpio_dev->lock, flags);
	pin_reg = readl(gpio_dev->base + (d->hwirq)*4);

	switch (type & IRQ_TYPE_SENSE_MASK) {
	case IRQ_TYPE_EDGE_RISING:
		pin_reg &= ~BIT(LEVEL_TRIG_OFF);
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_HIGH << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_edge_irq);
		break;

	case IRQ_TYPE_EDGE_FALLING:
		pin_reg &= ~BIT(LEVEL_TRIG_OFF);
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_LOW << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_edge_irq);
		break;

	case IRQ_TYPE_EDGE_BOTH:
		pin_reg &= ~BIT(LEVEL_TRIG_OFF);
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= BOTH_EDGES << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_edge_irq);
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		pin_reg |= LEVEL_TRIGGER << LEVEL_TRIG_OFF;
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_HIGH << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_level_irq);
		break;

	case IRQ_TYPE_LEVEL_LOW:
		pin_reg |= LEVEL_TRIGGER << LEVEL_TRIG_OFF;
		pin_reg &= ~(ACTIVE_LEVEL_MASK << ACTIVE_LEVEL_OFF);
		pin_reg |= ACTIVE_LOW << ACTIVE_LEVEL_OFF;
		irq_set_handler_locked(d, handle_level_irq);
		break;

	case IRQ_TYPE_NONE:
		break;

	default:
		dev_err(&gpio_dev->pdev->dev, "Invalid type value\n");
		ret = -EINVAL;
	}

	pin_reg |= CLR_INTR_STAT << INTERRUPT_STS_OFF;
	mask = BIT(INTERRUPT_ENABLE_OFF);
	pin_reg_irq_en = pin_reg;
	pin_reg_irq_en |= mask;
	pin_reg_irq_en &= ~BIT(INTERRUPT_MASK_OFF);
	writel(pin_reg_irq_en, gpio_dev->base + (d->hwirq)*4);
	while ((readl(gpio_dev->base + (d->hwirq)*4) & mask) != mask)
		continue;
	writel(pin_reg, gpio_dev->base + (d->hwirq)*4);
	raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

	return ret;
}

static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct platform_device mock_pdev;
static struct gpio_chip mock_gc;
static struct irq_data mock_irq_data;

static void test_amd_gpio_irq_set_type_edge_rising(struct kunit *test)
{
	struct irq_data d = mock_irq_data;
	d.hwirq = 0;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = &mock_pdev;
	writel(0xFFFFFFFF, mock_gpio_dev.base);
	
	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_EDGE_RISING);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_type_edge_falling(struct kunit *test)
{
	struct irq_data d = mock_irq_data;
	d.hwirq = 1;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = &mock_pdev;
	writel(0xFFFFFFFF, mock_gpio_dev.base + 4);
	
	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_EDGE_FALLING);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_type_edge_both(struct kunit *test)
{
	struct irq_data d = mock_irq_data;
	d.hwirq = 2;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = &mock_pdev;
	writel(0xFFFFFFFF, mock_gpio_dev.base + 8);
	
	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_EDGE_BOTH);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_type_level_high(struct kunit *test)
{
	struct irq_data d = mock_irq_data;
	d.hwirq = 3;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = &mock_pdev;
	writel(0xFFFFFFFF, mock_gpio_dev.base + 12);
	
	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_LEVEL_HIGH);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_type_level_low(struct kunit *test)
{
	struct irq_data d = mock_irq_data;
	d.hwirq = 4;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = &mock_pdev;
	writel(0xFFFFFFFF, mock_gpio_dev.base + 16);
	
	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_LEVEL_LOW);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_type_none(struct kunit *test)
{
	struct irq_data d = mock_irq_data;
	d.hwirq = 5;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = &mock_pdev;
	writel(0xFFFFFFFF, mock_gpio_dev.base + 20);
	
	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_NONE);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_irq_set_type_invalid(struct kunit *test)
{
	struct irq_data d = mock_irq_data;
	d.hwirq = 6;
	int ret;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = &mock_pdev;
	writel(0xFFFFFFFF, mock_gpio_dev.base + 24);
	
	ret = amd_gpio_irq_set_type(&d, 0xFF);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
}

static void test_amd_gpio_irq_set_type_polling_timeout(struct kunit *test)
{
	struct irq_data d = mock_irq_data;
	d.hwirq = 7;
	int ret;
	static int read_count;
	
	mock_gpio_dev.base = test_mmio_buffer;
	mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
	mock_gpio_dev.pdev = &mock_pdev;
	writel(0x0, mock_gpio_dev.base + 28);
	
	read_count = 0;
	ret = amd_gpio_irq_set_type(&d, IRQ_TYPE_EDGE_RISING);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static struct kunit_case amd_gpio_irq_set_type_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_irq_set_type_edge_rising),
	KUNIT_CASE(test_amd_gpio_irq_set_type_edge_falling),
	KUNIT_CASE(test_amd_gpio_irq_set_type_edge_both),
	KUNIT_CASE(test_amd_gpio_irq_set_type_level_high),
	KUNIT_CASE(test_amd_gpio_irq_set_type_level_low),
	KUNIT_CASE(test_amd_gpio_irq_set_type_none),
	KUNIT_CASE(test_amd_gpio_irq_set_type_invalid),
	KUNIT_CASE(test_amd_gpio_irq_set_type_polling_timeout),
	{}
};

static struct kunit_suite amd_gpio_irq_set_type_test_suite = {
	.name = "amd_gpio_irq_set_type_test",
	.test_cases = amd_gpio_irq_set_type_test_cases,
};

kunit_test_suite(amd_gpio_irq_set_type_test_suite);