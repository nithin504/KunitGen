
void amd_pmf_deinit_auto_mode(struct amd_pmf_dev *dev)
{
	cancel_delayed_work_sync(&dev->work_buffer);
}