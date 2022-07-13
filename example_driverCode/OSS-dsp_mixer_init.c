//音频文件操作
static struct file_operations smdk2410_audio_fops = {
    llseek: smdk2410_audio_llseek,
    write: smdk2410_audio_write,
    read: smdk2410_audio_read,
    poll: smdk2410_audio_poll,
    ioctl: smdk2410_audio_ioctl,
    open: smdk2410_audio_open,
    release: smdk2410_audio_release
};

//混音器文件操作
static struct file_operations smdk2410_mixer_fops = {
    ioctl: smdk2410_mixer_ioctl,
    open: smdk2410_mixer_open,
    release: smdk2410_mixer_release
};

static __init s3c2410_uda1341_init(void)
{
    unsigned long flags;

    local_irq_save(flags);

    //设置IIS接口引脚gpio
    set_gpio_ctrl(GPIO_L3CLOCK);//GPB 4:L3CLOCK,输出
    set_gpio_ctrl(GPIO_L3DATA);//GPB 3:L3DATA,输出
    set_gpio_ctrl(GPIO_L3MODE);//GPB 2:L3MODE，输出

    set_gpio_ctrl(GPIO_E3 | GPIO_PULLUP_EN | GPIO_MODE_IISDI);//GPE 3: IISSDI
    set_gpio_ctrl(GPIO_E0 | GPIO_PULLUP_EN | GPIO_MODE_IISDI);//GPE 0: IISLRCK
    set_gpio_ctrl(GPIO_E1 | GPIO_PULLUP_EN | GPIO_MODE_IISSCLK);//GPE 1: IISSCLK
    set_gpio_ctrl(GPIO_E2 | GPIO_PULLUP_EN | GPIO_MODE_CDCLK);//GPE 2: CDCLK
    set_gpio_ctrl(GPIO_E4 | GPIO_PULLUP_EN | GPIO_MODE_IISSDO);//GPE 4: IISSDO

    local_irq_restore(flags);

    init_uda1341();

    //输出流采样DMA通道2
    output_stream.dma_ch = DMA_CH2;  
    if (audio_init_dma(&output_stream, "UDA1341 out")){
        audio_clear_dma(&output_stream);
        printk(KERN_WARNNING AUDIO_NAME_VERBOSE ": unable to get DMA channels\n");
        return -EBUSY;
    }

    //输入流采样DMA通道1
    input_stream.dma_ch = DMA_CH1;
    if (audio_init_dma(&input_stream, "UDA1341 in")){
        audio_clear_dma(&input_stream);
        printk(KERN_WARNNING AUDIO_NAME_VERBOSE ": unable to get DMA channels\n");
        return -EBUSY;
    }

    //注册dsp和mixer设备接口
    audio_dev_dsp = register_sound_dap(&smdk2410_audio_fops, -1);
    audio_dev_mixer = register_sound_mixer(&smdk2410_mixer_fops, -1);

    printk(AUDIO_NAME_VERBOSE " initialized\n");

    return 0;
}

static __exit s3c1410_uda1341_exit(void)
{
    //注销dsp和mixer设备接口
    unregister_sound_dsp(audio_dev_dsp);
    unregister_sound_mixer(audio_dev_mixer);

    //注销DMA通道
    audio_clear_dma(&output_stream);
    audio_clear_dma(&input_stream);
    printk(AUDIO_NAME_VERBOSE " unloaded\n");
}