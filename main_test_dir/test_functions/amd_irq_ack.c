
static void amd_irq_ack(struct irq_data *d)
{
	/*
	 * based on HW design,there is no need to ack HW
	 * before handle current irq. But this routine is
	 * necessary for handle_edge_irq
	*/
}