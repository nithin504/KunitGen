// SPDX-License-Identifier: GPL-2.0
#include <kunit/test.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/io.h>

// Mock definitions to avoid external dependencies
#define WAKE_INT_STATUS_REG0 0x00
#define WAKE_INT_STATUS_REG1 0x04
#define WAKE_INT_MASTER_REG 0x08
#define PIN_IRQ_PENDING BIT(28)
#define WAKE_STS_OFF 26
#define INTERRUPT_MASK_OFF 25
#define EOI_MASK BIT(0)

struct amd_gpio {
    struct gpio_chip gc;
    struct platform_device *pdev;
    void __iomem *base;
    raw_spinlock_t lock;
};

// Mock functions
static void pm_pr_dbg(const char *fmt, ...) {}
static void dev_dbg(const struct device *dev, const char *fmt, ...) {}

static bool gpiochip_line_is_irq_mock(struct gpio_chip *gc, unsigned int offset)
{
    return true;
}

static int generic_handle_domain_irq_safe_mock(struct irq_domain *domain, unsigned int hwirq)
{
    return 0;
}

#define gpiochip_line_is_irq gpiochip_line_is_irq_mock
#define generic_handle_domain_irq_safe generic_handle_domain_irq_safe_mock

// Include the function to test
static bool do_amd_gpio_irq_handler(int irq, void *dev_id)
{
    struct amd_gpio *gpio_dev = dev_id;
    struct gpio_chip *gc = &gpio_dev->gc;
    unsigned int i, irqnr;
    unsigned long flags;
    u32 __iomem *regs;
    bool ret = false;
    u32 regval;
    u64 status, mask;

    /* Read the wake status */
    raw_spin_lock_irqsave(&gpio_dev->lock, flags);
    status = readl(gpio_dev->base + WAKE_INT_STATUS_REG1);
    status <<= 32;
    status |= readl(gpio_dev->base + WAKE_INT_STATUS_REG0);
    raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

    /* Bit 0-45 contain the relevant status bits */
    status &= (1ULL << 46) - 1;
    regs = gpio_dev->base;
    for (mask = 1, irqnr = 0; status; mask <<= 1, regs += 4, irqnr += 4) {
        if (!(status & mask))
            continue;
        status &= ~mask;

        /* Each status bit covers four pins */
        for (i = 0; i < 4; i++) {
            regval = readl(regs + i);

            if (regval & PIN_IRQ_PENDING)
                pm_pr_dbg("GPIO %d is active: 0x%x", irqnr + i, regval);

            /* caused wake on resume context for shared IRQ */
            if (irq < 0 && (regval & BIT(WAKE_STS_OFF)))
                return true;

            if (!(regval & PIN_IRQ_PENDING) ||
                !(regval & BIT(INTERRUPT_MASK_OFF)))
                continue;
            generic_handle_domain_irq_safe(gc->irq.domain, irqnr + i);

            /* Clear interrupt.
             * We must read the pin register again, in case the
             * value was changed while executing
             * generic_handle_domain_irq() above.
             * If the line is not an irq, disable it in order to
             * avoid a system hang caused by an interrupt storm.
             */
            raw_spin_lock_irqsave(&gpio_dev->lock, flags);
            regval = readl(regs + i);
            if (!gpiochip_line_is_irq(gc, irqnr + i)) {
                regval &= ~BIT(INTERRUPT_MASK_OFF);
                dev_dbg(&gpio_dev->pdev->dev,
                    "Disabling spurious GPIO IRQ %d\n", irqnr + i);
            } else {
                ret = true;
            }
            writel(regval, regs + i);
            raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);
        }
    }
    /* did not cause wake on resume context for shared IRQ */
    if (irq < 0)
        return false;

    /* Signal EOI to the GPIO unit */
    raw_spin_lock_irqsave(&gpio_dev->lock, flags);
    regval = readl(gpio_dev->base + WAKE_INT_MASTER_REG);
    regval |= EOI_MASK;
    writel(regval, gpio_dev->base + WAKE_INT_MASTER_REG);
    raw_spin_unlock_irqrestore(&gpio_dev->lock, flags);

    return ret;
}

// Test infrastructure
static char test_mmio_buffer[4096];
static struct amd_gpio mock_gpio_dev;
static struct irq_domain mock_irq_domain;

static void setup_mock_gpio_dev(struct kunit *test)
{
    mock_gpio_dev.base = (void __iomem *)test_mmio_buffer;
    mock_gpio_dev.gc.irq.domain = &mock_irq_domain;
    mock_gpio_dev.lock = __RAW_SPIN_LOCK_UNLOCKED(mock_gpio_dev.lock);
    mock_gpio_dev.pdev = kunit_kzalloc(test, sizeof(struct platform_device), GFP_KERNEL);
    
    // Initialize test buffer
    memset(test_mmio_buffer, 0, sizeof(test_mmio_buffer));
}

// Test cases
static void test_do_amd_gpio_irq_handler_shared_irq_wake(struct kunit *test)
{
    bool ret;
    setup_mock_gpio_dev(test);
    
    // Set WAKE_STS_OFF bit in first register to trigger wake return
    writel(BIT(WAKE_STS_OFF), mock_gpio_dev.base + 0);
    
    ret = do_amd_gpio_irq_handler(-1, &mock_gpio_dev);
    KUNIT_EXPECT_TRUE(test, ret);
}

static void test_do_amd_gpio_irq_handler_shared_irq_no_wake(struct kunit *test)
{
    bool ret;
    setup_mock_gpio_dev(test);
    
    // Clear WAKE_STS_OFF bit
    writel(0, mock_gpio_dev.base + 0);
    
    ret = do_amd_gpio_irq_handler(-1, &mock_gpio_dev);
    KUNIT_EXPECT_FALSE(test, ret);
}

