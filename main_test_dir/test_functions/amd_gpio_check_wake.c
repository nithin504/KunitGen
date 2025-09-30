
static bool __maybe_unused amd_gpio_check_wake(void *dev_id)
{
	return do_amd_gpio_irq_handler(-1, dev_id);
}