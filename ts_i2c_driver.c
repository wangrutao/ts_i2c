#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>

//保存匹配的i2c_client给其他函数使用
struct i2c_client * clt;

//定义工作结构
struct work_struct ft5x06_work;

//定义input_dev 结构指针
struct input_dev* p_input_dev;

int test_and_set(struct i2c_client *client) {
    char subaddr = 0xA6;   //内部地址
    int ret ;
    s32  val ;

    //第一种方式:
    //读取Firmware ID , 寄存器地址是0xA6
    ret = val  = i2c_smbus_read_byte_data(client, subaddr);
    if(val < 0) {
        printk("error:file:%s,line:%d\r\n", __FILE__, __LINE__);
        return ret;
    }
    printk("Firmware ID:%d\r\n", val);

    //第二种方式:
    //1)先发送内部地址
    ret = i2c_master_send(client, &subaddr, 1); //从机地址+写  +  内部地址
    if(ret  < 0) {
        printk("error:file:%s,line:%d\r\n", __FILE__, __LINE__);
        return ret;
    }

    //2)开始读数据
    ret = i2c_master_recv(client, (char *)&val, 1); //从机地址+读  + 接收数据
    if(ret  < 0) {
        printk("error:file:%s,line:%d\r\n", __FILE__, __LINE__);
        return ret;
    }
    printk("Firmware ID:%d\r\n", val);

    //第三种方式:把上面的第二种方式的两个步骤分别使用一个 i2c_msg 来表达
    struct i2c_msg msg[2];
    msg[0].addr  = clt->addr;   //器件地址
    msg[0].flags = 0;              //7位地址，写方向
    msg[0].len   = 1;              //要发送的字节数量
    msg[0].buf   = &subaddr;       //发送的数据缓冲区

    msg[1].addr  = clt->addr;   //器件地址
    msg[1].flags = I2C_M_RD;       //7位地址，读方向
    msg[1].len   = 1;              //要接收的字节数量
    msg[1].buf   = (char *)&val;   //接收的数据缓冲区

    ret = i2c_transfer(client->adapter, msg, 2)  ;
    if(ret  < 0) {
        printk("error:file:%s,line:%d\r\n", __FILE__, __LINE__);
        return ret;
    }
    printk("Firmware ID:%d\r\n", val);

    return 0;

}


//工作队列函数,负责完成原来应该在中断程序中做的事情,读取数据
void ft5x06_work_func(struct wrok_struct *data) {
    int ret ;
    int point;            //存放当前的触摸点数量
    int x, y;             //存放当前的x,y坐标
    char buf[7];          //存放数据的缓冲区
    char subaddr = 0;     //内部寄存器地址
    struct i2c_msg msg[2];//消息数组

    //读取坐标:为了简单，节省时间，这里只读取一个点
    //第三种方式:把上面的第二种方式的两个步骤分别使用一个 i2c_msg 来表达
    msg[0].addr  = clt->addr;      //器件地址
    msg[0].flags = 0;              //7位地址，写方向
    msg[0].len   = 1;              //要发送的字节数量
    msg[0].buf   = &subaddr;       //发送的数据缓冲区

    msg[1].addr  = clt->addr;      //器件地址
    msg[1].flags = I2C_M_RD;       //7位地址，读方向
    msg[1].len   = 7;              //要接收的字节数量
    msg[1].buf   = buf;            //接收的数据缓冲区

    ret = i2c_transfer(clt->adapter, msg, 2)  ;
    if(ret  < 0) {
        printk("error:file:%s,line:%d\r\n", __FILE__, __LINE__);
    }

    //判断触摸屏触摸点数量
    point = buf[2] & 0x0f;    //取低4位，实际取3位就够了，因为这个芯片最多5点

    if(!point) {  
        input_report_key(p_input_dev, BTN_TOUCH, 0);    //触点松开
        input_report_abs(p_input_dev, ABS_PRESSURE, 0); //压力值为0
        input_sync(p_input_dev);                        //同步事件
    } else if(point == 1) {
        x = (s16)(buf[3] & 0x0f) << 8  | buf[4];
        y = (s16)(buf[5] & 0x0f) << 8  | buf[6];
   
        input_report_abs(p_input_dev, ABS_X, x);        //上报X轴坐标
        input_report_abs(p_input_dev, ABS_Y, y);        //上报Y轴坐标
        input_report_abs(p_input_dev, ABS_PRESSURE, 1); //压力值为1
        input_report_key(p_input_dev, BTN_TOUCH, 1);    //触点按下状态
        input_sync(p_input_dev);                        //同步事件
    }
}

//中断服务函数
//不能在中断服务函数调用 i2c_transfer函数，因为它会导致休眠
//所以，要把中断程序分成两部分:中断顶半部和中断底半部
//形式:可以使用工作队列方式实现
irqreturn_t  ft5x0x_irq_handler(int irq, void *id) {
    //调度工作，启动中断底半部
    schedule_work(&ft5x06_work);

    return IRQ_HANDLED;
}

