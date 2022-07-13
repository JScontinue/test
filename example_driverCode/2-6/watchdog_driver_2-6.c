//platform_device结构体
struct platform_device{
	const char *name;//设备名
	u32 id;
	struct device dev;
	u32 num_resources;//设备所使用个类资源数量
	struct resource *resource;//资源
};

//xxxp芯片的平台设备
struct platform_device *xxxp_uart_devs[];
struct platform_device xxxp_device_usb;	//USB控制器
struct platform_device xxxp_device_lcd;	//lcd控制器
struct platform_device xxxp_device_wdt;	//看门狗
struct platform_device xxxp_device_i2c;	//I2C控制器
struct platform_device xxxp_device_iis;	//IIS
struct platform_device xxxp_device_rtc;	//实时钟
...
//xxx开发板使用的平台设备
static struct platform_device *xxx_device[]__initdata = {
	&xxxp_device_usb,//USB
	&xxxp_device_lcd,//LCD
	&xxxp_device_wdt,//看门狗
	&xxxp_device_i2c,//I2C
	&xxxp_device_iis,//IIS
};

//xxxp芯片看门狗的platform_device结构体
struct platform_device xxxp_device_wdt = {
	.name 	= "xxxp-wdt",//设备名
	.id		= -1,
	.num_resources = ARRAY_SIZE(xxxp_wdt_resource),//资源数量
	.resource = xxxp_wdt_resource,//看门狗所使用资源
};

//注册平台设备到系统中的函数
int platform_add_devices(struct platform_device **devs, int num)
{
	int i, ret = 0;
	for (i = 0; i < num; i++){
		ret = platform_device_register(devs[i]);//注册平台设备
		if(ret)//注册失败
		{
			while(--i >= 0)
				platform_device_unregister(devs[i]);//注销已经注册的平台设备
			break;
		}
	}
	return ret;
}

//platform_driver结构体
struct platform_driver{
	int (*probe) (struct platform_device *);//探测
	int (*remove) (struct platform_device *);//移除
	void (*shutdown) (struct platform_device *);//关闭
	int (*suspend) (struct platform_device *, pm_message_t state);//挂起
	int (*resume) (struct platform_device *);//恢复
	struct device_driver driver;
};

//xxxp芯片看门狗的platform_driver结构体
static struct platform_driver xxxpwdt_driver ={
	.probe	= xxxpwdt_probe,//xxxp看门狗探测
	.remove	= xxxpwdt_remove,//xxxp看门狗移除
	.shutdown = xxxpwdt_shutdown,//xxxp看门狗关闭
	.suspend = xxxpwdt_suspend,//xxxp看门狗挂起
	.resume	= xxxpwdt_resume,//xxxp看门狗恢复
	.driver = {
		.owner = THIS_MODULE,
		.name  = "xxxp-wdt",//设备名
	},
};

//xxxp看门狗所用资源
static struct resource xxxp_wdt_resource[] = {
	[0] ={
		.start 	= xxxp_PA_WATCHDOG,//看门狗I/O内存开始位置
		.end	= xxxp_PA_WATCHDOG + xxxp_SZ_WATCHDOG - 1,//看门狗I/O内存结束位置
		.flags	= IORESOURCE_MEM,//I/O内存资源
	},
	[1] ={
		.start 	= IRQ_WDT,//看门狗开始IRQ号
		.end	= IRQ_WDT,//看门狗结束IRQ号
		.flags	= IORESOURCE_IRQ,//IRQ资源
	}
};

//看门狗驱动的打开和释放函数
static int xxxpwdt_open(struct inode *inode, struct file *file)
{
	if(down_trylock(&open_lock))//获得打开锁
		return -EBUSY;
	
	if(nowayout){
		__module_get(THIS_MODULE);
	}else{
		allow_close = CLOSE_STATE_ALLOW;
	}
	
	//启动看门狗
	xxxpwdt_start();
	return nonseekable_open(inode, file);
}
static int xxxpwdt_release(struct inode *inode, struct file *file)
{
	//停止看门狗
	if (allow_close == CLOSE_STATE_ALLOW){
		xxxpwdt_stop();
	}else{
		printk(KERN_INFO PFX "Unexpected close, not stopping watchdog!\n");
		xxxpwdt_keepalive();
	}
	
	allow_close = CLOSE_STATE_NOT;
	up(&open_lock);//释放打开锁
	return 0;
}

