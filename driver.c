/***************************************************************************//**
*  \file       driver.c
*
*  \details    Simple driver for pwm custom ip core
*
*  \author     Yosel de Jesus Balibrea Lastre
*
*  \Tested with Linux minized zynq fpga dev board
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  //copy_to/from_user()
#include <linux/of_device.h>  //compatible property
#include <linux/platform_device.h>
#include <linux/io.h>

#define MY_DEV_NAME "PWM_DRIVER"

u32 *pwm_base;
dev_t dev = 0;
static struct class *dev_class;
static struct cdev pwm_cdev;

static int __init pwm_driver_init(void);
static void __exit pwm_driver_exit(void);


/*************** Driver functions **********************/
static int pwm_open(struct inode *inode, struct file *file);
static int pwm_release(struct inode *inode, struct file *file);
static ssize_t pwm_read(struct file *filp,
                char __user *buf, size_t len,loff_t * off);
static ssize_t pwm_write(struct file *filp,
                const char *buf, size_t len, loff_t * off);

static int pwm_dev_probe(struct platform_device *pdev);
static int pwm_dev_remove(struct platform_device *pdev);
/******************************************************/

//File operation structure
static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = pwm_read,
  .write          = pwm_write,
  .open           = pwm_open,
  .release        = pwm_release,
};


/*
* Making the Driver Autoload
*/
static struct of_device_id pwm_of_match[] = {
  {.compatible = "xlnx,my-pwm-ip-c2-1.0", },
  {}
};

// Platform driver structure
static struct platform_driver pwm_platform_driver = {
  .probe = pwm_dev_probe,
  .remove = pwm_dev_remove,
  .driver = {
    .name = MY_DEV_NAME,
    .owner = THIS_MODULE,
    .of_match_table = pwm_of_match,
  },
};

// ASCII to NUM
unsigned int ascii_to_num(uint8_t data[10]){
    int d[4];
    int i;

    for(i = 0; i< 4; i++){
        
        if(data[i] < 0x30 || data[i] > 0x39){
            d[i] = 0;
        }else{
            d[i] = data[i] - 0x30;
        }
    }
    return d[3] + d[2] * 10 +d[1] * 100 + d[0] * 1000;
}

// Probe
static int pwm_dev_probe(struct platform_device *pdev){

  // get resource 
	struct resource *regs;
  
  printk("PWM custom IP probe\n");

  regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&pdev->dev, "could not get IO memory\n");
		return -ENXIO;
	}

  // get the pwm base register 
	pwm_base = ioremap(regs->start, resource_size(regs));
	if (!pwm_base) {
		dev_err(&pdev->dev, "could not remap memory\n");
		return -1;
	}
	printk("regs->start: %d, regs->end: %d, virt_mem_start: %p\n", regs->start, regs->end, pwm_base);


  return 0;
}

// Remove
static int pwm_dev_remove(struct platform_device *pdev){
  iounmap((void*)pwm_base);
  return 0;
}

/*
** This function will be called when we open the Device file
*/
static int pwm_open(struct inode *inode, struct file *file)
{
  pr_info("Device File Opened...!!!\n");
  return 0;
}

/*
** This function will be called when we close the Device file
*/
static int pwm_release(struct inode *inode, struct file *file)
{
  pr_info("Device File Closed...!!!\n");
  return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t pwm_read(struct file *filp,
                char __user *buf, size_t len, loff_t *off)
{
  uint8_t pwm_state = 0;

  //reading PWM DC value
  // TODO pwmC3 ...

  //write to user
  len = 1;
  if( copy_to_user(buf, &pwm_state, len) > 0) {
    pr_err("ERROR: Not all the bytes have been copied to user\n");
  }

  pr_info("Read function : PWM_Duty_Cycle = %d \n", pwm_state);

  return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t pwm_write(struct file *filp,
                const char __user *buf, size_t len, loff_t *off)
{
  uint8_t rec_buf[10] = {0};

  unsigned int duty = 0; 

  if( copy_from_user( rec_buf, buf, len ) > 0) {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
  }

  duty = ascii_to_num(rec_buf);

  pr_info("Write Function : PWM_Duty_Cycle Set = %c\n", rec_buf[0]);

  iowrite32((u32)rec_buf[0], pwm_base);

  return len;
}

/*
** Module Init function
*/
static int __init pwm_driver_init(void)
{
  /*
  if (platform_driver_register(&pwm_platform_driver) < 0){
    pr_err("Can not register the device\n");
    return -1;
  }  
  */
  
  if (platform_driver_probe(&pwm_platform_driver, pwm_dev_probe) < 0){
    pr_err("Device not properlly initialized\n");
    return -1;
  } 
  
  /*Allocating Major number*/
  if((alloc_chrdev_region(&dev, 0, 1, "pwm_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    
    unregister_chrdev_region(dev,1);
    return -1;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

  /*Creating cdev structure*/
  cdev_init(&pwm_cdev,&fops);

  /*Adding character device to the system*/
  if((cdev_add(&pwm_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    
    cdev_del(&pwm_cdev);
    unregister_chrdev_region(dev,1);
    return -1;
  }

  /*Creating struct class*/
  if((dev_class = class_create(THIS_MODULE,"pwm_class")) == NULL){
    pr_err("Cannot create the struct class\n");
    
    class_destroy(dev_class);
    cdev_del(&pwm_cdev);
    unregister_chrdev_region(dev,1);
    return -1;
  }

  /*Creating device*/
  if((device_create(dev_class,NULL,dev,NULL,"pwm_c2")) == NULL){
    pr_err( "Cannot create the Device \n");
    
    device_destroy(dev_class,dev);
    class_destroy(dev_class);
    cdev_del(&pwm_cdev);
    unregister_chrdev_region(dev,1);
    return -1;
  }

  pr_info("Device Driver Insert...Done!!!\n");
  return 0;

}

/*
** Module exit function
*/
static void __exit pwm_driver_exit(void)
{
  platform_driver_unregister(&pwm_platform_driver);
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&pwm_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Device Driver Remove...Done!!\n");
}

module_init(pwm_driver_init);
module_exit(pwm_driver_exit);

MODULE_DEVICE_TABLE(of, pwm_of_match);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yosel <yosel.balibrea@reduc.edu.cu>");
MODULE_DESCRIPTION("A simple device driver - PWM IP Driver");
MODULE_VERSION("1.0");
