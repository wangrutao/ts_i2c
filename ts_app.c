#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>    //lseek
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h> //ioctl
#include <linux/input.h> //input_event

#if 0   //输入子系统返回的数据都是以下结构形式返回
struct input_event {
    struct timeval time;     //存放时间戳，占8字节
    __u16 type;            //表示事件类型
    __u16 code;           //键码，和type有关
    __s32 value;           //键值，和code有关
};
#endif

//./btn_app /dev/input/event3

struct input_event key_buf;

int main(int argc, char*argv[]) {
    int fd;                      //存放文件描述符号
    int ret;
    int i;

    if(argc != 2) {
        printf("Usage:%s /dev/input/eventX\r\n", argv[0]);
        return -1;
    }
    printf("open %s \r\n", argv[1]);

    fd = open(argv[1], O_RDWR); //以读写方式进行打开
    if(fd < 0) {
        printf("open error\r\n");
        return -1;
    }

    //实际程序需要循环读取按键动作，然后根据动作完成不同事情
    while(1) {
        ret = read(fd, &key_buf, sizeof(key_buf));

        switch(key_buf.type) {
            case EV_SYN:
                printf("EV_SYN \r\n");
                break;
            case EV_KEY:
                if(key_buf.code == BTN_TOUCH) {
                    printf("TBN_TOUCH:%s\r\n", key_buf.value ? "press" : "up");
                }
                break;
            case EV_ABS:
                switch(key_buf.code) {
                    case ABS_X:
                        printf("ABS_X:%d\r\n", key_buf.value);
                        break;
                    case ABS_Y:
                        printf("ABS_Y:%d\r\n", key_buf.value);
                        break;
                }
                break;
            default:
                break;
        }
    }

    //关闭文件
    close(fd);

    return 0;
}






