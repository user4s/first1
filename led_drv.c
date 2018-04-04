#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>		// --->cdev
#include <linux/fs.h>		// --->file_operations
#include <linux/errno.h>	// 错误码
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>


// 1、定义一个字符设备的结构体
struct cdev gec6818_led_dev;

static unsigned int led_major = 0;	// 主设备号
static unsigned int led_minor = 0;	// 次设备号
static dev_t led_num;	// 设备号

static struct class * leds_class;			// class
static struct device * leds_device;
static struct resource * leds_res;
static void __iomem * gpioc_base_va;
static void __iomem * gpiocout_va;		// 0x00
static void __iomem * gpiocoutenb_va;	// 0x04
static void __iomem * gpiocaltfn0_va;	// 0x20
static void __iomem * gpiocaltfn1_va;	// 0x24
static void __iomem * gpiocpad_va;	// 0x18


/*
* 函数名称：gec6848_led_open
* 函数功能：打开led函数
* 函数参数：
*		struct inode * 节点指针
*		struct file  * 文件结构体指针
* 返回值：
*    	
* 说明：函数的原型必须根据内核规定的模型来
*/
int gec6848_led_open(struct inode *inode, struct file *filep)
{
	printk(KERN_WARNING "led_driver open\n");

	return 0;
}

/*
* 函数名称：gec6818_led_read
* 函数功能：led_read函数
* 函数参数：
*		struct file * 文件结构体指针（在linux中一切皆文件）
*		char __user * 用户程序里面的数据
*       size_t		 应用程序读取的字节数
*       loff_t *		 文件移动指针
* 返回值：
*    	
* 说明：
*/
ssize_t gec6818_led_read(struct file *filep, char __user *user_buf, size_t size, loff_t *off)
{
	
	return 0;
}

/*
* 函数名称：gec6818_led_write
* 函数功能：led_write函数
* 函数参数：
*		struct file * 文件结构体指针（在linux中一切皆文件）
*		char __user * 用户程序里面的数据(应用程序写下来的数据)
*       size_t		 应用程序写的字节数
*       loff_t *		 文件移动指针
* 返回值：
*    	
* 说明：
* 定义一个数据的格式：user_buf[1] ---》LED灯号：8/9/10/11
*					  user_buf[0] ---》LED灯的状态：1亮  0灭
*/
ssize_t gec6818_led_write(struct file *filep, const char __user *user_buf, size_t size, loff_t *off)
{
	// 接收用户写下来的数据，并利用这些数据来控制LED灯
	char kbuf[2];
	int ret;
	
	if(size != 2)
		return -EINVAL;

	ret = copy_from_user(kbuf, user_buf, size);	// 得到用户空间的数据
	if(ret != 0)
		return -EFAULT;
		
	switch(kbuf[1]){
	case 8:
		if(user_buf[0] == 1)
			*(unsigned int *)gpiocout_va &= ~(1<<7);
		else if(user_buf[0] == 1)
			*(unsigned int *)gpiocout_va |= (1<<7);
		else 
			return -EINVAL;
		break;
	case 9:
		if(user_buf[0] == 1)
			*(unsigned int *)gpiocout_va &= ~(1<<8);
		else if(user_buf[0] == 1)
			*(unsigned int *)gpiocout_va |= (1<<8);
		else 
			return -EINVAL;
		break;
	case 10:
		if(user_buf[0] == 1)
			*(unsigned int *)gpiocout_va &= ~(1<<12);
		else if(user_buf[0] == 1)
			*(unsigned int *)gpiocout_va |= (1<<12);
		else 
			return -EINVAL;
		break;
	case 11:
		if(user_buf[0] == 1)
			*(unsigned int *)gpiocout_va &= ~(1<<17);
		else if(user_buf[0] == 1)
			*(unsigned int *)gpiocout_va |= (1<<17);
		else 
			return -EINVAL;
		break;
	}

	return 0;
}

/*
* 函数名称：gec6818_led_release
* 函数功能：led_release函数
* 函数参数：
*		struct inode * 节点指针
*		struct file  * 文件结构体指针
* 返回值：
*    	
* 说明：
*/
int gec6818_led_release(struct inode *inode, struct file *filep)
{
	printk(KERN_WARNING "led_driver release\n");

	return 0;
}

// 2、定义一个文件操作集并初始化
static const struct file_operations gec6818_led_fops = {
	.owner = THIS_MODULE,
	.open  = gec6848_led_open,
	.read  = gec6818_led_read,
	.write = gec6818_led_write,
	.release = gec6818_led_release,
};


