//块设备驱动的模块加载函数（使用blk_alloc_queue）
static int __init xxx_init(void)
{
	//分配gendisk
	xxx_disks =  alloc_disk(1);
	if(!xxx_disks){
		goto out;
	}
	
	//块设备驱动注册
	if(register_blkdev(XXX_MAJOR, "xxx")){
		err = -EIO;
		goto out;
	}
	
	//“请求队列”分配
	xxx_queue = blk_alloc_queue(GFP_KERNEL);
	if(!xxx_queue){
		goto out_queue;
	}
	
	blk_queue_make_request(xxx_queue, &xxx_make_request);//绑定“制造请求”函数
	blk_queue_hardsect_size(xxx_queue, xxx_blocksize);//硬件扇区尺寸设置
	
	//gendisk初始化
	xxx_disks->major = XXX_MAJOR;
	xxx_disks->first_minor = 0;
	xxx_disks->fops = &xxx_op;
	xxx_disks->queue = xxx_queue;
	sprintf(xxx_disks->disk_name, "xxx%d", i);
	set_capacity(xxx_disks, xxx_size);//xxx_size以512bytes为单位
	add_disk(xxx_disk);//添加gendisk
	
	return 0;

out_queue: unregister_blkdev(XXX_MAJOR, "xxx");
out: put_disk(xxx_disks);
	 blk_cleanup_queue(xxx_queue);
	 
	 return -ENOMEM;
}