/* char_driiver.c 
 * 
 * implementation of a character device driver 
 */


#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/module.h>	
#include <linux/fs.h>		
#include <linux/uaccess.h>	
#include <linux/init.h>		
#include <linux/cdev.h>		
#include <linux/list.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include "char_driv.h"

MODULE_AUTHOR("Sudeep Reddy"); 
MODULE_LICENSE("GPL v2");

#define MY_DEVICE_NAME "mycdrv"
#define ramdisk_size (size_t) (16*PAGE_SIZE)
#define SCULL_IOC_MAGIC  'k'
#define CDRV_IOC_MAGIC 'Z'
#define ASP_CHGACCDIR _IOW(CDRV_IOC_MAGIC, 1, int)


static struct asp_dev *my_asp_devices = NULL;

struct class *asp_class=NULL;
static unsigned int major_num = 0;

module_param(NUM_DEVICES, int, S_IRUGO);

struct asp_dev mylist;

/*
 * Name: asp_open
 * 
 * Input: inode ptr, file ptr
 * Description:
 * Used to open the dev driver with given minor num
 * 
 * returns 0 for success
 */
int asp_open(struct inode *inode, struct file *filp)
{
	struct list_head *pos;
	unsigned int mn = iminor(inode);
	struct asp_dev *cur_dev=NULL;
    list_for_each(pos, &(mylist.list)) {
        cur_dev = list_entry(pos, struct asp_dev, list);
        if(MINOR((cur_dev->dev).dev)==mn)
         break;
    }
    
    
	filp->private_data = cur_dev; 
	return 0;
}


/*
 * Name: asp_read
 * 
 * Input: file ptr, buf ptr, size, file pos
 * Description:
 * 
 * 
 * Reads based on the direction, checks for boundary conditions
 */
ssize_t asp_read(struct file *filp, char __user *buf, size_t count,loff_t *file_pos)
{
	struct asp_dev *dev = (struct asp_dev *)filp->private_data;
	ssize_t retval = 0;
	
	if (down_interruptible(&dev->sem))
		return -EINTR;
	
	if (*file_pos >= ramdisk_size) /* EOF */
		goto out;
	
	if(dev->dir==0)
	{
		printk("amouunt of string read %s",&dev->ramdisk[*file_pos]);
		if (*file_pos + count > ramdisk_size)
			   count = ramdisk_size - *file_pos;
			   
	    if (copy_to_user(buf, &(dev->ramdisk[*file_pos]), count) != 0) {
			 retval = -EFAULT;
			 goto out;
	    }
	    *file_pos += count;
    } else { 
        if(*file_pos - count<=0)
		   count = *file_pos;
    	
    	if (copy_to_user(buf, &(dev->ramdisk[*file_pos-count]), count) != 0) {

			retval = -EFAULT;
			goto out;
	    }
    		*file_pos -= count;
	}

	retval = count;
	goto out;
	
	
	
	out:
	up(&dev->sem);
	return retval;
}

/*
 * Name: asp_ioctl
 * 
 * Input: file ptr, id, param
 * Description:
 * Used to change the direction of read/write.
 * 
 * 
 */
static long asp_ioctl(struct file *file, unsigned int id, unsigned long param)
{
    struct asp_dev* mycdrv = file->private_data;
    
    int dir=0;
    switch(id) {
    case ASP_CHGACCDIR:
        down(&(mycdrv->sem));
        dir = mycdrv->dir;
        if(dir !=*(int *)param){
            mycdrv->dir = *(int *)param;
        }
        up(&(mycdrv->sem));
        break;
    }
    return dir;
}



	/*
 * Name: asp_write
 * 
 * Input: file ptr, buf ptr, size, file pos
 * Description:
 * Writes to the device
 * 
 * 
 */			
ssize_t asp_write(struct file *filp, const char __user *buf, size_t count, loff_t *file_pos)
{
	struct asp_dev *dev = (struct asp_dev *)filp->private_data;
	ssize_t retval = 0;
	
	if (down_interruptible(&dev->sem))
	{
		printk("===Sempaphore Erro====");
		return -EINTR;
	}
	
	if (*file_pos >= ramdisk_size) {
		printk("Error: file_pos beyond ramdisk size");
		retval = -EINVAL;
		goto out;
	}
	
	if(dev->dir==0)
	{
		if (*file_pos + count > ramdisk_size)
		    count = ramdisk_size - *file_pos;
        if (copy_from_user(&(dev->ramdisk[*file_pos]), buf, count) != 0)
	      {
		   retval = -EFAULT;
		   goto out;
	     }
	     	*file_pos += count;
    }
    else
    {
    	 if(*file_pos - count<=0)
		   count = *file_pos;
    	if (copy_from_user(&(dev->ramdisk[*file_pos-count]), buf, count) != 0)
	      {
		   retval = -EFAULT;
		   goto out;
	     }
	     	*file_pos -= count;
    	
    	
	}
	printk("String value written:  %s",&dev->ramdisk[*file_pos]);
	retval = count;
	goto out;
	
out:
	up(&dev->sem);
	return retval;
}
	
