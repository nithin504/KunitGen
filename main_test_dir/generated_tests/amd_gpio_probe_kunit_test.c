#include <kunit/test.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/acpi.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>

static struct amd_gpio {
	struct gpio_chip gc;
	struct pinctrl_dev *pctrl;
	raw_spinlock_t lock;
	void __iomem *base;
	int irq;
	struct platform_device *pdev;
#ifdef CONFIG_SUSPEND
	u32 *saved_regs;
#endif
	int hwbank_num;
	const struct group_desc *groups;
	unsigned ngroups;
} mock_gpio_dev;

static struct pinctrl_desc amd_pinctrl_desc;
static const struct group_desc kerncz_groups[] = {};

static void amd_gpio_irq_init(struct amd_gpio *gpio_dev) {}
static int amd_gpio_get_direction(struct gpio_chip *gc, unsigned offset) { return 0; }
static int amd_gpio_direction_input(struct gpio_chip *gc, unsigned offset) { return 0; }
static int amd_gpio_direction_output(struct gpio_chip *gc, unsigned offset, int value) { return 0; }
static int amd_gpio_get_value(struct gpio_chip *gc, unsigned offset) { return 0; }
static void amd_gpio_set_value(struct gpio_chip *gc, unsigned offset, int value) {}
static int amd_gpio_set_config(struct gpio_chip *gc, unsigned offset, unsigned long config) { return 0; }
static void amd_gpio_dbg_show(struct seq_file *s, struct gpio_chip *gc) {}
static void amd_get_iomux_res(struct amd_gpio *gpio_dev) {}
static irqreturn_t amd_gpio_irq_handler(int irq, void *dev_id) { return IRQ_NONE; }
static bool amd_gpio_check_wake(int irq, void *dev_id) { return false; }
static void amd_gpio_register_s2idle_ops(void) {}
static struct irq_chip amd_gpio_irqchip;

static char test_mmio_buffer[8192];
static struct resource test_res = {
	.start = 0x1000,
	.end = 0x1FFF,
	.name = "test_gpio",
	.flags = IORESOURCE_MEM,
};

static int platform_get_irq_mock(struct platform_device *pdev, unsigned int num)
{
	return 123;
}

static void *devm_kzalloc_mock(struct device *dev, size_t size, gfp_t gfp)
{
	return kunit_kzalloc(test, size, gfp);
}

static void *devm_kcalloc_mock(struct device *dev, size_t n, size_t size, gfp_t gfp)
{
	return kunit_kcalloc(test, n, size, gfp);
}

static void *devm_platform_get_and_ioremap_resource_mock(struct platform_device *pdev, unsigned int index, struct resource **res)
{
	*res = &test_res;
	return test_mmio_buffer;
}

static struct pinctrl_dev *devm_pinctrl_register_mock(struct device *dev, const struct pinctrl_desc *pctldesc, void *driver_data)
{
	return driver_data;
}

static int gpiochip_add_data_mock(struct gpio_chip *gc, void *data)
{
	return 0;
}

static int gpiochip_add_pin_range_mock(struct gpio_chip *gc, const char *pinctl_name, unsigned int gpio_offset, unsigned int pin_offset, unsigned int npins)
{
	return 0;
}

static int devm_request_irq_mock(struct device *dev, unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev_id)
{
	return 0;
}

static void acpi_register_wakeup_handler_mock(int irq, bool (*wake)(int, void*), void *dev) {}

static void platform_set_drvdata_mock(struct platform_device *pdev, void *data) {}

static void gpiochip_remove_mock(struct gpio_chip *gc) {}

#define devm_kzalloc(...) devm_kzalloc_mock(__VA_ARGS__)
#define devm_kcalloc(...) devm_kcalloc_mock(__VA_ARGS__)
#define devm_platform_get_and_ioremap_resource(...) devm_platform_get_and_ioremap_resource_mock(__VA_ARGS__)
#define platform_get_irq(...) platform_get_irq_mock(__VA_ARGS__)
#define devm_pinctrl_register(...) devm_pinctrl_register_mock(__VA_ARGS__)
#define gpiochip_add_data(...) gpiochip_add_data_mock(__VA_ARGS__)
#define gpiochip_add_pin_range(...) gpiochip_add_pin_range_mock(__VA_ARGS__)
#define devm_request_irq(...) devm_request_irq_mock(__VA_ARGS__)
#define acpi_register_wakeup_handler(...) acpi_register_wakeup_handler_mock(__VA_ARGS__)
#define platform_set_drvdata(...) platform_set_drvdata_mock(__VA_ARGS__)
#define gpiochip_remove(...) gpiochip_remove_mock(__VA_ARGS__)

