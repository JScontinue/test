//触摸屏设备结构体
typedef struct{
	unsigned int pen_status; /*PEN_UP、PEN_DOWN、PEN_SAMPLE*/
	TS_RET buf[MAX_TS_BUF];  /*缓冲区*/
	unsigned int head, tail; /*缓冲区头和尾*/
	wait_queue_head_t wq;	 /*等待队列*/
	spinlock_t lock;
	#ifdef USE_ASYNC
		struct fasync_struct *aq;
	#endif
	struct cdev cdev;
} TS_DEV;

typedef struct{
	unsigned short pressure;//PEN_DOWN、PEN_UP
	unsigned short x;		//x坐标
	unsigned short y;		//y坐标
	unsigned short pad;
} TS_RET;

#define wait_down_int() {ADCTSC = DOWN_INT | XP_PULL_UP_EN |\
				XP_AIN | XM_HIN | YP_AIN | YM_GND |\
				XP_PST(WAIT_INT_MODE);}
#define wait_up_int()	{ADCTSC = UP_INT | XP_PULL_UP_EN | XP_AIN |\
				XM_AIZ | YP_AIN | YM_GND | XP_PST(WAIT_INT_MODE);}
#define mode_x_axis()	{ADCTSC = XP_EXTVLT | XM_GND | YP_AIN |\
				YM_HIZ | XP_PULL_UP_DIS | XP_PST(X_AXIS_MODE);}
#define mode_x_axis_n()	{ADCTSC = XP_EXTVLT | XM_GND | YP_AIN |\
				YM_HIZ | XP_PULL_UP_DIS | XP_PST(NOP_MODE);}
#define mode_y_axis()	{ADCTSC = XP_AIN | XM_HIZ | YP_EXTVLT |\
				YM_GND | XP_PULL_UP_DIS | XP_PST(Y_AXIS_MODE);}
#define start_adc_x()	{ADCSTC = PRESCALE_EN | PRSCVL(49) |\
				ADC_INPUT(ADC_IN5) | ADC_START_BY_RD_EN |\
				ADC_NORMAL_MODE;\
			ADCDAT0;}
#define start_adc_y()	{ADCCON = PRESCALE_EN | PRSCVL(49) |\
				ADC_INPUT(ADC_IN7) | ADC_START_BY_RD_EN |\
				ADC_NORMAL_MODE;\
			ADCDAT1;}
#define disable_ts_adc()	{ADCCON &= ~(ADCCON_READ_START);}

//触摸屏设备驱动中获得X/Y坐标
static inline void xxx_get_XY(void)
{
	if(adc_state == 0){
		adc_state = 1;
		disable_ts_adc();//禁止INT_ADC
		y = (ADCDAT0 & 0x3ff);//读取坐标值
		mode_y_axis();
		start_adc_y();//开始y位置转换
	}else if(adc_state == 1){
		adc_state = 0;
		disable_ts_adc();//禁止INT_ADC
		x = (ADCDAT1 & 0x3ff);//读取坐标值
		tsdev.pen_status = PEN_DOWN;
		DPRINTK("PEN DOWN: x: %08d, y: %08d\n", x, y);
		wait_up_int();
		tsEvent();
	}
}

//触摸屏设备驱动的tsEvent_raw()函数
static void tsEvent_raw(void)
{
	if(tsdev.pen_status == PEN_DOWN){
		/*填充缓冲区*/
		BUF_HEAD.x = x;
		BUF_HEAD.y = y;
		BUF_HEAD.pressure = PEN_DOWN;
		
		#ifdef HOOK_FOR_DRAG
			ts_timer.expires = jiffies + TS_TIMER_DELAY;
			add_timer(&ts_timer);//启动定时器
		#endif
	}else{
		#ifdef HOOK_FOR_DRAG
			del_timer(&ts_timer);
		#endif
		/*填充缓冲区*/
		BUF_HEAD.x = 0;
		BUF_HEAD.y = 0;
		BUF_HEAD.pressure = PEN_UP;
	}
	
	tsdev.head = INCBUF(tsdev.head, MAX_TS_BUF);
	wake_up_interruptibles(&(tsdev.wq));//唤醒等待队列
	
	#ifdef USE_ASYNC
		if(tsdev.aq)
			kill_fasync(&(tsdev.aq), SIGIO, POLL_IN);//异步通知
	#endif	
}

//触摸屏设备驱动的定时器处理函数
#ifdef HOOK_FOR_DRAG
static void ts_timer_handler(unsigned long data)
{
	spin_lock_irq(&(tsdev.lock));
	if(tsdev.pen_status == PEN_DOWN){
		start_ts_adc();//开始X/Y位置转换
	}
	spin_unlock_irq(&(tsdev.lock));
}
#endif
}

