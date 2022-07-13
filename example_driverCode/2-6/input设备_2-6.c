/*在按键中断中报告事件*/
static void button_interrupt(int irq, void *dummy, struct pt_regs *fp)
{
	input_report_key(&button_dev, BTN_1, inb(BUTTON_PORT), &1);
	input_syns(&button_dev);
}

static int __init button_init(void)
{
	//申请中断
	if(request_irq(BUTTON_IRQ, button_interrupt, 0, "button", NULL)){
		printk(KERN_ERR "button.c: Can't allocate irq %d\n", button_irq);
		return - EBUSY;
	}
	
	button_dev.evbit[0] = BIT(EV_KEY);//支持EV_KEY事件
	button_dev.keybit[LONG(BTN_0)] = BIT(BTN_0);
	
	input_register_device(&button_dev);//注册input设备
}

static void __exit button_exit(void)
{
	input_unregister_device(&button_dev);//注销input设备
	free_irq(BUTTON_IRQ, button_interrupt);//释放中断
}