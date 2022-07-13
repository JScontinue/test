#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/delay.h>
#include <linux/pros_fs.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/wakelock.h>

#include <linux/iio/iio.h>
#include <linux/iio/machime.h>
#include <linux/iio/driver.h>
#include <linux/iio/consumer.h>

#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/rk_keys.h>



static irqreturn_t keys_isr(int irq, void *dev_id)
{
    struct keys_button *button = (struct keys_button *)dev_id;
    struct keys_drvdata *pdata = dev_get_drvdata(button->dev);
    struct input_dev *input = pdata->input;

    BUG_ON(irq != gpio_to_irq(button->gpio));
    if (button->wakeup && pdata->in_suspend){
        button->state = 1;
        key_dbg(pdata, "wakeup: %skey[%s]: repoet event[%d] state[%d]\n",
                        (button->type == TYPE_ADC)? "adc" : "gpio",
                        button->desc, button->code, button->state);
        input_event(input, EV_KEY, button->code, button->state);
        input_sync(input);
    }
    if (button->wakeup)
        wake_lock_timeout(&pdata->wake_lock, WAKE_LOCK_JIFFIES);
    mod_timer(&button->timer, jiffies + DEBOUN)
}

static const struct of_device_id key_match[] = {
    { .compatible = "rockchip,key", 
      .data = NULL},
    {},
};
MODULE_DEVICE_TABLE(of, key_match);/*把函数导出到用户空间*/

static int key_adc_iio_read(struct keys_drvdata *data)
{
    struct iio_channel *channel = data->chan;
    int val, ret;

    if (!channel)/*判断采样通道是否申请成功，失败返回无效值*/
        return INVALID_ADVALUE;

    ret = iio_read_channel_raw(channel, &val);/*读取ADC采集的数据并存入val*/
    if (ret < 0){
        pr_err("read channel() error: %d\n", ret);
        return ret;
    }
    return val;
}

static void adc_key_poll(struct work_struct *work)
{
    struct keys_drvdata *ddata;
    int i, result = -1;

    /*把adc_poll_work.work强制转化为struct keys_drvdata包含的结构*/
    ddata = container_of(work, struct keys_drvdata, adc_poll_work.work);
    if (!ddata->in_suspend){/*挂起为空*/
        result = key_adc_iio_read(ddata);/*读取ADC电压*/
        /*判断值是否有效*/
        if (result > INVALID_ADVALUE && 
            result < (EMPTY_DEFAULT_ADVALUE - ddata->drift_advalue))
            ddata->result = result;
        /*采样值与理想值是否相等*/
        for (i = 0; i < ddata->nbuttons; i++){
            struct keys_button *button = &ddata->button[i];
            if (!button->adc_value)
                continue;
            if (result < button->adc_value + ddata->drift_advalue &&
                result > button->adc_value - ddata->drift_advalue)
                button->adv_state = 1;
            if (button->state != button->adc_state)
                mod_timer(&button->timer, jiffies + DEBOUNCE_JIFFIES);
        }
    }
    /*周期性调用，延迟后将工作任务放入全局工作队列*/
    schedule_delayed_work(&ddata->adc_poll_work, ADC_SAMPLE_JIFFIES);
}


static int key_type_get(struct device_node *node, struct keys_button *button)
{
    u32 adc_value;

    if (!of_property_read_u32(node, "rockchip,adc_value", &adc_value))
        return TYPE_ADC;
    else if (of_get_gpio(node, 0) >= 0)
        return TYPE_GPIO;
    else
        return -1;
}

