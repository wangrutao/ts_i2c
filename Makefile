#hello是模块名，也是对应的c文件名。 
obj-m += ts_i2c_driver.o   ts_i2c_client.o  

#KDIR  内核源码路径，根据自己需要设置
#X86 源码路径统一是/lib/modules/$(shell uname -r)/build
#如果要编译ARM的模块，则修改成ARM的内核源码路径
#KDIR  :=/lib/modules/$(shell uname -r)/build
KDIR  := /a_linux_station/kernel/linux-3.5

all:
	@make -C $(KDIR) M=$(PWD) modules   
	@rm -rf  .tmp_versions *.o *.mod.o *.mod.c  *.bak *.symvers *.markers *.unsigned *.order *~ .*.*.cmd .*.*.*.cmd
	arm-linux-gcc ts_app.c -o ts_app
	cp ts_app *.ko /root/work/rootfs/home
	
clean:
	make -C  $(KDIR)   M=$(PWD)  modules  clean
	@rm -rf btn_app *.ko .tmp_versions *.o *.mod.o *.mod.c  *.bak *.symvers *.markers *.unsigned *.order *~ .*.*.cmd .*.*.*.cmd
