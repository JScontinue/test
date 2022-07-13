//在块设备的open( )函数中赋值private_data
static int xxx_open(struct inode *inode, struct file *filp)
{
	struct xxx_dev *dev = inode->i_bdev->bd_disk->private_data;
	filp->private_data = dev;//赋值file的private_data
	...
	return 0;
}

//块设备驱动的I/O控制函数模板
int xxx_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	long size;
	struct ha_geometry geo;
	struct xxx_dev *dev = filp->private_data;//通过file->private获取设备结构体
	
	switch(cmd){
		case HDIO_GETGEO:
			size = dev->size * (hardsect_size / KERNEL_SECTOR_SIZE);
			geo.cylinders = (size & ~0x3f) >> 6;
			geo.heads = 4;
			geo.sectors = 16;
			geo.start = 4;
			if (copy_to_user((void __user*)arg, &geo, sizeof(geo))){
				return -EFAULT;
			}
			return 0;
	}
	
	return -ENOTTY;//不知道的命令
}


//块设备驱动的模块加载函数模板使用blk_init_queue函数
static int __init xxx_init(void)
{
	//块设备驱动注册
	if (register_blkdev(XXX_MAJOR, "xxx")){
		err = -EIO;
		goto out;
	}
	
	//请求队列初始化
	xxx_queue = blk_init_queue(xxx_request, xxx_lock);
	if(!xxx_queue){
		goto out_queue;
	}
	
	blk_queue_hardsect_size(xxx_queue, xxx_blocksize);//硬件扇区尺寸设置
	
	//gendisk初始化
	xxx_disks->major = XXX_MAJOR;
	xxx_disks->first_minor = 0;
	xxx_disks->fops = &xxx_op;
	xxx_disks->queue = xxx_queue;
	sprintf(xxx_disks->disk_name, "xxx%d", i);
	set_capacity(xxx_disks, xxx_size * 2);
	add_disk(xxx_disks);//添加gendisk
	
	return 0;

out_queue: unregister_blkdev(XXX_MAJOR, "xxx");
out: put_disk(xxx_disks);
	 blk_cleanup_queue(xxx_queue);
	 
	 return -ENOMEM;
}

static void __exit xxx_exit(void)
{
	if(bdev){
		invalidate_bdev(xxx_bdev, 1);
		blkdev_put(xxx_bdev);
	}
	del_gendisk(xxx_disks);//删除gendisk
	put_disk(xxx_disks);
	blk_cleanup_queue(xxx_queue[i]);//清除请求队列
	unregister_blkdev(XXX_MAJOR, "xxx");
}