//看门狗设备驱动中启停看门狗函数
static int xxxpwdt_stop(void)
{
	unsigned long wtcon;
	
	wtcon = readl(wdt_base + xxxp_WTCON);
	//停止看门狗，禁止复位
	wtcon &= ~(xxxp_WTCON_ENABLE | xxxp_WTCON_RSTEN);
	writel(wtcon, wdt_base + xxxp_WTCON);
	
	return 0;
}
static int xxxpwdt_start(void)
{
	unsigned long wtcon;
	
	xxxpwdt_stop();
	
	wtcon = readl(wdt_base + xxxp_WTCON);
	//使能看门狗，128分频
	wtcon |= xxxp_WTCON_ENABLE | xxxp_WTCON_DIV128;
	
	if(soft_noboot){
		wtcon |= xxxp_WTCON_INTEN; //使能中断
		wtcon &= ~xxxp_WTCON_RSTEN;//禁止复位
	}else{
		wtcon &= ~xxxp_WTCON_INTEN;//禁止中断
		wtcon |= xxxp_WTCON_RETEN;//使能复位
	}
	
	DBG("%s: wdt_count=0x%08x, wtcon=%lx\n", __FUNCTION__,
		wdt_count, wtcon);
	writel(wdt_count, wdt_base + xxxp_WTDAT);
	writel(wdt_count, wdt_base + xxxp_WTCNT);
	writel(wtcon, wdt_base + xxxp_WTCON);
	
	return 0;
}

//看门狗驱动写函数，写入“V”，关闭看门狗
static ssize_t xxxpwdt_write(struct file *filp, const char __user *data,
		size_t len, loff_t *ppos)
{
	//刷新看门狗
	if(len){
		if(!nowayout){
			size_t i;
			allow_close = CLOSE_STATE_NOT;
			for (i = 0; i != len; i++){
				char c;
				if (get_user(c, data + i))//用户空间->内核空间
					return -EFAULT;
				if (c == 'V')//写入“V”，关闭看门狗
					allow_close = CLOSE_STATE_ALLOW;
			}
		}
		xxxpwdt_keepalive();
	}
	return len;
}


//xxxp芯片看门狗驱动的miscdevice结构体
static struct miscdevice xxxpwdt_miscdev ={
	.minor 	= WATCHDOG_MINOR,//次设备号
	.name	= "watchdog",
	.fops	= &xxxpwdt_fops,//文件操作结构体
};

struct resource *platform_get_resource(struct platform_device *dev, unsigned int type, unsigned int num)
{
	int i;
	for (i = 0; i < dev->num_resources; i++){
		struct resource *r = &dev->resource[i];
		if((r->flags &(IORESOURCE_IO | IORESOURCE_MEM |
			IORESOURCE_IRQ | IORESOURCE_DMA)) == type)//如果类型匹配
			if(num-- == 0)//序号
				return r;//返回resource指针
		
	}
	return NULL;
}

