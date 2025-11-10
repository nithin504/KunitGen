
void amd_pmf_handle_amt(struct amd_pmf_dev *dev)
{
	amd_pmf_set_automode(dev, config_store.current_mode, NULL);
}