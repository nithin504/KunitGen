
void amd_pmf_init_auto_mode(struct amd_pmf_dev *dev)
{
	amd_pmf_load_defaults_auto_mode(dev);
	amd_pmf_init_metrics_table(dev);
}