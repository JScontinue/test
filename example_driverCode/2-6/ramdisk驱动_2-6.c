//ramdisk驱动

static int rd_make_request(request_queue_t *q, struct bio *bio)
{
	struct block_device *bdev = bio->bi_bdev;
	struct address_space *mapping = bdev->bd_inode->i_mapping;
	sector_t sector = bio->bi_sector;
	unsigned long len = bio->bi_size >> 9;
	int rw = bio_data_dir(bio);//数据传输方向：读/写
	struct bio_vec *bvec;
	int ret = 0, i;
	
	if (sector + len > get_capacity(bdev->bd_disk))
		//超出容量
		goto fail;
	
	if (rw == READA)
		rw = READ;
	//遍历每个段
	bio_for_each_segment(bvec, bio, i){
		ret |= rd_blkdev_pagecache_IO(rw, bvec, sector, mapping);
		sector += bvec->bv_len >> 9;
	}
	if (ret)
		goto fail;
	
	bio_endio(bio, bio->bi_size, 0);//处理结束
	return 0;
	
fail:
	bio_io_error(bio, bio->bi_size);
	return 0;

}

static int rd_open(struct inode *inode, struct file *filp)
{
	unsigned unit = iminor(inode);//获取次设备号
	
	if (rd_bdev[unit] == NULL){
		struct block_device *bdev = inode->i_bdev;//获取block_device结构体指针
		struct address_space *mapping;//地址空间
		unsigned bsize;
		gfp_t gfp_mask;
		
		//设置inode成员
		inode = igrab(bdev->bd_inode);
		rd_bdev[unit] = bdev;
		bdev->bd_openers++;
		bsize = bdev_hardsect_size(bdev);
		bdev->bd_block_size = bsize;
		inode->bd_block_size = bsize;
		inode->i_blkbits = blksize_bits(bsize);
		
		mapping = inode->i_mapping;
		mapping->a_ops = &ramdisk_aops;
		mapping->backing_dev_info = &rd_backing_dev_info;
		bdev->bd_inode_backing_dev_info = &rd_file_backing_dev_info;
		
		gfp_mask = mapping_gfp_mask(mapping);
		gfp_mask &= ~(__GFP_FS|__GFP_IO);
		gfp_mask |= __GFP_HIGH;
		mapping_set_gfp_mask(mapping, gfp_mask);
	}
	return 0;
}

static int rd_ioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	int error;
	struct block_device *bdev = inode->i_bdev;
	
	if (cmd != BLKFLSBUF)//不是flush buffer cache命令
		return -ENOTTY;
	//刷新buffer cache
	error = -EBUSY;
	down(&bdev->bd_sem);
	if (bdev->bd_openers <= 2){
		truncate_inode_pages(bdev->bd_inode->i_mapping, 0);
		error = 0;
	}
	up(&bdev->bd_sem);
	return error;
}

static struct block_device_operations rd_bd_op = {
	.owner = THIS_MODULE,
	.open = rd_open,
	.ioctl = rd_ioctl,
};

static int __init rd_init(void)
{
	int i;
	int err = -ENOMEM;
	
	//调整块尺寸
	if (rd_blocksize > PAGE_SIZE || rd_blocksize < 512 || (rd_blocksize & (rd_blocksize - 1))){
		printk("RAMDISK: wrong blocksize %d, reverting to defaults\n", rb_blocksize);
		rd_blocksize = BLOCK_SIZE;
	}
	
	//分配gendisk
	for (i = 0; i < CONFIG_BLK_DEV_RAM_COUNT; i++){
		rd_disks[i] = alloc_disk(1); //分配gendisk
		if (!rd_disks[i])
			goto out;
	}
	
	//块设备注册
	if (register_blkdev(RAMDISK_MAJOR, "ramdisk"){
		err = -EIO;
		goto out;
	}
	
	devfs_mk_dir("rd");//创建devfs目录
	
	for (i =0; i < CONFIG_BLK_DEV_RAM_COUNT; i++){
		struct gendisk *disk = rd_disks[i];
		//分配并绑定请求队列与“制造请求”函数
		rd_queue[i] = blk_alloc_queue(GFP_KERNEL);
		if (!rd_queue[i])
			goto out_queue;
		blk_queue_make_request(rd_queue[i], &rd_make_request);//绑定“制造请求”函数
		blk_queue_hardsect_size(rd_queue[i], rd_blocksize);//硬件扇区尺寸设置
		
		//初始化gendisk
		disk->major = RAMDISK_MAJOR;
		disk->first_minor = i;
		disk->fops = &rd_bd_op;
		disk->queue = rd_queue[i];
		disk->flags |= GENHD_FL_SUPPRESS_PARTITION_INFO;
		sprintf(disk->disk_name, "ram%d", i);
		sprintf(disk->devfs_name, "rd/%d", i);
		add_disk(rd_disks[i]);//添加gendisk
	}
	
	//rd_size以KB为单位
	printk("RAMDISK driver initalized:"
		"%d RAMDISKs of %dK size %d blocksize\n",
		CONFIG_BLK_DEV_RAM_COUNT, rd_size, rd_blocksize);
	
	return 0;
	
out_queue: unregister_blkdev(RAMDISK_MAJOR, "ramdisk");
out:	while(i--){
			put_disk(rd_disks[i];
			blk_cleanup_queue(rd_queue[i]);
		}
	return err;
}

static void __exit rd_cleanup(void)
{
	int i;
	for (i = 0; i < CONFIG_BLK_DEV_RAM_COUNT; i++){
		struct block_device *bdev = rd_bdev[i];
		rd_bdev[i] = NULL;
		if (bdev){
			invalidate_bdev(bdev, 1);
			blkdev_put(bdev);
		}
		del_gendisk(rd_disks[i]);//删除gendisk
		put_disk(rd_disks[i]);//释放对gendisk的引用
		blk_cleanup_queue(rd_queue[i]);//清除请求队列
	}
	devfs_remove("rd");
	unregister_blkdev(RAMDISK_MAJOR, "ramdisk");//块设备注销
}