/* 探测函数，在i2c_driver和i2c_client匹配时候会被调用，
  *匹配依是i2c_client.name和i2c_device_id.name相同就匹配上了，
  *匹配后传递下来参数 i2c_client就是和它匹配 i2c_client结构地址
*/
int ft5x0x_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret ;
    unsigned int irq;
    unsigned long flags;

    printk("File:%s\r\nline:%d, %s is call!\r\n", __FILE__, __LINE__, __FUNCTION__);

    printk("client->name:%s,client->addr:0x%x\r\n", client->name, client->addr);

    //保存匹配的 i2c_client 到全局变量中，给其他函数使用，像file_operations实现
    clt = client;

    //初始化工作结构
    INIT_WORK(&ft5x06_work, ft5x06_work_func);

    //读取固件ID
    ret = test_and_set(client);
    if(ret  < 0) {
        printk("error:file:%s,line:%d\r\n", __FILE__, __LINE__);
        return ret;
    }

    //注册触摸屏中断
    flags = IRQ_TYPE_EDGE_FALLING | IRQF_DISABLED;  //下降沿，独占中断
    irq   = IRQ_EINT(14) ;   //中断编号
    ret   = request_irq(irq, ft5x0x_irq_handler, flags, "irq_ft5x06", NULL);
    if(ret  < 0) {
        printk("error:file:%s,line:%d\r\n", __FILE__, __LINE__);
        return ret;
    }

    //实现输入设备注册
    //1) 分配input_dev结构空间
    p_input_dev = input_allocate_device();
    if(NULL == p_input_dev) {
        ret = -ENOMEM;
        goto  error_input_allocate_device;
    }

	  p_input_dev->name = "ts_ft5x06";               //设置设备名

    //2) 填充输入设备支持的事件类型
    set_bit(EV_KEY, p_input_dev->evbit);
    set_bit(EV_ABS, p_input_dev->evbit);

    //3) 根据输入设备支持事件类型填充支持的具体键值(哪一个按键，哪一个轴)
    set_bit(ABS_X, p_input_dev->absbit);            //支持 X轴
    set_bit(ABS_Y, p_input_dev->absbit);            //支持 Y轴
    set_bit(ABS_PRESSURE, p_input_dev->absbit);     //支持 Y轴
    set_bit(BTN_TOUCH, p_input_dev->keybit);        //支持 BTN_TOUCH 按键

    //3) 对于触摸屏设备还需要再设置各个轴的有效数值范围
    input_set_abs_params(p_input_dev, ABS_X, 0, 799, 0, 0); //设置X方向有效范围
    input_set_abs_params(p_input_dev, ABS_Y, 0, 479, 0, 0); //设置Y方向有效范围
    input_set_abs_params(p_input_dev, ABS_PRESSURE, 0, 1, 0, 0); //设置ABS_PRESSURE有效范围


    //4) 注册输入设备
    ret = input_register_device(p_input_dev);
    if(ret < 0) {
        goto  error_input_register_device;
    }

    //5) 在适当的地方上报数据，这里不是适当的地，适当地方写中断底半部

    return 0;

error_input_register_device:
    input_free_device(p_input_dev);
error_input_allocate_device:
    free_irq(irq, NULL);      //释放中断


}


int ft5x0x_remove(struct i2c_client *client) {
    printk("File:%s\r\nline:%d, %s is call!\r\n", __FILE__, __LINE__, __FUNCTION__);
    printk("client->name:%s,client->addr:0x%x\r\n", client->name, client->addr);
    //6) 注销输入设备
    input_unregister_device(p_input_dev);

    //释放input_dev结构空间
    input_free_device(p_input_dev);

    //释放中断
    free_irq(IRQ_EINT(14) , NULL);
    return 0;
}

//定义本驱动支持设备列表，这里的name和client.name进行比较，相同表示支持
const struct i2c_device_id  ft5x0x_id_table[] = {
    {"ft5x0x_ts", 0}, 
    {}   //表示结束了
};

//定义 i2c_driver 结构，并且初始必须成员
static struct  i2c_driver  ft5x0x_driver = {
    .driver = {
        .owner  = THIS_MODULE,
        .name  = "WRT_TS",
    },
    .probe    = ft5x0x_probe,
    .remove   = ft5x0x_remove,
    .id_table = ft5x0x_id_table,
};

//模块加载函数
static int __init ft5x0x_dev_init(void) {
    int ret = 0;

    //注册i2c_driver
    ret = i2c_add_driver(&ft5x0x_driver);
    if(ret < 0 ) {
        printk("error:i2c_add_driver\r\n");
    }

    return ret;
}

//模块卸载函数
static void __exit ft5x0x_dev_exit(void) {
    i2c_del_driver(&ft5x0x_driver); //注销i2c_driver
}

module_init(ft5x0x_dev_init);
module_exit(ft5x0x_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wrt");
MODULE_DESCRIPTION("Touch ft5x06 driver");
MODULE_VERSION("v1.4");




