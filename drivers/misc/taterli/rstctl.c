#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/cdev.h>
#include <linux/of.h>         /* dts操作相关 */
#include <linux/of_address.h> /* dts地址相关 */
#include <linux/of_gpio.h>    /* gpio子系统相关 */
#include <linux/platform_device.h>

struct rstio_dev
{
    struct device_node *nd; /* 设备节点 */
    int gpio;               /* gpio编号 */
    enum of_gpio_flags  gpio_flags;
};

static int rstio_init(struct platform_device *mdev)
{
    static struct rstio_dev dev;
    int ret;

    /* 新增的从dts获取数据的过程 */
    dev.nd = mdev->dev.of_node;

    /* IO当然也可以是一个数组 */
    dev.gpio = of_get_named_gpio_flags(dev.nd, "gpio", 0, &dev.gpio_flags);
    if (!gpio_is_valid(dev.gpio))
    {
        /* IO是独占资源,因此可能申请失败! */
        return -EINVAL;
    }
    /* 申请IO并给一个名字 */
    ret = gpio_request(dev.gpio, dev.nd->name);
    if (ret < 0)
    {
        /* 除了返回EINVAL,也可以返回上一层传递的错误. */
        return ret;
    }

    dev_info(&mdev->dev, "reset start.\n");
    
    gpio_direction_output(dev.gpio, 0);
    msleep(500);
    gpio_direction_output(dev.gpio, 1);

    dev_info(&mdev->dev, "reset end.\n");

    /* 这里就清爽很多了,释放IO就行. */
    gpio_free(dev.gpio);
    return ret;
}

static int rstio_exit(struct platform_device *mdev)
{
    return 0;
}

/* 匹配列表 */
static const struct of_device_id rstpin_of_match[] = {
    {.compatible = "taterli,rst-pin"},
    {}
};

static struct platform_driver rst_driver = {
    .driver = {
		.name = "taterli-rst-pin", /* 即使不用也要保留一个!  */
        .of_match_table = rstpin_of_match,
    },
    .probe = rstio_init,
    .remove = rstio_exit,
};

static int __init rst_init(void)
{
    return platform_driver_register(&rst_driver);
}

static void __exit rst_exit(void)
{
    platform_driver_unregister(&rst_driver);
}

module_init(rst_init);
module_exit(rst_exit);

MODULE_AUTHOR("Taterli <admin@taterli.com>");
MODULE_DESCRIPTION("Led GPIO");
MODULE_LICENSE("GPL");