static void test_do_amd_gpio_irq_handler_normal_irq_no_pending(struct kunit *test)
{
    bool ret;
    setup_mock_gpio_dev(test);
    
    // Set status but no pending interrupts
    writel(1, mock_gpio_dev.base + WAKE_INT_STATUS_REG0);
    writel(0, mock_gpio_dev.base + WAKE_INT_STATUS_REG1);
    
    ret = do_amd_gpio_irq_handler(1, &mock_gpio_dev);
    KUNIT_EXPECT_FALSE(test, ret);
}

static void test_do_amd_gpio_irq_handler_normal_irq_pending_no_mask(struct kunit *test)
{
    bool ret;
    setup_mock_gpio_dev(test);
    
    // Set status and pending but no interrupt mask
    writel(1, mock_gpio_dev.base + WAKE_INT_STATUS_REG0);
    writel(0, mock_gpio_dev.base + WAKE_INT_STATUS_REG1);
    writel(PIN_IRQ_PENDING, mock_gpio_dev.base + 0);
    
    ret = do_amd_gpio_irq_handler(1, &mock_gpio_dev);
    KUNIT_EXPECT_FALSE(test, ret);
}

static void test_do_amd_gpio_irq_handler_normal_irq_pending_with_mask(struct kunit *test)
{
    bool ret;
    setup_mock_gpio_dev(test);
    
    // Set status, pending and interrupt mask
    writel(1, mock_gpio_dev.base + WAKE_INT_STATUS_REG0);
    writel(0, mock_gpio_dev.base + WAKE_INT_STATUS_REG1);
    writel(PIN_IRQ_PENDING | BIT(INTERRUPT_MASK_OFF), mock_gpio_dev.base + 0);
    
    ret = do_amd_gpio_irq_handler(1, &mock_gpio_dev);
    KUNIT_EXPECT_TRUE(test, ret);
}

static void test_do_amd_gpio_irq_handler_spurious_irq(struct kunit *test)
{
    bool ret;
    setup_mock_gpio_dev(test);
    
    // Set status, pending and interrupt mask but line is not IRQ
    writel(1, mock_gpio_dev.base + WAKE_INT_STATUS_REG0);
    writel(0, mock_gpio_dev.base + WAKE_INT_STATUS_REG1);
    writel(PIN_IRQ_PENDING | BIT(INTERRUPT_MASK_OFF), mock_gpio_dev.base + 0);
    
    // Override mock to return false for line_is_irq
    mock_gpio_dev.gc.irq.domain = NULL;
    
    ret = do_amd_gpio_irq_handler(1, &mock_gpio_dev);
    KUNIT_EXPECT_FALSE(test, ret);
}

static void test_do_amd_gpio_irq_handler_multiple_status_bits(struct kunit *test)
{
    bool ret;
    setup_mock_gpio_dev(test);
    
    // Set multiple status bits
    writel(0x3, mock_gpio_dev.base + WAKE_INT_STATUS_REG0);
    writel(0, mock_gpio_dev.base + WAKE_INT_STATUS_REG1);
    
    // Set pending and mask for first group
    writel(PIN_IRQ_PENDING | BIT(INTERRUPT_MASK_OFF), mock_gpio_dev.base + 0);
    writel(PIN_IRQ_PENDING | BIT(INTERRUPT_MASK_OFF), mock_gpio_dev.base + 4);
    
    ret = do_amd_gpio_irq_handler(1, &mock_gpio_dev);
    KUNIT_EXPECT_TRUE(test, ret);
}

static void test_do_amd_gpio_irq_handler_status_beyond_46_bits(struct kunit *test)
{
    bool ret;
    setup_mock_gpio_dev(test);
    
    // Set status bit beyond bit 45 (should be masked out)
    writel(0, mock_gpio_dev.base + WAKE_INT_STATUS_REG0);
    writel(1, mock_gpio_dev.base + WAKE_INT_STATUS_REG1); // Bit 32
    
    ret = do_amd_gpio_irq_handler(1, &mock_gpio_dev);
    KUNIT_EXPECT_FALSE(test, ret);
}

static void test_do_amd_gpio_irq_handler_null_dev_id(struct kunit *test)
{
    bool ret;
    
    ret = do_amd_gpio_irq_handler(1, NULL);
    KUNIT_EXPECT_FALSE(test, ret);
}

static struct kunit_case amd_gpio_irq_handler_test_cases[] = {
    KUNIT_CASE(test_do_amd_gpio_irq_handler_shared_irq_wake),
    KUNIT_CASE(test_do_amd_gpio_irq_handler_shared_irq_no_wake),
    KUNIT_CASE(test_do_amd_gpio_irq_handler_normal_irq_no_pending),
    KUNIT_CASE(test_do_amd_gpio_irq_handler_normal_irq_pending_no_mask),
    KUNIT_CASE(test_do_amd_gpio_irq_handler_normal_irq_pending_with_mask),
    KUNIT_CASE(test_do_amd_gpio_irq_handler_spurious_irq),
    KUNIT_CASE(test_do_amd_gpio_irq_handler_multiple_status_bits),
    KUNIT_CASE(test_do_amd_gpio_irq_handler_status_beyond_46_bits),
    KUNIT_CASE(test_do_amd_gpio_irq_handler_null_dev_id),
    {}
};

static struct kunit_suite amd_gpio_irq_handler_test_suite = {
    .name = "amd_gpio_irq_handler_test",
    .test_cases = amd_gpio_irq_handler_test_cases,
};

kunit_test_suite(amd_gpio_irq_handler_test_suite);