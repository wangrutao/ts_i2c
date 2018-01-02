#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/i2c.h>

static struct i2c_client * clt;

//模块加载函数
static int __init ft5x0x_client_init(void) {
    int ret = 0;

    struct i2c_adapter *ad;
    struct i2c_board_info  info;                         //自己填充必须成员
    unsigned short  ft5x0x_i2c[] = { 0x38, I2C_CLIENT_END }; //

    //假设设备是挂载在i2c-1 总线，
    ad = i2c_get_adapter(1);

    //填充 i2c_board_info，一定保证和驱动中的id_table成员中的name相同
    strcpy(info.type, "ft5x0x_ts");

    //7位地址
    info.flags = 0;

    //动态创建一个i2c_client,并且注册
    clt = i2c_new_probed_device(ad, &info, ft5x0x_i2c, NULL);
    if(clt == NULL ) {
        ret = -EINVAL;
        printk("error:i2c_new_probed_device\r\n");
    }

    i2c_put_adapter(ad);   //减少adapter引用计数

    return ret;
}

//模块卸载函数
static void __exit ft5x0x_client_exit(void)
{
    i2c_unregister_device(clt);
}

module_init(ft5x0x_client_init);
module_exit(ft5x0x_client_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wrt");
MODULE_DESCRIPTION("Touch ft5x06 client");
MODULE_VERSION("v1.0");