static int amd_gpio_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	struct amd_gpio *gpio_dev;
	struct gpio_irq_chip *girq;

	gpio_dev = devm_kzalloc(&pdev->dev,
				sizeof(struct amd_gpio), GFP_KERNEL);
	if (!gpio_dev)
		return -ENOMEM;

	raw_spin_lock_init(&gpio_dev->lock);

	gpio_dev->base = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
	if (IS_ERR(gpio_dev->base)) {
		return PTR_ERR(gpio_dev->base);
	}

	gpio_dev->irq = platform_get_irq(pdev, 0);
	if (gpio_dev->irq < 0)
		return gpio_dev->irq;

#ifdef CONFIG_SUSPEND
	gpio_dev->saved_regs = devm_kcalloc(&pdev->dev, amd_pinctrl_desc.npins,
					    sizeof(*gpio_dev->saved_regs),
					    GFP_KERNEL);
	if (!gpio_dev->saved_regs)
		return -ENOMEM;
#endif

	gpio_dev->pdev = pdev;
	gpio_dev->gc.get_direction	= amd_gpio_get_direction;
	gpio_dev->gc.direction_input	= amd_gpio_direction_input;
	gpio_dev->gc.direction_output	= amd_gpio_direction_output;
	gpio_dev->gc.get			= amd_gpio_get_value;
	gpio_dev->gc.set			= amd_gpio_set_value;
	gpio_dev->gc.set_config		= amd_gpio_set_config;
	gpio_dev->gc.dbg_show		= amd_gpio_dbg_show;

	gpio_dev->gc.base		= -1;
	gpio_dev->gc.label			= pdev->name;
	gpio_dev->gc.owner			= THIS_MODULE;
	gpio_dev->gc.parent			= &pdev->dev;
	gpio_dev->gc.ngpio			= resource_size(res) / 4;

	gpio_dev->hwbank_num = gpio_dev->gc.ngpio / 64;
	gpio_dev->groups = kerncz_groups;
	gpio_dev->ngroups = ARRAY_SIZE(kerncz_groups);

	amd_pinctrl_desc.name = dev_name(&pdev->dev);
	amd_get_iomux_res(gpio_dev);
	gpio_dev->pctrl = devm_pinctrl_register(&pdev->dev, &amd_pinctrl_desc,
						gpio_dev);
	if (IS_ERR(gpio_dev->pctrl)) {
		return PTR_ERR(gpio_dev->pctrl);
	}

	amd_gpio_irq_init(gpio_dev);

	girq = &gpio_dev->gc.irq;
	girq->parent_handler = NULL;
	girq->num_parents = 0;
	girq->parents = NULL;
	girq->default_type = IRQ_TYPE_NONE;
	girq->handler = handle_simple_irq;

	ret = gpiochip_add_data(&gpio_dev->gc, gpio_dev);
	if (ret)
		return ret;

	ret = gpiochip_add_pin_range(&gpio_dev->gc, dev_name(&pdev->dev),
				0, 0, gpio_dev->gc.ngpio);
	if (ret) {
		goto out2;
	}

	ret = devm_request_irq(&pdev->dev, gpio_dev->irq, amd_gpio_irq_handler,
			       IRQF_SHARED | IRQF_COND_ONESHOT, KBUILD_MODNAME, gpio_dev);
	if (ret)
		goto out2;

	platform_set_drvdata(pdev, gpio_dev);
	acpi_register_wakeup_handler(gpio_dev->irq, amd_gpio_check_wake, gpio_dev);
	amd_gpio_register_s2idle_ops();

	return ret;

out2:
	gpiochip_remove(&gpio_dev->gc);

	return ret;
}

static void test_amd_gpio_probe_success(struct kunit *test)
{
	struct platform_device pdev;
	int ret;

	ret = amd_gpio_probe(&pdev);
	KUNIT_EXPECT_EQ(test, ret, 0);
}