/*
 * Name: asp_llseek
 * 
 * Input: file ptr, offset, whence
 * Description:
 * 
 * Seek to position depends on whence
 * 
 */


loff_t asp_llseek(struct file *filp, loff_t off, int whence)
{
	loff_t newpos = 0;
	
	switch(whence) {
	  case 0: 
		newpos = off;
		break;

	  case 1: 
		newpos = filp->f_pos + off;
		break;

	 case 2:
		newpos = ramdisk_size + off;
		break;
	 
	 default:
		return -EINVAL;
	}
	newpos = newpos < ramdisk_size ? newpos : ramdisk_size;
	newpos = newpos >= 0 ? newpos : 0;

	
	if (newpos < 0 || newpos > ramdisk_size) 
		return -EINVAL;
	
	filp->f_pos = newpos;
	return newpos;
}



struct file_operations asp = {
	.owner =    THIS_MODULE,
	.open =     asp_open,
		.read =     asp_read,
	.write =    asp_write,

	.llseek =   asp_llseek,
	.unlocked_ioctl=  asp_ioctl,
};

/*
 * Name: my_exit
 * 
 * Input: void
 * Description:
 * Called when module is removed/exited.
 * 
 * 
 */

static void __exit my_exit(void)
{
	int i=0;
	struct list_head *pos,*q;
	struct asp_dev *cur_dev=NULL;
	
	
	
	
    list_for_each(pos,&(mylist.list)) {
        cur_dev = list_entry(pos, struct asp_dev, list);
        if(cur_dev && cur_dev->ramdisk)
          {
			  kfree(cur_dev->ramdisk);
		  }
		 cdev_del(&(cur_dev->dev));
        device_destroy(asp_class,MKDEV(major_num,i));
        i++;
    }
    
    
    
    
	class_unregister(asp_class);
	if (asp_class)
		class_destroy(asp_class);
		
		
	unregister_chrdev_region(MKDEV(major_num, 0), NUM_DEVICES);
	list_for_each_safe(pos,q,&(mylist.list)) {
        cur_dev = list_entry(pos, struct asp_dev, list);
        list_del(pos);
        kfree(cur_dev);
    }
    
    
	return;
}


/*
 * Name: my_init_module
 * 
 * Input: Input File name
 * Description:
 * Called during init of module
 * 
 * 
 */

static int my_init_module(void)
{
	int err = 0;
	int i = 0;
	dev_t dev = 0;
	dev_t dev_no;
	err = alloc_chrdev_region(&dev, 0,NUM_DEVICES, MY_DEVICE_NAME);
	major_num = MAJOR(dev);
    struct device *device = NULL;
	asp_class = class_create(THIS_MODULE,MY_DEVICE_NAME);
	INIT_LIST_HEAD(&mylist.list);
	
	
	
	for (i = 0; i < NUM_DEVICES; ++i) {
		my_asp_devices = (struct asp_dev *)kzalloc(sizeof(struct asp_dev), GFP_KERNEL);
		INIT_LIST_HEAD(&my_asp_devices->list);
		
		
		list_add(&(my_asp_devices->list), &(mylist.list));
		dev_no = MKDEV(major_num, i);
        my_asp_devices->ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
	    my_asp_devices->devNo = i;
	    my_asp_devices->dir=0;
	    
	    //Semaphore init
	    sema_init(&my_asp_devices->sem, 1);
	    cdev_init(&my_asp_devices->dev, &asp);
	    
	    //Set owner
	    my_asp_devices->dev.owner = THIS_MODULE;
	    cdev_add(&my_asp_devices->dev, dev_no, 1);
	    device = device_create(asp_class, NULL, dev_no, NULL, MY_DEVICE_NAME "%d", i);
	}
	return 0; 
}


module_init(my_init_module);
module_exit(my_exit);

