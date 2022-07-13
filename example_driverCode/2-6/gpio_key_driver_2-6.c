//按键驱动的设备结构体、定时器
#define MAX_KEY_BUF 16 					//按键缓冲区大小
typedef unsigned char KEY_RET;
//设备结构体
typedef struct{
	unsigned int key_status[KEY_NUM];	//按键的按键状态
	KEY_RET buf[MAX_KEY_BUF];	//按键缓冲区
	unsigned int head, tail;	//按键缓冲区头和尾
	wait_queue_head_t wq;		//等待队列
	struct cdev cdev;			//cdev结构体
}KEY_DEV;
static struct timer_list key_timer[KEY_NUM];//按键去抖定时器

//按键硬件资源、键值信息结构体
static struct key_info{
	int irq_no;		//中断号
	unsigned int gpio_port;	//GPIO端口
	int key_no;		//键值
} key_info_tab[KEY_NUM] =
{
	/*按键所使用的CPU资源*/
	{IRQ_EINT10, GPIO_G2, 1},
	{IRQ_EINT13, GPIO_G5, 2},
	{IRQ_EINT14, GPIO_G6, 3}，
	{IRQ_EINT15, GPIO_G7, 4},
};

//申请系统中断，中断方式为下降沿触发
static int request_irqs(void)
{
	struct key_info *k;
	int i;
	for(i = 0; i < sizeof(key_info_tab)/sizeof(key_info_tab[1]); i++){
		k = key_info_tab + i;
		set_external_irq(k->irq_no, EXI_LOWLEVEL, GPIO_PULLUP_DIS);//设置低电平触发
		//申请中断，将按键序号作为参数传入中断服务函数
		if(request_irq(k->irq_no, &buttons_irq, SA_INTERRUPT, DEVICE_NAME, i)){
			return -1;
		}
	}
	return 0;
}

//按键设备驱动的中断释放函数
static void free_irqs(void)
{
	struct key_info *k;
	int i;
	for(i = 0; i < sizeof(key_info_tab)/sizeof(key_info_tab[1]); i++){
		k = key_info_tab + i;
		free_irq(k->irq_no, buttons_irq);//释放中断
}

//按键设备驱动的中断处理函数
static void xxx_eint_key(int irq, void *dev_id, struct pt_regs *reg)
{
	int key = dev_id;
	disable_irq(key_info_tab[key].irq_no);//关中断，转入查询模式
	
	keydev.key_status[key] = KEYSTATUS_DOWNX;//状态为按下
	key_timer[key].expires == jiffies + KEY_TIMER_DELAY1;//延迟
	add_timer(&key_timer[key]);//启动定时器
}

//按键设备驱动的定时器处理函数
static void key_timer_handler(unsigned long data)
{
	int key = data;
	if(ISKEY_DOWN(key)){
		if(keydev.key_status[key] == KEYSTATUS_DOWNX){
			keydev.key_status[key] == jiffies + KEY_TIMER_DELAY;//延迟
			keyEvent();//记录键值，唤醒等待队列
			add_timer(&key_timer[key]);
		}else{
			key_timer[key].expires == jiffies + KEY_TIMER_DELAY;//延迟
			add_timer(&key_timer[key]);
		}
	}else{//键以抬起
		keydev.key_status[key] = KEYSTATUS_UP;
		enable_irq(key_info_tab[key].irq_no);
	}
}

//按键设备驱动的打开、释放函数
static int xxx_key_open(struct inode *inode, struct file *filp)
{
	keydev.head = keydev.tail = 0;//清空按键动作缓冲区
	keyEvent = keyEvent_raw; //函数指针指向按键处理函数keyEvent_raw
	return 0;
}

static int xxx_key_release(struct inode *inode, struct file *filp)
{
	keyEvent = keyEvent_dummy;//函数指针指向空函数
	return 0;
}

//按键驱动的读函数
static ssize_t xxx_key_read(struct file *filp, char *buf, ssize_t count, loff_t *ppos)
{
	retry: if(keydev.head != keydev.tail)//当前循环队列中有数据
	{
		key_ret = keyRead(); //读取按键
		copy_to_user(..);//把数据从内核空间传送到用户空间
	}else{
		if(filp->f_flags & O_NONBLOCK)//若用户采用非阻塞方式读取
		{
			return - EAGAIN;
		}
		interruptible_sleep_on(&(keydev.wq));
		//若用户采用阻塞方式读取，调用该函数是进程睡眠
		goto retry;
	}
	return 0;
}

//按键设备驱动文件操作结构体
static struct file_operations xxx_key_fops = {
	.owner 	= THIS_MODULE,
	.open	= xxx_key_open,	//启动设备
	.release= xxx_key_release, //关闭设备
	.read	= xxx_key_read,	 //读取按键的键值
};

static int __init xxx_key_init(void)
{
	...//申请设备号，添加cdev
	
	request_irqs();//注册中断函数
	keydev.head = keydev.tail = 0;//初始化结构体
	for(i = 0; i < KEY_NUM; i++)
		keydev.key_status[i] = KEYSTATUS_UP;
	init_waitqueue_head(&(keydev.wq));//等待队列
	
	//初始化定时器，实现软件的去抖动
	for(i = 0; i < KEY_NUM; i++)
		setup_timer(&key_timer[i], key_timer_hander, i);//把按键的序号作为传入定时器处理函数的参数
	
}	

static void __exit xxx_key_exit(void)
{
	free_irqs();//注销中断
	...//释放设备号，删除cdev
}