static void test_amd_gpio_probe_devm_kzalloc_fail(struct kunit *test)
{
	struct platform_device pdev;
	int ret;

	kunit_defer_kzalloc(test, false);
	ret = amd_gpio_probe(&pdev);
	KUNIT_EXPECT_EQ(test, ret, -ENOMEM);
	kunit_defer_kzalloc(test, true);
}

static void test_amd_gpio_probe_ioremap_fail(struct kunit *test)
{
	struct platform_device pdev;
	int ret;

	kunit_defer_devm_platform_get_and_ioremap_resource(test, false);
	ret = amd_gpio_probe(&pdev);
	KUNIT_EXPECT_TRUE(test, IS_ERR_VALUE(ret));
	kunit_defer_devm_platform_get_and_ioremap_resource(test, true);
}

static void test_amd_gpio_probe_platform_get_irq_fail(struct kunit *test)
{
	struct platform_device pdev;
	int ret;

	kunit_defer_platform_get_irq(test, -ENXIO);
	ret = amd_gpio_probe(&pdev);
	KUNIT_EXPECT_EQ(test, ret, -ENXIO);
	kunit_defer_platform_get_irq(test, 123);
}

#ifdef CONFIG_SUSPEND
static void test_amd_gpio_probe_devm_kcalloc_fail(struct kunit *test)
{
	struct platform_device pdev;
	int ret;

	kunit_defer_devm_kcalloc(test, false);
	ret = amd_gpio_probe(&pdev);
	KUNIT_EXPECT_EQ(test, ret, -ENOMEM);
	kunit_defer_devm_kcalloc(test, true);
}
#endif

static void test_amd_gpio_probe_pinctrl_register_fail(struct kunit *test)
{
	struct platform_device pdev;
	int ret;

	kunit_defer_devm_pinctrl_register(test, ERR_PTR(-ENODEV));
	ret = amd_gpio_probe(&pdev);
	KUNIT_EXPECT_EQ(test, ret, -ENODEV);
	kunit_defer_devm_pinctrl_register(test, NULL);
}

static void test_amd_gpio_probe_gpiochip_add_data_fail(struct kunit *test)
{
	struct platform_device pdev;
	int ret;

	kunit_defer_gpiochip_add_data(test, -ENODEV);
	ret = amd_gpio_probe(&pdev);
	KUNIT_EXPECT_EQ(test, ret, -ENODEV);
	kunit_defer_gpiochip_add_data(test, 0);
}

static void test_amd_gpio_probe_gpiochip_add_pin_range_fail(struct kunit *test)
{
	struct platform_device pdev;
	int ret;

	kunit_defer_gpiochip_add_pin_range(test, -ENODEV);
	ret = amd_gpio_probe(&pdev);
	KUNIT_EXPECT_EQ(test, ret, -ENODEV);
	kunit_defer_gpiochip_add_pin_range(test, 0);
}

static void test_amd_gpio_probe_devm_request_irq_fail(struct kunit *test)
{
	struct platform_device pdev;
	int ret;

	kunit_defer_devm_request_irq(test, -ENODEV);
	ret = amd_gpio_probe(&pdev);
	KUNIT_EXPECT_EQ(test, ret, -ENODEV);
	kunit_defer_devm_request_irq(test, 0);
}

static struct kunit_case amd_gpio_probe_test_cases[] = {
	KUNIT_CASE(test_amd_gpio_probe_success),
	KUNIT_CASE(test_amd_gpio_probe_devm_kzalloc_fail),
	KUNIT_CASE(test_amd_gpio_probe_ioremap_fail),
	KUNIT_CASE(test_amd_gpio_probe_platform_get_irq_fail),
#ifdef CONFIG_SUSPEND
	KUNIT_CASE(test_amd_gpio_probe_devm_kcalloc_fail),
#endif
	KUNIT_CASE(test_amd_gpio_probe_pinctrl_register_fail),
	KUNIT_CASE(test_amd_gpio_probe_gpiochip_add_data_fail),
	KUNIT_CASE(test_amd_gpio_probe_gpiochip_add_pin_range_fail),
	KUNIT_CASE(test_amd_gpio_probe_devm_request_irq_fail),
	{}
};

static struct kunit_suite amd_gpio_probe_test_suite = {
	.name = "amd_gpio_probe_test",
	.test_cases = amd_gpio_probe_test_cases,
};

kunit_test_suite(amd_gpio_probe_test_suite);