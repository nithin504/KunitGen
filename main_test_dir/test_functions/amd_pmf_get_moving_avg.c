
static int amd_pmf_get_moving_avg(struct amd_pmf_dev *pdev, int socket_power)
{
	int i, total = 0;

	if (pdev->socket_power_history_idx == -1) {
		for (i = 0; i < AVG_SAMPLE_SIZE; i++)
			pdev->socket_power_history[i] = socket_power;
	}

	pdev->socket_power_history_idx = (pdev->socket_power_history_idx + 1) % AVG_SAMPLE_SIZE;
	pdev->socket_power_history[pdev->socket_power_history_idx] = socket_power;

	for (i = 0; i < AVG_SAMPLE_SIZE; i++)
		total += pdev->socket_power_history[i];

	return total / AVG_SAMPLE_SIZE;
}