static int keys_parse_dt(struct keys_drcdata *pdata, struct platform_device *pdev)
{
    struct device_node *node = pdev->dev.of_node;
    struct device_node *child_node;
    struct iio_channel *chan;/*定义IIO通道结构体*/
    int ret, gpio, i = 0;
    u32 code, adc_value, flags, drift;

    /*获取ADC通道*/
    if (of_preperty_read_u32(node, "adc-drift", &drift))
        pdata->drift_advalue = DRIFT_DEFAULT_ADVALUE;
    else
        pdata->drift_advalue = (int)drift;
    
    /*获取ADC通道*/
    chan = iio_channel_get(&pdev->dev, NULL);
    if (IS_ERR(chan)){
        dev_info(&pdev->dev, "no io-channels defined\n");
        chan = NULL;
    }
    pdata->chan = chan;

    /*获取各个节点信息*/
    for_each_child_of_node(node, child_node){
        if (of_property_read_u32(child_node, "linux,code", &code)){
            dev_err(&pdev->dev, "Missing linux, code property in the DT\n");
            goto error_ret;
        }
        pdata->button[i].code = code;
        pdata->button[i].desc = of_get_property(child_node, "label", NULL);
        pdata->button[i].type = key_type_get(child_node, &pdata->button[i]);
        switch (pdata->button[i].type){
        case TYPE_GPIO:
            gpio = of_get_gpio_flags(child_node, 0, &flags);
            if (gpio < 0){
                ret = gpio;
                if (ret != -EPROBE_DEFER)
                    dev_err(&pdev->dev, "Failed to get gpio flags, error: %d\n", ret);
                    goto error_ret;
            }
            pdata->button[i].gpio = gpio;
            pdata->button[i].active_low = flags & OF_GPIO_ACTIVE_LOW;
            pdata->button[i].wakeup = !!of_get_property(child_node, "gpio-key,wakeup", NULL);
            break;
        
        case TYPE_ADC:
            if (of_property_read_u32(child_node, "rockchip,adc_value", &adc_value)){
                dev_err(&pdev->dev, "Missing rockchip, adc_value property in the DT.\n");
                ret = -EINVAL;
                goto error_ret;
            }
            pdata->button[i].adc_value = adc_value;
            break;

        default:
            dev_err(&pdev->dev, "Error rockchip, type property in the DT.\n");
            ret = -EINVAL;
            goto error_ret;
        }
        i++;
    }
    return 0;

error_ret:
    return ret;
}


