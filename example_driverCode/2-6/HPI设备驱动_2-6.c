//HPI接口地址、数据和控制寄存器访问
#define HPI_BASEADDR	0x08000000	//BANK 1
#define bHPI(Nb)		__REG1(HPI_BASEADDR + (Nb))
#define HPIC_WRITE		bHPI(0x0)	//EA3(HCNTL0):0 EA4(HCNTL1):0 EA5:0(w)
#define HPIC_READ		bHPI(0x20)	//EA3(HCNTL0):0 EA4(HCNTL1):0 EA5:1(r)
#define HPIA_WRITE		bHPI(0x08)
#define HPIA_READ		bHPI(0x48)
#define HPID_WRITE		bHPI(0x10)
#define HPID_READ		bHPI(Ox30)	

//HPI接口设备结构体
typedef struct{
	struct semaphore sem;	//信号量
	char *HpiBaseBufRead;	//读缓冲区指针
	char *HpiBaseBufWrite;	//写缓冲区指针
	struct cdev cdev;
}HPI_DEVICE;

//HPI接口设备驱动的读函数
static ssize_t hpi_read(struct file *filp, char *buf, size_t count, loff_t *oppos)
{
	...
	down(&(pHpiDevice->sem));//获取并发访问信号量
	HPIC_WRITE = AUTO_READ_FLAGS;//表明要读DSP内存
	HPIA_WRITE = *oppos;//读的起始地址
	count = min(count, MAX_DSP_ADDRESS - *oppos);
	for(i = 0; i < count; i++)//连续地都count个数据
	{
		read_buf[i] = HPID_READ;//读取到HpiBaseBufRead缓冲区
	}
	
	ret = copy_to_user(buf, read_buf, count) ? -EFAULT : ret;//复制到HpiBaseBufRead缓冲区
	...
	
	up(&(pHpiDevice->sem));//释放并发访问信号量
	return count;
}

//HPI接口设备驱动的写函数
static ssize_t hpi_write(struct file *file, char *buf, size_t count, loff_t *oppos)
{
	...
	ret = copy_from_user(write_buf, buf, count) ? -EFAULT : ret;
	...
	down(&(pHpiDevice->sem));
	HPIC_WRITE = AUTO_WRITE_FLAGS;//表明要读DSP内存
	HPIA_WRITE = *oppos;//写的起始地址
	count = min(count, MAX_DSP_ADDRESS-*oppos);
	for(i = 0; i < count; i++){
		HPID_WRITE = write_buf[i];//读取到HpiBaseBufRead缓冲区
	}
	...
	up(&(pHpiDevice->sem));
	return count;
}

static struct file_operations hpi_fops = {
	.owner	= THIS_MODULE,
	.open	= hpi_open,		//打开函数
	.read	= hpi_read,		//读函数
	.write	= hpi_write,	//写函数
	.ioctl	= hpi_ioctl,	//ioctl()函数
	.release= hpi_release,	//释放函数
};

