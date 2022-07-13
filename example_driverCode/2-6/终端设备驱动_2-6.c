

//在tty驱动打开函数中赋值tty_struct的driver_data成员
//设备“私有“数据结构体
struct xxx_tty{
	struct tty_struct *tty;//tty_struct指针
	int open_conut;//打开次数
	struct semaphore sem;//结构体锁定信号量
	int xmit_buf;//传输缓冲区
	...
};
static int xxx_open(struct tty_struct *tty, struct file *file)
{
	struct xxx_tty *xxx;
	
	//分配xxx_tty
	xxx = kmalloc(sizeof(*xxx), GFP_KERNEL);
	if (!xxx)
		return -ENOMEM;
	
	//初始化xxx_tty中的成员
	init_MUTEX(&xxx->sem);
	xxx->open_conut = 0;
	...
	
	//使tty_struct中的driver_data指向xxx_tty
	tty->driver_data = xxx;
	xxx->tty = tty;
	...
	return 0;
}

static int xxx_write(struct tty_struct *tty, const unsigned char *buf, int count)
{
	//获得tty设备私有数据
	struct xxx_tty *xxx = (struct xxx_tty*)tty->driver_data;
	...
	//开始发送
	while(1){
		local_irq_save(flags);
		c = min_t(int, count, min(SERIAL_XMIT_SIZE - xxx->xmit_cnt - 1,
				SERIAL_XMIT_SIZE - xxx->xmit_head));
				
		if (c <= 0){
			local_irq_restore(flags);
			break;
		}
		
		//复制到发送缓冲区
		memcpy(xxx->xmit_buf + xxx->xmit_head, buf, c);
		xxx->xmit_head = (xxx->xmit_head + c) & (SERIAL_XMIT_SIZE - 1);
		xxx->xmit_cnt += c;
		local_irq_restore(flags);
		
		buf += c;
		count -= c;
		total += c;
	}
	
	if (xxx->xmit_cnt && !tty->stopped && !tty->hw_stopped){
		start_xmit(xxx);//开始发送
	}
	
	return total;//返回发送的字节数
}


//终端设备驱动的模块加载函数
static int __init xxx_init(void)
{
	...
	//分配tty_driver结构体
	xxx_tty_driver = alloc_tty_driver(XXX_PORTS);
	//初始化tty_driver结构体
	xxx_tty_driver->owner = THIS_MODULE;
	xxx_tty_driver->devfs_name = "tts/";
	xxx_tty_driver->name = "ttyS";
	xxx_tty_driver->major = TTY_MAJOR;
	xxx_tty_driver->minor_start = 64;
	xxx_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	xxx_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	xxx_tty_driver->init_termios = tty_std_termios;
	xxx_tty_driver->init_termios.c_cflag = B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	xxx_tty_driver->flags = TTY_DRIVER_REAL_RAW;
	tty_set_operations(xxx_tty_driver, &xxx_ops);
	
	ret = tty_register_driver(xxx_tty_driver);
	if (ret){
		printk(KERN_ERR "Couldn't register xxx serial driver\n");
		put_tty_driver(xxx_tty_driver);
		return ret;
	}
	
	...
	ret = request_irq(...);//硬件资源申请
	...
}