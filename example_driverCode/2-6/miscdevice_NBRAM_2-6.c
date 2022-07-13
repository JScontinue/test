//miscdevice结构体
struct miscdevice{
	int minor;
	const char *name;//设备名
	struct file_operations *fops;//文件操作
	struct list_head list;//链表头
	struct device *dev;
	struct class_device *class;
	char devfs_name[64];//devfs文件节点名
};

//NVRAM设备结构体
static struct miscdevice nvram_dev ={
	NVRAM_MINOR,//次设备号
	"nvram",//设备名
	&nvram_fops//文件操作结构体
};

//NVRAM设备驱动读写函数
static ssize_t read_nvram(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	unsigned int i;
	char __user *p = buf;
	if(!access_ok(VERIFY_WRITE, buf, count))
		return -EFAULT;
	//偏移过大
	if(*ppos >= NVRAM_SIZE)
		return 0;
	for(i = *ppos; count > 0 && i < NVRAM_SIZE; ++i, ++p, --count)
	//读取1字节并复制到用户空间
	{
		if(__put_user(nvram_read_byte(i), p))
			return -EFAULT;
	}
	*ppos = i;
	return p-buf;
}
static ssize_t write_nvram(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned int i;
	const char __user *p = buf;
	char c;
	
	if(!access_ok(VERIFY_READ, buf, count))
		return -EFAULT;
	//偏移过大
	if(*ppos >= NVRAM_SIZE)
		return 0;
	for (i = *ppos; count > 0 && i <NVRAM_SIZE; ++i, ++p, --count){
		//从用户空间取得1字节并写入NVRAM
		if(__get_user(c, p))
			return -EFAULT;
		nvram_write_byte(c, i);
	}
	*ppos = i;
	return p-buf;
}

//NVRAM设备驱动的seek函数
static loff_t nvram_llseek(struct file *file, loff_t offset, int origin)
{
	lock_kernel();//大内核锁，已经过时
	switch(origin){
		case 1://相对当前位置偏移
			offset += file->f_pos;
			break;
		case 2://相对文件尾偏移
			offset += NVRAM_SIZE;
			break;
	}
	if(offset < 0){
		unlock_kernel();
		return -EINVAL;
	}
	file->f_pos = offset;
	unlock_kernel();
	return file->f_pos;
}

//NVRAM设备驱动文件操作结构体
struct file_operations nvram_fops ={
	.owner	= THIS_MODULE,
	.llseek	= nvram_llseek,//seek
	.read	= read_nvram,//读
	.write	= write_nvram,//写
	.ioctl	= nvram_ioctl,//IO控制
};

int __init nvram_init(void)
{
	printk(KERN_INFO "Macintosh non-volatile memory driver v%s\n", NVRAM_VERSION);
	return misc_register(&nvram_dev);//注册miscdevice
}

void __exit nvram_cleanup(void)
{
	misc_deregister( &nvram_dev );//注销miscdevice
}