// 使用static修饰，防止驱动名称与其它的驱动名称重名
// 入口函数---》安装驱动
static int __init gec6818_led_init(void)
{
	int ret;

	// 3、申请设备号
	if(led_major == 0){	// 主设备是不能为0的，所以一旦主设备为0，动态分配
		ret = alloc_chrdev_region(&led_num, led_minor, 1, "led_device");	
	}else{				// 静态注册
		led_num = MKDEV(led_major, led_minor);
		ret = register_chrdev_region(led_num, 1, "led_device");	
	}
	if(ret < 0){
		printk(KERN_WARNING "can not get a device number\n");
		return ret;
	}
	
	// 4、字符设备的初始化
	cdev_init(&gec6818_led_dev, &gec6818_led_fops);
	
	// 5、将字符设备加入内核
	ret =  cdev_add(&gec6818_led_dev, led_num, 1);
	if(ret < 0){
		unregister_chrdev_region(led_num, 1);
		printk(KERN_WARNING "can not add cdev \n");
		return ret;
	}
	
	// 6、创建class
	leds_class = class_create(THIS_MODULE, "gec210_leds");
	if(leds_class == NULL){
		printk(KERN_WARNING "class create error\n");
		unregister_chrdev_region(led_num, 1);	// 注销设备号
		cdev_del(&gec6818_led_dev);
		
		return -EBUSY;
	}
	
	// 7、创建device
	leds_device = device_create(leds_class, NULL,
			     led_num, NULL, "led_drv");
	if(leds_device < 0){
		class_destroy(leds_class);
		cdev_del(&gec6818_led_dev);
		unregister_chrdev_region(led_num, 1);
		printk(KERN_WARNING "device_create failed\n");
		
		return -EBUSY;
	}
	
	// 8、申请物理内存区
	leds_res = request_mem_region(0xC001C000, 0x1000, "GPIOC_MEM");
	if(leds_res == NULL){
		printk(KERN_WARNING"request mem error\n");
		device_destroy(leds_class, led_num);
		class_destroy(leds_class);
		cdev_del(&gec6818_led_dev);
		unregister_chrdev_region(led_num, 1);
		
		return -EBUSY;
	}
	
	// 9、IO内存映射，得到物理内存对应的虚拟地址
	gpioc_base_va = ioremap(0xC001C000, 0x1000);
	if(leds_res == NULL){
		printk(KERN_WARNING"ioremap error\n");
		device_destroy(leds_class, led_num);
		class_destroy(leds_class);
		cdev_del(&gec6818_led_dev);
		unregister_chrdev_region(led_num, 1);
		
		return -EBUSY;
	}
	// 得到每个寄存器的虚拟地址
	gpiocout_va = gpioc_base_va + 0x00;		// 0x00
	gpiocoutenb_va = gpioc_base_va + 0x04;	// 0x04
	gpiocaltfn0_va = gpioc_base_va + 0x20;	// 0x20
	gpiocaltfn1_va = gpioc_base_va + 0x24;	// 0x24
	gpiocpad_va = gpioc_base_va + 0x18;	// 0x18
	
	printk(KERN_WARNING"gpiocout_va = %p, gpiocpad_va = %p\n", gpiocout_va, gpiocpad_va);
	
	// 10、访问虚拟地址
	// 10.1 GPIOC7,8,12,17 function1，作为普通GPIO
	*(unsigned int *)gpiocaltfn0_va &= ~((3<<14)|(3<<16)|(3<<24));
	*(unsigned int *)gpiocaltfn1_va &= ~(3<<2);
	*(unsigned int *)gpiocaltfn0_va |= (1<<14|(1<<16)|(1<<24));
	*(unsigned int *)gpiocaltfn1_va |= (1<<2);
	// 10.2  GPIOC7,8,12,17 作为输出
	*(unsigned int *)gpiocoutenb_va |= ((1<<7)|(1<<8)|(1<<12)|(1<<17));
	
	// 10.3 GPIOC7,8,12,17 输出高电平
	*(unsigned int *)gpiocout_va |= ((1<<7)|(1<<8)|(1<<12)|(1<<17));
	
	printk(KERN_WARNING "gec6818_led_init \n");
	
	return 0;
}

// 出口函数---》卸载驱动
static void __exit gec6818_led_exit(void)
{
	iounmap(gpioc_base_va);
	release_mem_region(0xC001C000, 0x1000);
	device_destroy(leds_class, led_num);
	class_destroy(leds_class);
	cdev_del(&gec6818_led_dev);
	unregister_chrdev_region(led_num, 1);	// 注销设备号
	
	
	printk("<4>" "gec6818_led_exit \n");
}


// 驱动程序的入口 #insmod led_drv.ko --》module_init()--->gec6818_led_init()
module_init(gec6818_led_init);	
// 驱动程序的出口 #rmmod  led_drv.ko -->module_exit() --->gec6818_led_exit()
module_exit(gec6818_led_exit);	



// module的描述
MODULE_AUTHOR("user-s@163.com");
MODULE_DESCRIPTION("led driver for GEC6818");
MODULE_LICENSE("GPL");

// MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);	杂项设备
MODULE_ALIAS("led_dev");
MODULE_VERSION("v1.0");