//触摸屏设备驱动的触点/抬起中断处理程序
static void xxx_isr_tc(int irq, void *dev_id, struct pt_regs *reg)
{
	spin_lock_irq(&(tsdev.lock));
	if(tsdev.pen_status == PEN_UP){
		start_ts_adc();//开始X/Y位置转换
	}else{
		tsdev.pen_status = PEN_UP;
		DPRINTK("PEN UP: x: %08d, y: %08d\n", x, y);
		wait_down_int();//置于等待触点中断模式
		tsEvent();
	}
	spin_unlock_irq(&(tsdev.lock));
}
//触摸屏设备驱动X/Y位置转换中断处理程序
static void xxx_isr_adc(int irq, void *dev_id, struct pt_regs *reg)
{
	spin_lock_irq(&(tsdev.lock));
	if(tsdev.pen_status == PEN_UP)
		xxx_get_XY();//读取坐标
	#ifdef HOOK_FOR_DRAG
		else
			xxx_get_XY();
	#endif
	spin_unlock_irq(&(tsdev.lock));
}

//触摸屏设备驱动的打开函数
static int xxx_ts_open(struct inode *inode, struct file *filp)
{
	tsdev.head = tsdev.tail = 0;
	tsdev,pen_status = PEN_UP;//初始化触摸屏状态为PEN_UP
	#ifdef HOOK_FOR_DRAG//如果定义了拖动钩子函数
		init_timer(&ts_timer);//初始化定时器
		ts_timer.function = ts_timer_handler;
	#endif
	tsEvent = tsEvent_raw;
	init_waitqueue_head(&(tsdev.wq));//初始化等待队列
	return 0;
}

//触摸屏设备驱动的释放函数
static int xxx_ts_release(struct inode *inode, struct file *filp)
{
	#ifdef HOOK_FOR_DRAG//如果定义了拖动钩子函数
		del_timer(&ts_timer);
	#endif
	return 0;
}

//触摸屏设备驱动的读函数
static ssize_t xxx_ts_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
	TS_RET ts_ret;
	
	retry:
	if(tsdev.head != tsdev.tail){//缓冲区有数据
		int count;
		count = tsRead(&ts_ret);
		if(count)
			copy_to_user(buffer, (char *) &ts_ret, count);//复制到用户空间
		return count;
	}else{
		if(filp->f_flags & O_NONBLOCK)//非阻塞读
			return - EAGAIN;
		interruptible_sleep_on(&(tsdev.wq));//在等待队列上睡眠
		if(signal_pending(current))
			return - ERESTARTSYS;
		goto retry;
	}
	return sizeof(TS_RET);
}

//触摸屏设备驱动的poll()函数
static unsigned int xxx_ts_poll(struct file *filp, struct poll_table_struct *wait)
{
	poll_wait(filp, &(tsdev.wq), wait);//添加等待队列到poll_table
	return (tsdev.head == tsdev.tail) ? 0 : (POLLIN | POLLRDNORM);
}

//实现触摸屏设备驱动对应用程序的异步通知，fasync()函数
#ifdef USE_ASYNC
static int xxx_ts_fasync(int fd, struct file *filp, int mode)
{
	return fasync_helper(fd, filp, mode, &(tsdev.aq));
}
#endif


static struct file_operations xxx_fops ={
	.owner	= THIS_MODULE,
	.open	= xxx_ts_open,//打开
	.read	= xxx_ts_read,//读坐标
	.release= xxx_ts_release,
	#ifdef USE_ASYNC
		.fasync = xxx_ts_fasync,//fasync()函数
	#endif
	.poll	= xxx_ts_poll,//轮询
};

static int __init xxx_ts_init(void)
{
	int ret;
	tsEvent = tsEvent_dummy;
	...//申请设备号，添加cdev
	
	/*设置XP、YM、YP和YM对应引脚*/
	set_gpio_ctrl(GPIO_YPON);
	set_gpio_ctrl(GPIO_YMON);
	set_gpio_ctrl(GPIO_XPON);
	set_gpio_ctrl(GPIO_XMON);
	
	/*使能触摸屏中断*/
	ret = requset_irq(IRQ_ADC_DONE, xxx_isr_adc, SA_INTERRUPT, DEVICE_NAME, xxx_isr_adc);
	if(ret)
		goto adc_failed;
	ret = requset_irq(IRQ_TC, xxx_isr_tc, SA_INTERRUPT, DEVICE_NAME, xxx_isr_tc);
	if(ret)
		goto tc_failed;
		
	/*置于等待触点中断模式*/
	wait_down_int();
	
	printk(DEVICE_NAME "initialized\n");
	return 0;
tc_failed:
	free_irq(IRQ_ADC_DONE, xxx_isr_adc);
adc_failed:
	return ret;
}

static void __exit xxx_ts_exit(void)
{
	...//释放设备号，删除cdev
	free_irq(IRQ_ADC_DONE, xxx_isr_adc);
	free_irq(IRQ_RC, xxx_isr_tc);
}