static int keys_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *np = pdev->dev.of_node;
    struct keys_drvdata *ddata = NULL;
    struct input_dev *input = NULL;
    int i, error = 0;
    int wakeup, key_num = 0;

    //确认按键节点个数
    key_num = of_get_child_count(np);
    if (key_num == 0)
        dev_info(&pdev->dev, "no key defined\n");

    /*内核内存分配设备被拆卸或者驱动卸载时，内存会自动释放 -- devm_kzalloc */
    ddata = devm_kzalloc(dev, sizeof(struct keys_drvdata)
                +key_num*sizeof(struct keys_button), GFP_KERNEL);
    /*分配一个input类*/
    input = devm_input_alloc_device(dev);
    if (!ddata || !input){/*如果空间不足*/
        error = -ENOMEM;
        return error;
    }

    platform_set_drvdata(pdev, ddata);/*保存probe函数中定义的私有数据，保存局部变量*/
    dev_set_drvdata(&pdev->dev, ddata);/*设置device的私有数据*/
    input->name = "keypad";
    input->phys = "gpio-keys/input0";
    input->dev.parent = dev;

    input->id.bustype = BUS_HOST;
    input->id.vendor = 0x0001;
    input->id.product = 0x0001;
    input->id.version = 0x0100;
    ddata->input = input;

    ddata->nbuttons = key_num;
    /*从DTS中解析信息*/
    error = keys_parse_dt(ddata, pdev);
    if (error)
        goto fail0;
    /*启用Linux input子系统的自动重复功能*/
    if (ddata->rep)
        __set_bit(EV_REP, input->evbit);

    /*注册input设备*/
    error = input_register_device(input);
    if (error){
        pr_err("gpio-keys:Unable to register input device, error:%d\n", error);
        goto fail0;
    }
    sinput_dev = input;

    /*每个key都会注册一个定时器，用来处理状态变化并通知用户空间*/
    for (i = 0; i < ddata->nbuttons; i++){
        struct keys_button *button = &ddata->button[i];
        if (button->code){
            setup_time(&button->timer, keys_timer, (unsigned long)button);
        }
        if (button->wakeup)
            wakeup = 1;
        /*设置输入设备为EV_KEY类型*/
        input_set_capability(input, EV_KEY, button->code);
    }

    /*初始化一个新锁，阻止进入深度休眠模式*/
    wake_lock_init(&ddata->wake_lock, WAKE_LOCK_SUSPEND, input->name);
    /*赋予设备是否支持唤醒系统和唤醒系统的特征，为1时赋值*/
    device_init_wakeup(dev, wakeup);

    for (i = 0; i <ddata->nbuttons; i++){
        struct keys_button *button = &ddata->button[i];
        button->dev = &pdev->dev;
        if (button->type == TYPE_GPIO){
            int irq;
            /*申请gpio口的使用*/
            error = devm_gpio_request(dev, button->gpio, button->desc? :"keys");
            if (error < 0){
                pr_err("gpio-keys:failed to request GPIO %d, error %d\n", button->gpio, error);
                goto fail1;
            }

            /*设置gpio为输入功能*/
            error = gpio_direction_input(button->gpio);
            if (error < 0){
                pr_err("gpio-keys: failed to configure input direction for GPIO %d, error %d\n",
                                button->gpio, error);
                gpio_free(button->gpio);
                goto fail1;
            }

            /*申请获得对应硬件中断的软件中断号*/
            irq = gpio_to_irq(button->gpio);
            if (irq < 0){
                error = irq;
                pr_err("gpio-keys: Unable to get irq number for GPIO %d, error %d\n",
                            button->gpio, error);
                gpio_free(button->gpio);
                goto fail1;
            }

            /*申请对应的中断*/
            error = devm_request_irq(dev, irq, keys_isr,
                        button->active_low?IRQF_TRIGGER_FALLING:IRQF_TRIGGER_RISING,
                        button->desc?button->desc:"keys", button);
            if (error){
                pr_err("gpio-keys:Unable to claim ira %d; error %d\n", irq, error);
                gpio_free(button->gpio);
                goto fail1;
            }
        }
    }

    /*设置输入设备为EV_KEY类型，码表号为KEY_WAKEUP*/
    input_set_capability(input, EV_KEY, KEY_WAKEUP);
    /*adc polling work*/
    if (ddata->chan){
        /*初始化延迟的工作，指定工作函数adc_key_poll*/
        INIT_DELAYED_WORK(&ddata->adc_poll_work, adc_key_poll);
        /*经过一段延迟以后在提交任务到工作队列*/
        schedule_delayed_work(&ddata->adc_poll_work, ADC_SAMPLE_JIFFIES);
    }

    return error;

fail1:
    while(--i)/*删除定时器*/
        del_timer_sync(&ddata->button[i], timer);
    device_init_wakeup(dev, 0);/*不支持使用唤醒系统*/
    wake_lock_destory(&ddata->wake_lock);/*删除锁*/
fail0:
    platform_set_drvdata(pdev, NULL);/*把device的私有数据置空*/

    return error;
}

static int keys_remove(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct keys_drvdata *ddata = dev_get_drvdata(dev);
    struct input_dev *input = ddata->input;
    int i;

    device_init_wakeup(dev, 0);
    for (i = 0; i < ddata->nbuttons; i++)
        del_timer_sync(&ddata->button[i], timer);/*删除定时器*/
    if (ddata->chan)
        cancel_delayed_work_sync(&ddata->adc_poll_work);/*撤销延时工作队列*/
    input_unregister_device(input);/*撤销input类*/
    wake_lock_destroy(&ddata->wake_lock);/*删除锁*/
    sinput_dev = NULL;
    return 0;
}


static struct platform_driver keys_device_driver = {
    .probe = keys_probe,
    .remove = keys_remove,
    .driver = {
        .name = "keypad",
        .owner = THIS_MODULE,
        .of_match_table = key_match,
        #ifdef CONFIG_PM
            .pm = &keys_pm_ops,
        #endif
    }
};

static int __init keys_driver_init(void)
{
    /*根据名字寻找注册的设备与驱动进行匹配，匹配成功就运行探测函数*/
    return platform_driver_register(&keys_device_driver);
}

static void __exit keys_driver_exit(void)
{
    platform_driver_unregister(&keys_device_driver);
}

late_initcall_sync(keys_driver_init);
module_exit(keys_driver_exit);