//xxxp芯片看门狗驱动的探测函数
static int xxxpwdt_probe(struct platform_devixe *pdev)
{
	struct resource *res;
	int started = 0;
	int ret;
	int size;
	
	DBG("%s: probe=%p\n", __FUNCTION__, pdev);
	
	//获得看门狗的内存区域
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL){
		printk(KERN_INFO PFX "failed to get memory region resouce\n");
		return -ENOENT;
	}
	
	size = (res->end - res->start) + 1;
	//申请I/O内存
	wdt_mem = request_mem_region(res->start, size, pdev->name);
	if(wdt_mem == NULL){
		printk(KERN_INFO PFX "failed to get memory region\n");
		return -ENOENT;
	}
	
	wdt_base = ioremap(res->start, size);//设备内存->虚拟地址
	if(wdt_base == 0){
		printk(KERN_INFO PFX "failed to ioremap() region\n");
		return -EINVAL;
	}
	
	DBG("probe: mapped wdt_base=%p\n", wdt_base);
	
	//获取看门狗的中断
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if(res == NULL){
		printk(KERN_INFO PFX "failed to get irq resource\n");
		return -ENOENT;
	}
	//申请中断
	ret = request_irq(res->start, xxxpwdt_irq, 0, pdev->name, pdev);
	if(ret != 0){
		printk(KERN_INFO PFX "failed to install irq (%d)\n", ret);
		return ret;
	}
	
	wdt_clock = clk_get(&pdev->dev, "watchdog");//获取看门狗时钟源
	if(wdt_clock == NULL){
		printk(KERN_INFO PFX "failed to find watchdog clock source\n");
		return -ENOENT;
	}
	
	clk_enable(wdt_clock);
	
	//看看是否能设置定时器的超时时间为期望的值，如果不能，使用缺省值
	if(xxxpwdt_set_heartbeat(tmr_margin)){
		started = xxxpwdt_set_heartbeat(CONFIG_xxxp_WATCHDOG_DEFAULT_TIME);
		if(started == 0){
			printk(KERN_INFO PFX "tmr_margin value out of range, default %d used\n",
					CONFIG_xxxp_WATCHDOG_DEFAULT_TIME);
		}else{
			printk(KERN_INFO PFX "default timer value is out of range, cannot start\n");
		}
	}
	
	//注册miscdevice
	ret = misc_register(&xxxpwdt_miscdev);
	if(ret){
		printk(KERN_INFO PFX "cannot register miscdev on minor=%d (%d)\n",
			WATCHDOG_MINOR, ret);
		return ret;
	}
	
	if(tmr_atboot && started == 0){
		printk(KERN_INFO PFX "Starting Watchdog Timer\n");
		xxxpwdt_start();
	}
	
	return 0;
}

//xxxp芯片看门狗的移除函数
static int xxxpwdt_remove(struct platform_device *dev)
{
	if(wdt_mem != NULL){
		release_resource(wdt_mem);//释放资源
		kfree(wdt_mem);//释放内存
		wdt_mem = NULL;
	}
	
	//释放中断
	if(wdt_irq != NULL){
		free_irq(wdt_irq->start, dev);
		wdt_irq = NULL;
	}
	
	//禁止时钟源
	if(wdt_clock != NULL){
		clk_disable(wdt_clock);
		clk_put(wdt_clock);
		wdt_clock = NULL;
	}
	
	misc_deregister(&xxxpwdt_miscdev);//注销miscdevice
	return 0;
}

//看门狗设备驱动的挂起、恢复函数
static int xxxpwdt_suspend(struct platform_device *dev, pm_message_t state)
{
	//保存看门狗状态（计数值、目标值），停止它
	wtcon_save = readl(wdt_base + xxxp_WTCON);
	wtdat_save = readl(wdt_base + xxxp_WTDAT);
	
	xxxpwdt_stop();
	
	return 0;
}
static int xxxpwdt_resume(struct platform_device *dev)
{
	//恢复看门狗状态
	writel(wtdat_save, wdt_base + xxxp_WTDAT);
	writel(wtdat_save, wdt_base + xxxp_WTCNT);
	writel(wtcon_save, wdt_base + xxxp_WTCON);
	
	printk(KERN_INFO PFX "watchdog %sadled\n", (wtcon_save&xxxp_WTCON_ENABLE)? "en" : "dis");
	return 0;
}

//xxxp芯片看门狗驱动的文件操作结构体
static struct file_operations xxxpwdt_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,//seek
	.write = xxxpwdt_write,//写函数
	.ioctl = xxxpwdt_ioctl,//ioctl函数
	.open  = xxxpwdt_open, //打开函数
	.release = xxxpwdt_release,//释放函数
};

static int __init watchdog_init(void)
{
	printk(banner);
	return platform_driver_register(&xxxpwdt_driver);//注册platform_driver
}
static void __exit watchdog_exit(void)
{
	platform_driver_unregister(&xxxpwdt_driver);//注销platform_
}


	